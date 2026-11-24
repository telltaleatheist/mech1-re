#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_heap.h"
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <vector>

/* The combat near heap (B5, docs/phase5/NO_EMULATION_PLAN.md): 0810's allocator (E980, FAAD, FB90, FBCA, FBEC) kept as
 * an accounting allocator over 16-bit offsets, with nothing behind them, so a request fails exactly where the
 * original's fails (step 0 measured the play: the 15044-byte bitmap workspace fails on every combat's screens, and a
 * random allocate/release run shows the allocator's fragmentation can decide a failure a byte budget would grant).
 * The objects are ordinary host allocations, owned here. A released object lives until the combat's heap is
 * discarded: the sound code may still read a released node (its link, which it keeps: step 0 saw none in play). A new
 * request of the same size, kind and place takes a released one over, so a fight that loads and frees the same screens
 * again and again keeps a bounded number (runtime_test). */
namespace {
/* The header words by offset. A word written at an odd offset beside another overlaps its byte, as in DS. */
std::map<uint16_t,uint16_t> headers;
struct NearBlock {
    std::unique_ptr<std::byte[]> object;
    size_t hostBytes;
    uint16_t offset,bytes;
    BtechNearOwner owner;
    bool released;
};
std::vector<NearBlock> blocks;
}

static uint16_t NearHeapHeader(uint16_t offset)
{
    auto found=headers.find(offset);
    if (found!=headers.end()) return found->second;
    std::fprintf(stderr,"Unrepresented near-heap header %04X\n",offset);
    /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("A header that aliases an unrepresented payload is not zero.");
}
static void SetNearHeapHeader(uint16_t offset,uint16_t value)
{
    auto below=headers.find((uint16_t)(offset-1)),above=headers.find((uint16_t)(offset+1));
    if (below!=headers.end()) below->second=(below->second&0x00FF)|(uint16_t)((value&0x00FF)<<8);
    if (above!=headers.end()) above->second=(above->second&0xFF00)|(value>>8);
    headers[offset]=value;
}
void BtechHostDiscardNearHeap(void)
{
    blocks.clear();headers.clear();
    combatNearHeap.nearHeapFirstHeader=combatNearHeap.nearHeapSearchHeader=combatNearHeap.nearHeapEnd=0;
    combatNearHeap.nearHeapProgramBreak=BTECH_NEAR_INITIAL_BREAK;combatNearHeap.nearHeapGrowthAlignment=BTECH_NEAR_GROWTH_ALIGNMENT;
}

/* FBEC->FD38/FDA6, the supported near DS resident block: additions that wrap
 * fail. DFCE has already retained all64KiB, so no DOS resize is needed here. */
static bool ExtendProgramBreak(uint16_t bytes,uint16_t *previous)
{
    if ((uint32_t)combatNearHeap.nearHeapProgramBreak+bytes>UINT16_MAX) return false;
    *previous=combatNearHeap.nearHeapProgramBreak;combatNearHeap.nearHeapProgramBreak+=bytes;return true;
}
/* FBCA. A zero-length query returns the break without touching heap headers.
 * Transient odd/overlapping header writes are significant during alignment. */
static bool ExtendNearHeap(uint16_t bytes,uint16_t *position,uint16_t *payload,uint16_t *oldPayload)
{
    uint16_t previous;
    if (!ExtendProgramBreak(bytes,&previous)) return false;
    if (!bytes) {*oldPayload=previous;return true;} /* FBEC returns ZF from the requested length. */
    *oldPayload=*payload;*payload=*position;*position=previous+bytes;
    SetNearHeapHeader((uint16_t)(*position-2),0xFFFE);
    combatNearHeap.nearHeapEnd=*position;
    SetNearHeapHeader((uint16_t)(*payload-2),(uint16_t)(*position-*payload-1));
    return true;
}
/* FB90. Round growth using the original descending power-of-two mask, falling
 * back to smaller requests rather than silently granting host-malloc space. */
static bool GrowNearHeap(uint16_t requested,uint16_t *position,uint16_t *payload,uint16_t *oldPayload)
{
    uint16_t needed=requested,header=NearHeapHeader((uint16_t)(*payload-2));
    if (header&1) needed=(uint16_t)(needed-header-1);
    needed+=2;
    uint16_t mask=0x7FFF;
    while (mask>combatNearHeap.nearHeapGrowthAlignment && mask) mask>>=1;
    for (;;)
    {
        uint32_t end=(uint32_t)needed+*position;
        if (end<=UINT16_MAX && end+mask<=UINT16_MAX)
        {
            uint16_t rounded=(end+mask)&(uint16_t)~mask;
            uint16_t bytes=rounded-*position;
            if (bytes && ExtendNearHeap(bytes,position,payload,oldPayload)) return true;
        }
        if (!mask) return false;
        mask>>=1;
    }
}

static uint16_t FindNearHeapBlock(uint16_t bytes);
/* E980. Initialize the shared heap only on its first allocation request. */
static uint16_t AllocateNearHeapBlock(uint16_t bytes)
{
    if (!combatNearHeap.nearHeapFirstHeader)
    {
        uint16_t previous;
        if (!ExtendProgramBreak(5,&previous)) return 0;
        combatNearHeap.nearHeapFirstHeader=combatNearHeap.nearHeapSearchHeader=(uint16_t)((previous+1)&0xFFFE);
        SetNearHeapHeader(combatNearHeap.nearHeapFirstHeader,1);
        combatNearHeap.nearHeapEnd=combatNearHeap.nearHeapFirstHeader+4;SetNearHeapHeader((uint16_t)(combatNearHeap.nearHeapEnd-2),0xFFFE);
    }
    return FindNearHeapBlock(bytes);
}
/* FAAD. Search from the shared cursor, then restart once. Coalescing is
 * demand-driven; releasing a block alone does not merge it with its neighbour. */
static uint16_t FindNearHeapBlock(uint16_t bytes)
{
    uint16_t requested=bytes+1;
    if (!requested || (requested&0xFFFE)>=0xFFEE) {combatNearHeap.nearHeapSearchHeader=combatNearHeap.nearHeapFirstHeader;return 0;}
    requested&=0xFFFE;
    uint16_t position=combatNearHeap.nearHeapSearchHeader,payload=position+2;
    uint16_t header=NearHeapHeader(position);
    position+=2;
    bool restarted=false;
    for (;;)
    {
        if (header&1)
        {
            uint16_t available=header-1;
            if (available>=requested)
            {
                uint16_t next=position+requested;
                if (available==requested) SetNearHeapHeader((uint16_t)(position-2),(uint16_t)(header-1));
                else
                {
                    SetNearHeapHeader((uint16_t)(position-2),requested);
                    SetNearHeapHeader(next,(uint16_t)(available-requested-1));
                }
                combatNearHeap.nearHeapSearchHeader=next;return position;
            }
            uint16_t following=position+available;
            uint16_t nextHeader=NearHeapHeader(following);
            if (nextHeader&1)
            {
                header=(uint16_t)(nextHeader+available+2);
                SetNearHeapHeader((uint16_t)(payload-2),header);
                position=payload;continue;
            }
            position=following+2;header=nextHeader;
        }
        if (header!=0xFFFE)
        {
            position+=header;
            header=NearHeapHeader(position);position+=2;payload=position;
            continue;
        }
        if (!restarted)
        {
            restarted=true;position=combatNearHeap.nearHeapFirstHeader;
            header=NearHeapHeader(position);position+=2;payload=position;continue;
        }
        position=combatNearHeap.nearHeapEnd;
        uint16_t oldEnd;
        if (!ExtendNearHeap(0,&position,&payload,&oldEnd)) {combatNearHeap.nearHeapSearchHeader=combatNearHeap.nearHeapFirstHeader;return 0;}
        if (oldEnd!=position)
        {
            uint16_t alignmentBytes=(oldEnd&1)+2;
            if (!ExtendNearHeap(alignmentBytes,&position,&payload,&oldEnd)) {combatNearHeap.nearHeapSearchHeader=combatNearHeap.nearHeapFirstHeader;return 0;}
            uint16_t at=(uint16_t)(payload-2),value=NearHeapHeader(at);
            SetNearHeapHeader(at,(value&0xFF00)|(uint8_t)(value-1));
        }
        if (!GrowNearHeap(requested,&position,&payload,&oldEnd)) {combatNearHeap.nearHeapSearchHeader=combatNearHeap.nearHeapFirstHeader;return 0;}
        position=oldEnd-2;header=NearHeapHeader(position);position+=2;payload=position;
    }
}
static void ReleaseNearHeapOffset(uint16_t offset)
{
    if (offset) SetNearHeapHeader((uint16_t)(offset-2),NearHeapHeader((uint16_t)(offset-2))|1);
}

/* CDBC/CE5D's request: `nativeBytes` decide the heap's placement and exhaustion; the host object is `hostBytes`. Its
 * bytes are the caller's to write: a taken-over object keeps its own, as the original's memory did, and a new one is
 * zero. `owner` names what the caller keeps there and is part of a takeover's match. */
void *BtechAllocateNearBytes(uint16_t nativeBytes,size_t hostBytes,BtechNearOwner owner)
{
    uint16_t offset=AllocateNearHeapBlock(nativeBytes);
    if (!offset) return nullptr;
    for (NearBlock &block:blocks)
        if (block.released && block.offset==offset && block.bytes==nativeBytes && block.hostBytes==hostBytes && block.owner==owner)
        {
            block.released=false;
            return block.object.get();
        }
    blocks.push_back(NearBlock{std::make_unique<std::byte[]>(hostBytes?hostBytes:1),hostBytes,offset,nativeBytes,owner,false});
    return blocks.back().object.get();
}
void *BtechAllocateZeroedNearElements(uint16_t count,uint16_t stride,size_t hostBytes,BtechNearOwner owner)
{
    uint32_t bytes=(uint32_t)count*stride;
    if (bytes>UINT16_MAX) return nullptr; /* E9C6 checks the multiply's high WORD. */
    void *object=BtechAllocateNearBytes((uint16_t)bytes,hostBytes,owner);
    if (object) std::memset(object,0,hostBytes);
    return object;
}
/* E96E/E978: OR BYTE [BX-2],1, the block's header marked free (again, for a block released twice). A header that a
 * live block's payload now covers would be a write into that payload: never seen (step 0), so it stops. */
void BtechReleaseNearHeapBlock(void *object)
{
    if (!object) return;
    for (NearBlock &block:blocks)
    {
        if (block.object.get()!=object) continue;
        uint16_t address=(uint16_t)(block.offset-2);
        for (const NearBlock &other:blocks)
            if (!other.released && &other!=&block && other.offset<=address && (uint32_t)address<(uint32_t)other.offset+other.bytes)
                /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("A released block's header inside a live block's payload.");
        ReleaseNearHeapOffset(block.offset);
        block.released=true;
        return;
    }
    /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Released object has no near-heap allocation.");
}
/* CDD9: the release, then a request for FFFF bytes, which fails and resets the search cursor (no coalescing). */
void BtechReleaseNearBlockAndProbeAllocation(void *object)
{BtechReleaseNearHeapBlock(object);(void)AllocateNearHeapBlock(UINT16_MAX);}
