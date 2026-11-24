#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_video.h"
#include "BTECH_2A36_CANVAS.h"
#include <stdlib.h>

/* Drawn on the canvas through BtechEgaCanvasWriter (BTECH_2A36_CANVAS.h): the
 * colour where the mask has a 1 (G3, G4). */

/*2A36:1E32. The leading and trailing partial bytes and the whole bytes between
 * (REP STOSB in the original). Forward game path; no added clip. */
void BtechEgaFillSolidSpan(uint16_t pattern,uint16_t x,int16_t count,uint16_t rowOffset,CombatPage page)
{
    if (count<=0) return;
    BtechEgaCanvasWriter ega(page);
    uint16_t cursor=rowOffset+(x>>3);
    uint16_t firstPixel=x&7,endPixel=(uint16_t)(firstPixel+(uint16_t)count);
    uint8_t mask=combatVideo.egaSpanLeadingMasks[firstPixel];
    if ((int16_t)endPixel<8)
    {
        mask&=combatVideo.egaSpanTrailingMasks[(uint8_t)endPixel];
        ega.setMask((uint8_t)(mask));
        ega.write(cursor,(uint8_t)pattern);
        return;
    }
    uint16_t remaining=(uint16_t)count-(8-firstPixel);
    ega.setMask((uint8_t)(mask));
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
        mask=combatVideo.egaSpanTrailingMasks[tail];
        ega.setMask((uint8_t)(mask));
        ega.write(cursor,(uint8_t)pattern);
    }
}

/*2A36:1D40. Two original raster phases kept inline. The original's second phase
 * reloads the latches only at its first whole byte; by copy (the only function
 * combat draws with), that leaves the same pixels as drawing each byte where
 * its mask has a 1 (G4; BTECH_2A36_CANVAS.h). */
void BtechEgaFillAlternatingSpan(uint16_t primary,uint16_t x,int16_t count,uint16_t alternate,uint16_t rowOffset,CombatPage page)
{
    combatVideo.spanAlternationState^=1;
    uint16_t pattern=combatVideo.spanAlternationState?alternate:primary;
    if (count<=0) return;
    BtechEgaCanvasWriter ega(page);
    combatVideo.egaSpanPrimaryColour=(uint8_t)(pattern>>8);
    combatVideo.egaSpanSecondaryColour=(uint8_t)pattern;
    uint16_t firstPixel=x&7,endPixel=(uint16_t)(firstPixel+(uint16_t)count);
    uint8_t leading=combatVideo.egaSpanLeadingMasks[firstPixel];
    uint16_t firstByte=rowOffset+(x>>3);
    for (uint16_t phase=0;phase<2;++phase)
    {
        uint8_t columns=phase?0x55:0xAA;
        uint8_t colour=phase?combatVideo.egaSpanSecondaryColour:combatVideo.egaSpanPrimaryColour;
        uint16_t cursor=firstByte;
        if ((int16_t)endPixel<8)
        {
            uint8_t mask=leading&combatVideo.egaSpanTrailingMasks[(uint8_t)endPixel]&columns;
            ega.setMask((uint8_t)(mask));
            ega.write(cursor,colour);
            continue;
        }
        uint16_t remaining=(uint16_t)count-(8-firstPixel);
        ega.setMask((uint8_t)((leading&columns)));
        ega.write(cursor++,colour);
        uint16_t wholeBytes=(remaining&0xFFF8)>>3;
        if (wholeBytes)
        {
            ega.setMask((uint8_t)(columns));
            do {ega.write(cursor++,colour);} while (--wholeBytes);
        }
        uint16_t tail=remaining&7;
        if (tail)
        {
            uint8_t mask=combatVideo.egaSpanTrailingMasks[tail]&columns;
            ega.setMask((uint8_t)(mask));
            ega.write(cursor,colour);
        }
    }
}
