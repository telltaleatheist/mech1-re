#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_video.h"
#include "BTECH_2A36_CANVAS.h"
#include <stdlib.h>

/*2A36:1BB0. Four plane-major REP MOVSW passes, high plane first. The RAM
 * cursor wraps as a WORD, row start advances40 regardless of row width.
 * Converted callers supply paragraph-aligned storage and clear DF. The buffer
 * keeps the planar form, one colour bit per pass (BTECH_2A36_CANVAS.h). */
void BtechEgaCaptureScreenRectangle(BtechResourceAllocation buffer,uint16_t x,uint16_t y,uint16_t width,uint16_t height)
{
    uint16_t leftByte=x>>3;
    uint16_t coveredBytes=((uint16_t)(x+width)>>3)-leftByte+1;
    uint16_t wordsPerRow=(coveredBytes>>1)+(coveredBytes&1);
    if (y>=COMBAT_SCREEN_HEIGHT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: y>=COMBAT_SCREEN_HEIGHT"); /* Adjacent row-owner aliases not represented. */
    uint16_t screenStart=combatVideo.graphicsRowOffsets[y]+leftByte;
    uint16_t page=combatVideo.copySourcePageSelector,bufferCursor=0;
    for (int16_t plane=COMBAT_EGA_PLANES-1;plane>=0;--plane)
    {
        uint16_t rowsRemaining=height,screenCursor=screenStart;
        do
        {
            uint16_t rowStart=screenCursor;
            for (uint16_t wordsRemaining=wordsPerRow;wordsRemaining;--wordsRemaining)
            {
                if (!buffer.bytes || (uint32_t)bufferCursor+2>buffer.byteCount) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !buffer.bytes || (uint32_t)bufferCursor+2>buffer.byteCount");
                /* A native WORD startingFFFF crosses the segment physically;
                 * only the later SI increment wraps. It is not two LODSBs. */
                uint8_t low=BtechEgaCanvasColourBits(BtechEgaCanvasPage(page),screenCursor,plane);
                uint8_t high=BtechEgaCanvasColourBits(BtechEgaCanvasPage((uint16_t)(page+(screenCursor==UINT16_MAX?0x1000:0))),(uint16_t)(screenCursor+1),plane);
                buffer.bytes[bufferCursor]=low;buffer.bytes[bufferCursor+1]=high;
                bufferCursor+=2;screenCursor+=2;
            }
            screenCursor=rowStart+COMBAT_SCREEN_ROW_BYTES;
        } while (--rowsRemaining);
    }
}

/*2A36:1C69. WORD-aligned RAM cursor; each pass writes one colour bit back by
 * copy (the only function combat draws with; set/reset is never enabled). No
 * clipping, pixel-mask helper or repaint. */
void BtechEgaRestoreScreenRectangle(BtechResourceAllocation buffer,uint16_t x,uint16_t y,uint16_t width,uint16_t height)
{
    uint16_t leftByte=x>>3;
    uint16_t coveredBytes=((uint16_t)(x+width)>>3)-leftByte+1;
    uint16_t wordsPerRow=(coveredBytes>>1)+(coveredBytes&1);
    if (y>=COMBAT_SCREEN_HEIGHT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: y>=COMBAT_SCREEN_HEIGHT");
    uint16_t screenStart=combatVideo.graphicsRowOffsets[y]+leftByte;
    uint16_t page=combatVideo.drawPageSelector,bufferCursor=0;
    for (int16_t plane=COMBAT_EGA_PLANES-1;plane>=0;--plane)
    {
        uint16_t rowsRemaining=height,screenCursor=screenStart;
        do
        {
            uint16_t rowStart=screenCursor;
            for (uint16_t wordsRemaining=wordsPerRow;wordsRemaining;--wordsRemaining)
            {
                if (!buffer.bytes || (uint32_t)bufferCursor+2>buffer.byteCount) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !buffer.bytes || (uint32_t)bufferCursor+2>buffer.byteCount");
                uint8_t low=buffer.bytes[bufferCursor],high=buffer.bytes[bufferCursor+1];
                BtechEgaCanvasSetColourBits(BtechEgaCanvasPage(page),screenCursor,plane,low);
                BtechEgaCanvasSetColourBits(BtechEgaCanvasPage((uint16_t)(page+(screenCursor==UINT16_MAX?0x1000:0))),(uint16_t)(screenCursor+1),plane,high);
                bufferCursor+=2;screenCursor+=2;
            }
            screenCursor=rowStart+COMBAT_SCREEN_ROW_BYTES;
        } while (--rowsRemaining);
    }
}
