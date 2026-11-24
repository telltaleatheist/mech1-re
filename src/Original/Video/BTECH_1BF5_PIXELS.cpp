#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_video.h"
#include <stdlib.h>

/*1BF5:2159. Native signed, inclusive clip followed by FAR tail. Colour is
 * the caller's complete WORD; the selected writer decides which bits matter. */
void BtechWriteClippedPixel(int16_t x,int16_t y,uint16_t colour)
{
    if (combatVideo.screenPolygonNeedsClip && (x<combatVideo.clipMinimumX || x>combatVideo.clipMaximumX || y<combatVideo.clipMinimumY || y>combatVideo.clipMaximumY)) return;
    if (!combatVideo.graphicsWritePixel) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !graphicsWritePixel");
    combatVideo.graphicsWritePixel((uint16_t)x,(uint16_t)y,colour);
}

/*1BF5:2127. Reject without touching video or the backend. */
uint16_t BtechReadClippedPixel(int16_t x,int16_t y)
{
    if (combatVideo.screenPolygonNeedsClip && (x<combatVideo.clipMinimumX || x>combatVideo.clipMaximumX || y<combatVideo.clipMinimumY || y>combatVideo.clipMaximumY)) return UINT16_MAX;
    if (!combatVideo.graphicsReadPixel) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !graphicsReadPixel");
    return combatVideo.graphicsReadPixel((uint16_t)x,(uint16_t)y);
}

/*1BF5:211F. Genuine dispatch tail, no new presentation policy. */
void BtechSetOverscanColour(uint16_t colour)
{
    if (!combatVideo.graphicsSetOverscanColour) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !graphicsSetOverscanColour");
    combatVideo.graphicsSetOverscanColour(colour);
}
