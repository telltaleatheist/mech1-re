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

/*2A36:22F1. One pixel, mode 2 with COPY, which the original sets itself; only
 * the map mask is inherited, checked to be every plane. The pixel is drawn on
 * the canvas (G2). The register writes and the latch read stay as they were, so
 * the paths still on the register model find the EGA as the original leaves it;
 * they go with that model in G4. */
void BtechEgaWritePixel(uint16_t x,uint16_t y,uint16_t colour)
{
    if (!btechHostWritePortWord || !btechHostReadVideoByte) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostWritePortWord || !btechHostReadVideoByte");
    uint16_t page=drawPageSelector;uint8_t shift;
    uint16_t offset=BtechEgaPixelByteOffset(x,y,&shift);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_BIT_MASK_REGISTER|((uint16_t)(1<<shift)<<8));
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_WRITE_MODE_TWO);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_DATA_ROTATE_COPY);
    (void)BtechEgaCanvasInheritedLogic(); /* Checks the map mask; the function is COPY, just set. */
    (void)btechHostReadVideoByte(page,offset);
    int canvasX,canvasY;CanvasPixelPosition(offset,shift,&canvasX,&canvasY);
    mw1o::video::GameCanvas().pixel(BtechEgaCanvasPage(page),canvasX,canvasY,(uint8_t)colour);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_ALL_PIXEL_BITS);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_WRITE_MODE_ZERO);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_DATA_ROTATE_COPY);
    btechHostWritePortWord(COMBAT_EGA_SEQUENCER_PORT,COMBAT_EGA_MAP_ALL_PLANES);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_WRITE_MODE_TWO);
}

/*2A36:22A8. The pixel's colour, from the canvas (G2). The original's reads, one
 * per plane from read map 3 down to 0, stay as they were, for the latches and the
 * read map the paths still on the register model find (until G4). */
uint16_t BtechEgaReadPixel(uint16_t x,uint16_t y)
{
    if (!btechHostWritePortWord || !btechHostReadVideoByte) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostWritePortWord || !btechHostReadVideoByte");
    uint16_t page=drawPageSelector;uint8_t shift;
    uint16_t offset=BtechEgaPixelByteOffset(x,y,&shift);
    for (int16_t plane=COMBAT_EGA_PLANES-1;plane>=0;--plane)
    {
        btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_READ_MAP_REGISTER|((uint16_t)plane<<8));
        (void)btechHostReadVideoByte(page,offset);
    }
    int canvasX,canvasY;CanvasPixelPosition(offset,shift,&canvasX,&canvasY);
    uint8_t colour=mw1o::video::GameCanvas().pixelAt(BtechEgaCanvasPage(page),canvasX,canvasY);
    btechHostWritePortWord(COMBAT_EGA_SEQUENCER_PORT,COMBAT_EGA_MAP_ALL_PLANES);
    btechHostWritePortWord(COMBAT_EGA_GRAPHICS_PORT,COMBAT_EGA_WRITE_MODE_TWO);
    return colour;
}

/*2A36:233D. Original BIOS1001 colour mapping; hardware border, not a game UI. */
void BtechEgaSetOverscanColour(uint16_t colour)
{
    uint8_t hardwareColour=(uint8_t)colour&0x0F;
    if (combatRendererKind==COMBAT_VIDEO_EGA && hardwareColour>=8) hardwareColour+=8;
    if (!btechHostSetOverscanColour) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostSetOverscanColour");
    btechHostSetOverscanColour(hardwareColour);
}
