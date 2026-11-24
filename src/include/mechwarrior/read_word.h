#ifndef MECHWARRIOR_READ_WORD_H
#define MECHWARRIOR_READ_WORD_H
#include <stdint.h>

/* A little-endian word and double word of bytes the caller owns. */
inline uint16_t MwReadWord(const uint8_t *bytes) { return (uint16_t)(bytes[0] | ((uint16_t)bytes[1] << 8)); }
inline uint32_t MwReadDword(const uint8_t *bytes)
{
    return (uint32_t)MwReadWord(bytes) | ((uint32_t)MwReadWord(bytes + 2) << 16);
}
#endif
