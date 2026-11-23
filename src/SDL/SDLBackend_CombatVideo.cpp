#include "SDLBackend.h"
#include "mechwarrior/ega_campaign.h"
#include "mechwarrior/combat_video.h"
#include <string.h>

/* Combat draws on the canvas (src/Video/Canvas, docs/phase5/NO_EMULATION_PLAN.md
 * G4): no EGA graphics controller, sequencer, latches or planes are modelled.
 * What is left of the video ports: the CRTC index and data registers, which
 * combat's adapter check writes and reads back (1BF5), and the status port's
 * retrace bit, which its page flip waits on. Both go with G5. */
bool mwSdlCombatActive;
static uint8_t crtcIndex,crtcRegisters[25];
enum { EGA_CRTC_INDEX=0x3D4,EGA_CRTC_DATA=0x3D5,EGA_SEQUENCE_INDEX=0x3C4,EGA_SEQUENCE_DATA=0x3C5,
    EGA_GRAPHICS_INDEX=0x3CE,EGA_GRAPHICS_DATA=0x3CF,EGA_STATUS=0x3DA };

void MwSdlResetCombatVideoHardware(void)
{
    memset(crtcRegisters,0,sizeof(crtcRegisters));crtcIndex=0;
}
bool MwSdlWriteCombatVideoPort(uint16_t port,uint8_t value)
{
    switch (port)
    {
    case EGA_GRAPHICS_INDEX:case EGA_GRAPHICS_DATA:case EGA_SEQUENCE_INDEX:case EGA_SEQUENCE_DATA:
        MwSdlFail("EGA graphics or sequencer register written: combat draws on the canvas");
    case EGA_CRTC_INDEX:crtcIndex=value;return true;
    case EGA_CRTC_DATA:
        if (crtcIndex>=sizeof(crtcRegisters)) MwSdlFail("Unmapped EGA CRTC register");
        crtcRegisters[crtcIndex]=value;return true;
    default:return false;
    }
}
bool MwSdlReadCombatVideoPort(uint16_t port,uint8_t *value)
{
    switch (port)
    {
    case EGA_GRAPHICS_DATA:case EGA_SEQUENCE_DATA:
        MwSdlFail("EGA graphics or sequencer register read: combat draws on the canvas");
    case EGA_CRTC_DATA:
        if (crtcIndex>=sizeof(crtcRegisters)) MwSdlFail("Read unmapped EGA CRTC register");
        *value=crtcRegisters[crtcIndex];return true;
    case EGA_STATUS:
        /* Independent 60Hz scanout. Reading status is not a simulation tick. */
        if (MwSdlVirtualClock()) MwSdlSpendNanoseconds(1000); /* One ISA read, so retrace waits end. */
        *value=MwSdlNowNanoseconds()%UINT64_C(16666667)>=UINT64_C(15000000)?COMBAT_EGA_VERTICAL_RETRACE:0;
        return true;
    default:return false;
    }
}
