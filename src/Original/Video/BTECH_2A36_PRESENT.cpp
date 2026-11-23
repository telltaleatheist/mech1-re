#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_startup.h"
#include "BTECH_2A36_CANVAS.h"
#include <stdlib.h>

/*2A36:1D00. Exchange display/draw buffers and present the new front page
 * BEFORE the optional retrace wait (the original sets the CRTC start there; G4).
 * The separate copy-source and drawing selectors stay stale. */
void BtechEgaSwapScreenPages(uint16_t waitForRetrace)
{
    if (waitForRetrace && !btechHostReadPort) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: waitForRetrace && !btechHostReadPort");
    uint16_t previousFrontPage=frontPageSelector;
    frontPageSelector=backPageSelector;
    backPageSelector=previousFrontPage;
    mw1o::video::GameCanvas().present(BtechEgaCanvasPage(frontPageSelector));
    if (waitForRetrace)
    {
        while (btechHostReadPort(COMBAT_EGA_STATUS_PORT)&COMBAT_EGA_VERTICAL_RETRACE) {}
        while (!(btechHostReadPort(COMBAT_EGA_STATUS_PORT)&COMBAT_EGA_VERTICAL_RETRACE)) {}
    }
}
