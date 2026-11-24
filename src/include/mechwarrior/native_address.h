#include "mechwarrior/port_diagnostics.h"
#ifndef MECHWARRIOR_NATIVE_ADDRESS_H
#define MECHWARRIOR_NATIVE_ADDRESS_H

#include <stdint.h>
#include <stdlib.h>

enum { NATIVE_SEGMENT_BYTE_COUNT = 0x10000, NATIVE_WORD_SIGN_BIT = 0x8000 };

/* Zero-copy native BYTE window. Named text storage need not be padded into
 * a second64K data segment merely to preserve a wrapping near cursor. */
typedef struct MwNativeByteView
{
    const uint8_t *bytes;
    uint16_t nativeBase;
    uint32_t byteCount;
    /* Only a proved physical-memory boundary uses this instead of allocation
     * bytes. Ordinary file/string/record views remain direct borrowed bytes. */
    uint8_t (*readPhysicalByte)(uint16_t offset);
} MwNativeByteView;

static inline uint8_t MwReadNativeViewByte(const MwNativeByteView *view,uint16_t offset)
{
    if (view->readPhysicalByte) return view->readPhysicalByte(offset);
    uint16_t relative=offset-view->nativeBase;
    if (relative>=view->byteCount) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: relative>=view->byteCount");
    return view->bytes[relative];
}

/* Binary stream WORD, including a real FFFF->0000 cursor wrap. Not an
 * accessor for known C record fields; use their members directly. */
static inline uint16_t MwReadNativeViewWord(const MwNativeByteView *view,uint16_t offset)
{
    uint16_t low=MwReadNativeViewByte(view,offset);
    return low|((uint16_t)MwReadNativeViewByte(view,(uint16_t)(offset+1))<<8);
}

/* One of the campaign's resource buffers: a whole native offset window, so a
 * uint16_t index is always inside it and wraps FFFF->0000 as the original's. */
struct MwCampaignBuffer
{
    uint8_t bytes[NATIVE_SEGMENT_BYTE_COUNT];
};

/* A buffer's bytes. NULL (not allocated, or released when its program ended) is
 * an unsupported integration error, never a successful empty read. */
static inline uint8_t *MwCampaignBufferBytes(MwCampaignBuffer *buffer)
{
    if (!buffer) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("A campaign resource buffer that is not allocated");
    return buffer->bytes;
}

/* Byte-order/wrap adapters, not additional game procedures. */
static inline uint16_t MwReadNativeWord(const uint8_t *bytes, uint16_t offset)
{
    return (uint16_t)(bytes[offset] | ((uint16_t)bytes[(uint16_t)(offset + 1)] << 8));
}
#endif
