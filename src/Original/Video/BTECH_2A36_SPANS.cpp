#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_video.h"
#include "BTECH_2A36_CANVAS.h"
#include <stdlib.h>

/* G3: drawn on the canvas through BtechEgaCanvasWriter (BTECH_2A36_CANVAS.h),
 * which keeps the bit mask writes and latch reads for the register model. */

/*2A36:1E32. Native mode2 writes, including inherited controller state and
 * latch reuse by REP STOSB. Forward game path; no added clip or mask restore. */
void BtechEgaFillSolidSpan(uint16_t pattern,uint16_t x,int16_t count,uint16_t rowOffset,uint16_t page)
{
    if (count<=0) return;
    if (!btechHostWritePortWord || !btechHostReadVideoByte) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostWritePortWord || !btechHostReadVideoByte");
    BtechEgaCanvasWriter ega(page);
    uint16_t cursor=rowOffset+(x>>3);
    uint16_t firstPixel=x&7,endPixel=(uint16_t)(firstPixel+(uint16_t)count);
    uint8_t mask=egaSpanLeadingMasks[firstPixel];
    if ((int16_t)endPixel<8)
    {
        mask&=egaSpanTrailingMasks[(uint8_t)endPixel];
        ega.setMask((uint8_t)(mask));
        ega.load(cursor);
        ega.write(cursor,(uint8_t)pattern);
        return;
    }
    uint16_t remaining=(uint16_t)count-(8-firstPixel);
    ega.setMask((uint8_t)(mask));
    ega.load(cursor);
    ega.write(cursor++,(uint8_t)pattern);
    uint16_t wholeBytes=(remaining&0xFFF8)>>3;
    if (wholeBytes)
    {
        ega.setMask(0xFF);
        do {ega.write(cursor++,(uint8_t)pattern);} while (--wholeBytes);
    }
    uint16_t tail=remaining&7;
    if (tail)
    {
        mask=egaSpanTrailingMasks[tail];
        ega.setMask((uint8_t)(mask));
        ega.load(cursor);
        ega.write(cursor,(uint8_t)pattern);
    }
}

/*2A36:1D40. Two original raster phases kept inline: the second refreshes
 * only its FIRST bulk latch, not every destination. That quirk is observable. */
void BtechEgaFillAlternatingSpan(uint16_t primary,uint16_t x,int16_t count,uint16_t alternate,uint16_t rowOffset,uint16_t page)
{
    spanAlternationState^=1;
    uint16_t pattern=spanAlternationState?alternate:primary;
    if (count<=0) return;
    if (!btechHostWritePortWord || !btechHostReadVideoByte) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostWritePortWord || !btechHostReadVideoByte");
    BtechEgaCanvasWriter ega(page);
    egaSpanPrimaryColour=(uint8_t)(pattern>>8);
    egaSpanSecondaryColour=(uint8_t)pattern;
    uint16_t firstPixel=x&7,endPixel=(uint16_t)(firstPixel+(uint16_t)count);
    uint8_t leading=egaSpanLeadingMasks[firstPixel];
    uint16_t firstByte=rowOffset+(x>>3);
    for (uint16_t phase=0;phase<2;++phase)
    {
        uint8_t columns=phase?0x55:0xAA;
        uint8_t colour=phase?egaSpanSecondaryColour:egaSpanPrimaryColour;
        uint16_t cursor=firstByte;
        if ((int16_t)endPixel<8)
        {
            uint8_t mask=leading&egaSpanTrailingMasks[(uint8_t)endPixel]&columns;
            ega.setMask((uint8_t)(mask));
            ega.load(cursor);
            ega.write(cursor,colour);
            continue;
        }
        uint16_t remaining=(uint16_t)count-(8-firstPixel);
        ega.setMask((uint8_t)((leading&columns)));
        ega.load(cursor);
        ega.write(cursor++,colour);
        uint16_t wholeBytes=(remaining&0xFFF8)>>3;
        if (wholeBytes)
        {
            ega.setMask((uint8_t)(columns));
            if (phase) ega.load(cursor);
            do {ega.write(cursor++,colour);} while (--wholeBytes);
        }
        uint16_t tail=remaining&7;
        if (tail)
        {
            uint8_t mask=egaSpanTrailingMasks[tail]&columns;
            ega.setMask((uint8_t)(mask));
            ega.load(cursor);
            ega.write(cursor,colour);
        }
    }
}
