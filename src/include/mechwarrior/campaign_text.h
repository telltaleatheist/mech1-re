#ifndef MECHWARRIOR_CAMPAIGN_TEXT_H
#define MECHWARRIOR_CAMPAIGN_TEXT_H
#include "mechwarrior/port_diagnostics.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Text to draw: from its first byte up to its storage's end. The drawing stops
 * at the NUL and returns a pointer to it; reading past the end is refused. */
struct MwText
{
    const uint8_t *cursor, *end;
};
template <size_t N> MwText MwArrayText(const char (&text)[N])
{
    const uint8_t *bytes=reinterpret_cast<const uint8_t *>(text);
    return MwText{bytes,bytes+N};
}
template <size_t N> MwText MwArrayText(const uint8_t (&text)[N]) { return MwText{text,text+N}; }
inline MwText MwTerminatedText(const char *text)
{
    const uint8_t *bytes=reinterpret_cast<const uint8_t *>(text);
    return MwText{bytes,bytes+strlen(text)+1};
}
inline uint8_t MwTextByte(const MwText &text)
{
    if (text.cursor>=text.end) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: text.cursor>=text.end");
    return *text.cursor;
}
#endif
