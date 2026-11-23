#ifndef MECHWARRIOR_KEY_EVENTS_H
#define MECHWARRIOR_KEY_EVENTS_H
#include <stdbool.h>
#include <stdint.h>

/* Keys as the games receive them (docs/phase5/NO_EMULATION_PLAN.md, H2): typed
 * events from the host's key events, with no BIOS keyboard, ring or IRQ1.
 *
 * A key's number is the PC keyboard's scan code (set 1), which both games' key
 * tables and key constants are written in. */
typedef struct MwKeyEvent
{
    uint8_t key;
    bool released;
} MwKeyEvent;
/* Combat takes every press and release, as its keyboard handler did. */
static inline uint8_t MwKeyEventScanByte(MwKeyEvent event)
{ return (uint8_t)(event.key|(event.released?0x80:0)); } /* The make/break byte the handler's tables use. */

/* The campaign takes presses: the key, the character it types (0 for none)
 * and whether Control was held. */
typedef struct MwCampaignKey
{
    uint8_t key;
    uint8_t character;
    bool control;
} MwCampaignKey;
/* The campaign's key word, the key above its character, as MW_MAIN compares
 * keys (CAMPAIGN_KEY_*). */
static inline uint16_t MwCampaignKeyWord(MwCampaignKey key)
{ return (uint16_t)(((uint16_t)key.key<<8)|key.character); }
#endif
