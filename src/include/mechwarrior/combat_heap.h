#ifndef MECHWARRIOR_COMBAT_HEAP_H
#define MECHWARRIOR_COMBAT_HEAP_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
enum {
    BTECH_NEAR_POINTER_BYTES=2,BTECH_NEAR_SOUND_NODE_BYTES=0x10,
    BTECH_NEAR_ARCHIVE_BYTES=0x21,BTECH_NEAR_ARCHIVE_STATE_BYTES=0x80,
    BTECH_NEAR_WORLD_OBJECT_BYTES=0x30,BTECH_NEAR_BITMAP_ENTRY_BYTES=0x0A,
    BTECH_NEAR_STREAM_BUFFER_BYTES=0x200,
    BTECH_NEAR_INITIAL_BREAK=0xE196,BTECH_NEAR_GROWTH_ALIGNMENT=0x2000
};

typedef enum BtechNearOwner {
    BTECH_NEAR_BYTES,BTECH_NEAR_ARCHIVE,BTECH_NEAR_SOUND_NODE,
    BTECH_NEAR_BITMAP_LIST,BTECH_NEAR_BITMAP_IMAGES,BTECH_NEAR_WORLD_OBJECT,
    BTECH_NEAR_STREAM_BUFFER,BTECH_NEAR_SHAPE_BANKS
} BtechNearOwner;
/* Combat state owned by combat_heap.h (plan, slice B3a). */
struct CombatNearHeapState
{
    /* Native304E CRT control words; definitions stay in the runtime memory owner. */
    uint16_t nearHeapFirstHeader, nearHeapSearchHeader, nearHeapEnd;
    uint16_t nearHeapProgramBreak, nearHeapGrowthAlignment;
};
extern CombatNearHeapState combatNearHeap;

/* The near heap (BTECH_0810_HEAP.cpp, B5): native lengths decide placement and exhaustion, as 0810's allocator; host
 * sizes and owner kinds describe the widened C records, never additional game arguments. */
void *BtechAllocateNearBytes(uint16_t nativeBytes,size_t hostBytes,BtechNearOwner owner);
void *BtechAllocateZeroedNearElements(uint16_t count,uint16_t stride,size_t hostBytes,BtechNearOwner owner);
void BtechReleaseNearHeapBlock(void *record);
void BtechReleaseNearBlockAndProbeAllocation(void *record);
/* Process/image lifetime, not a game cleanup call. */
void BtechHostDiscardNearHeap(void);
#endif
