#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_startup.h"
#include "BTECH_2A36_CANVAS.h"
#include <stdlib.h>

/*2A36:1D00. Exchange display/draw buffers and present the new front page,
 * where the original sets the CRTC start (G4). The separate copy-source and
 * drawing selectors stay stale. The original can then wait for the next
 * vertical retrace on the status port (3DAh); no caller asks it to, and the
 * fixed combat tick paces frames, so that wait is gone (G5). */
void BtechEgaSwapScreenPages(uint16_t waitForRetrace)
{
    (void)waitForRetrace;
    CombatPage previousFrontPage=combatVideo.frontPage;
    combatVideo.frontPage=combatVideo.backPage;
    combatVideo.backPage=previousFrontPage;
    mw1o::video::GameCanvas().present(BtechEgaCanvasPage(combatVideo.frontPage));
}
