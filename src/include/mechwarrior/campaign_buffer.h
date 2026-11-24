#ifndef MECHWARRIOR_CAMPAIGN_BUFFER_H
#define MECHWARRIOR_CAMPAIGN_BUFFER_H
#include "mechwarrior/port_diagnostics.h"
#include <stdint.h>

enum { CAMPAIGN_BUFFER_BYTE_COUNT = 0x10000 };

/* One of the campaign's resource buffers: a whole native offset window, so a
 * uint16_t index is always inside it and wraps FFFF->0000 as the original's. */
struct MwCampaignBuffer
{
    uint8_t bytes[CAMPAIGN_BUFFER_BYTE_COUNT];
};

/* A buffer's bytes. NULL (not allocated, or released when its program ended) is
 * an unsupported integration error, never a successful empty read. */
static inline uint8_t *MwCampaignBufferBytes(MwCampaignBuffer *buffer)
{
    if (!buffer) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("A campaign resource buffer that is not allocated");
    return buffer->bytes;
}
#endif
