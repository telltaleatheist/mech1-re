#include "mechwarrior/port_diagnostics.h"
#ifndef MECHWARRIOR_NATIVE_ADDRESS_H
#define MECHWARRIOR_NATIVE_ADDRESS_H

#include <stdint.h>
#include <stdlib.h>

enum { NATIVE_SEGMENT_BYTE_COUNT = 0x10000, NATIVE_WORD_SIGN_BIT = 0x8000 };

/* Byte-order/wrap adapters, not additional game procedures. */
static inline uint16_t MwReadNativeWord(const uint8_t *bytes, uint16_t offset)
{
    return (uint16_t)(bytes[offset] | ((uint16_t)bytes[(uint16_t)(offset + 1)] << 8));
}
#endif
