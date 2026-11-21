/* BTECH.EXE's load image: the known revisions and the EXEPACK decoder, moved
 * here from CombatInstallation.cpp so the story importer and the import
 * table's generator (tools/import/btech_image.cpp) unpack it the same way the
 * game does. Needs nothing of the game but the layout constants. */
#include "mechwarrior/combat_installation.h"
#include "mechwarrior/native_address.h"
#include <stdlib.h>
#include <string.h>

/* Known BTECH.EXE revisions. The reviewed revision is the port's reference
 * layout. The 1990-01-22 build (shipped on the 1994 BattleTech compilation CD)
 * patches code in segments 0810/17FB: 0810 grows by 0x12 paragraphs and 17FB
 * shrinks by one, so every later segment (1ADE..304E, including DGROUP) sits
 * 0x11 paragraphs higher. DGROUP size, DGROUP far pointers and all tail
 * segments are otherwise unchanged, so that build is rebased into the reference
 * layout: the head is truncated, the tail is moved down by 0x110 bytes and
 * relocated segment words are remapped. Patched 0810/17FB code bytes are not
 * reference-equivalent; the port reads none of them as data. */
typedef struct BtechInstallationRevision
{
    BtechRevision name;size_t packedBytes;uint32_t crc;size_t imageBytes;
    uint16_t headShrinkStart,headShift,tailStart,tailShift; /* paragraphs, image-relative */
    const uint16_t (*dgroupFixups)[3];size_t dgroupFixupCount; /* {offset, this revision, reference} */
} BtechInstallationRevision;
/* 1990 build: 0x44 bytes of new BSS precede every reference BSS variable and
 * 0810 runtime code near its end moved by 0x126. Only initialized DGROUP words
 * that are real near pointers are listed, each with its verified 1990 value;
 * blind value matching would corrupt text and numeric tables. */
static const uint16_t btech1990DgroupFixups[][3]={
    {0x244E,0x546E,0x542A},{0x2450,0x5470,0x542C},{0x2452,0x5470,0x542C},{0x2454,0x546E,0x542A}, /* avoidance probes */
    {0x2456,0x5470,0x542C},{0x2458,0x546E,0x542A},{0x245A,0x546E,0x542A},{0x245C,0x5470,0x542C},
    {0x4DFA,0xFFA6,0xFE80}, /* runtime exit flush callback */
};
static const BtechInstallationRevision btechInstallationRevisions[]={
    {BTECH_REVISION_REFERENCE,BTECH_PACKED_BYTES,0x36A13E1Au,BTECH_IMAGE_BYTES,0,0,0,0,NULL,0},
    {BTECH_REVISION_1990_01_22,0x2A9C5,0x7E06BB28u,0x2D3D0,0x0FFD,0x12,0x12DF,0x11,
        btech1990DgroupFixups,sizeof(btech1990DgroupFixups)/sizeof(btech1990DgroupFixups[0])},
};
BtechRevision btechLoadedRevision=BTECH_REVISION_REFERENCE;
static uint32_t InstallationCrc(const uint8_t *bytes,size_t count)
{
    uint32_t crc=UINT32_MAX;
    for (size_t byte=0;byte<count;++byte)
    {
        crc^=bytes[byte];
        for (uint16_t bit=0;bit<8;++bit) crc=(crc>>1)^((crc&1)?0xEDB88320u:0);
    }
    return ~crc;
}
static uint16_t RebaseSegment(const BtechInstallationRevision *revision,uint16_t segment)
{
    if (!revision->tailShift) return segment;
    if (segment>=revision->tailStart) return (uint16_t)(segment-revision->tailShift);
    if (segment>=revision->headShrinkStart) return (uint16_t)(segment-revision->headShift);
    return segment;
}
size_t BtechInstallationPackedBytes(size_t fileBytes)
{
    for (size_t i=0;i<sizeof(btechInstallationRevisions)/sizeof(btechInstallationRevisions[0]);++i)
        if (btechInstallationRevisions[i].packedBytes==fileBytes) return fileBytes;
    return 0;
}

/* Actual installed BTECH.EXE's backward EXEPACK B0/B2 interpreter and sixteen
 * relocation groups. Corresponds to Inspect-NativeVideoImage.py; no dependency
 * on Reko exports. CRC pins known revisions, not cryptographic trust. */
static bool DecodeInto(const BtechInstallationRevision *revision,const uint8_t *packed,size_t packedBytes,uint8_t *decoded)
{
    const size_t imageBytes=revision->imageBytes;
    size_t start=(size_t)MwReadNativeWord(packed,8)*0x10;
    size_t stub=start+(size_t)MwReadNativeWord(packed,0x16)*0x10;
    if (stub+0x12D>packedBytes || MwReadNativeWord(packed,0x14)!=0x12
        || MwReadNativeWord(packed+stub,0x10)!=0x4252 || MwReadNativeWord(packed+stub,0x0E)!=1
        || (uint32_t)MwReadNativeWord(packed+stub,0x0C)*0x10!=imageBytes) return false;
    size_t stubEnd=stub+MwReadNativeWord(packed+stub,6);
    if (stubEnd>packedBytes) return false;
    size_t source=stub,destination=imageBytes;
    while (source>start && packed[source-1]==0xFF) --source;
    bool last=false;
    while (!last)
    {
        if (source-start<3) return false;
        uint8_t command=packed[--source];
        source-=2;uint16_t count=MwReadNativeWord(packed+source,0);
        if (count>destination) return false;
        destination-=count;
        if ((command&0xFE)==0xB0)
        {
            if (source==start) return false;
            memset(decoded+destination,packed[--source],count);
        }
        else if ((command&0xFE)==0xB2)
        {
            if (count>source-start) return false;
            source-=count;memcpy(decoded+destination,packed+source,count);
        }
        else return false;
        last=(command&1)!=0;
    }
    if (destination) return false;
    size_t cursor=stub+0x12D;
    for (uint16_t group=0;group<0x10;++group)
    {
        if (cursor+2>stubEnd) return false;
        uint16_t count=MwReadNativeWord(packed+cursor,0);cursor+=2;
        while (count--)
        {
            if (cursor+2>stubEnd) return false;
            size_t at=(size_t)group*0x10000+MwReadNativeWord(packed+cursor,0);cursor+=2;
            if (at+2>imageBytes) return false;
            uint16_t selector=(uint16_t)(RebaseSegment(revision,MwReadNativeWord(decoded+at,0))+BTECH_IMAGE_LOAD_SELECTOR);
            decoded[at]=(uint8_t)selector;decoded[at+1]=(uint8_t)(selector>>8);
        }
    }
    if (revision->tailShift)
    {
        size_t tail=(size_t)revision->tailStart*0x10,shift=(size_t)revision->tailShift*0x10;
        if (imageBytes-shift!=BTECH_IMAGE_BYTES || tail>imageBytes) return false;
        memmove(decoded+tail-shift,decoded+tail,imageBytes-tail);
    }
    for (size_t i=0;i<revision->dgroupFixupCount;++i)
    {
        uint8_t *dgroup=decoded+BTECH_DATA_IMAGE_OFFSET;
        uint16_t at=revision->dgroupFixups[i][0];
        if (at+2>BTECH_BSS_START || MwReadNativeWord(dgroup,at)!=revision->dgroupFixups[i][1]) return false;
        dgroup[at]=(uint8_t)revision->dgroupFixups[i][2];dgroup[at+1]=(uint8_t)(revision->dgroupFixups[i][2]>>8);
    }
    return true;
}
bool BtechDecodeImage(const uint8_t *packed,size_t packedBytes,uint8_t **image,BtechRevision *revisionName)
{
    *image=NULL;
    if (!packed) return false;
    const BtechInstallationRevision *revision=NULL;
    for (size_t i=0;i<sizeof(btechInstallationRevisions)/sizeof(btechInstallationRevisions[0]);++i)
        if (btechInstallationRevisions[i].packedBytes==packedBytes) {revision=&btechInstallationRevisions[i];break;}
    if (!revision) return false;
    if (InstallationCrc(packed,packedBytes)!=revision->crc || MwReadNativeWord(packed,0)!=0x5A4D || MwReadNativeWord(packed,6)) return false;
    uint8_t *decoded=static_cast<uint8_t *>(malloc(revision->imageBytes));
    if (!decoded) return false;
    if (!DecodeInto(revision,packed,packedBytes,decoded)) {free(decoded);return false;}
    *image=decoded;
    if (revisionName) *revisionName=revision->name;
    return true;
}
bool BtechDecodeInstallationImage(const uint8_t *packed,size_t packedBytes,uint8_t **image)
{
    return BtechDecodeImage(packed,packedBytes,image,&btechLoadedRevision);
}
