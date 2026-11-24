#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_video.h"
#include "BTECH_2A36_CANVAS.h"
#include <stdlib.h>

/*2A36:228B. BX byte offset and CL bit shift. Output pointer only transports
 * that register; it is not a third native stack argument. */
uint16_t BtechEgaPixelByteOffset(uint16_t x,uint16_t y,uint8_t *shift)
{
    *shift=(uint8_t)(7^(x&7));
    return (uint16_t)((x>>3)+(uint16_t)(y*COMBAT_SCREEN_ROW_BYTES));
}

/* The pixel a byte offset and bit shift name, on the canvas (G2). */
static void CanvasPixelPosition(uint16_t offset,uint8_t shift,int *x,int *y)
{
    BtechEgaCanvasPosition(offset,x,y);
    *x+=7-shift;
}

/*2A36:22F1. One pixel, by copy, on the canvas (G2, G4). The original sets its
 * bit mask, write mode 2 and copy, and puts them back afterwards; on the canvas
 * there is nothing to set. */
void BtechEgaWritePixel(uint16_t x,uint16_t y,uint16_t colour)
{
    uint16_t page=combatVideo.drawPageSelector;uint8_t shift;
    uint16_t offset=BtechEgaPixelByteOffset(x,y,&shift);
    int canvasX,canvasY;CanvasPixelPosition(offset,shift,&canvasX,&canvasY);
    mw1o::video::GameCanvas().pixel(BtechEgaCanvasPage(page),canvasX,canvasY,(uint8_t)colour);
}

/*2A36:22A8. The pixel's colour, from the canvas (G2, G4). The original reads it
 * one plane at a time. */
uint16_t BtechEgaReadPixel(uint16_t x,uint16_t y)
{
    uint16_t page=combatVideo.drawPageSelector;uint8_t shift;
    uint16_t offset=BtechEgaPixelByteOffset(x,y,&shift);
    int canvasX,canvasY;CanvasPixelPosition(offset,shift,&canvasX,&canvasY);
    return mw1o::video::GameCanvas().pixelAt(BtechEgaCanvasPage(page),canvasX,canvasY);
}

/*2A36:233D. Original BIOS1001 colour mapping; the border, not a game UI. The
 * overscan register write is the canvas's border entry (G5). */
void BtechEgaSetOverscanColour(uint16_t colour)
{
    uint8_t hardwareColour=(uint8_t)colour&0x0F;
    if (combatVideo.combatRendererKind==COMBAT_VIDEO_EGA && hardwareColour>=8) hardwareColour+=8;
    mw1o::video::GameCanvas().setPaletteEntry(mw1o::video::Canvas::BORDER,mw1o::video::PaletteCodeColour(hardwareColour));
}
