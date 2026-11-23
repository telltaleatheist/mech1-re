#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_polygons.h"
#include "BTECH_2A36_CANVAS.h"
#include <stdlib.h>

/*2A36:0071. Native polygon-span writer, including first-attempt count0,
 * signed endpoint comparison, palette/parity and partial masks, drawn on the
 * canvas (G4). Two raster phases stay inline; they are not calls to invented
 * draw helpers. The original's second phase reloads the latches only at its
 * first whole byte; by copy, that leaves the same pixels as drawing each byte
 * where its mask has a 1 (BTECH_2A36_CANVAS.h). */
void BtechEgaFillPolygonSpans(const BtechPolygonSpan *header)
{
    uint16_t page=drawPageSelector,row=header->header.firstRow;
    BtechEgaCanvasWriter ega(page);
    egaSpanRowsRemaining=header->header.rowCount;
    const BtechPolygonSpan *span=header+1;
    uint8_t colour=drawLineColour;
    spanPrimaryPattern=palettePrimarySlots[colour/COMBAT_PALETTE_WORDS][colour%COMBAT_PALETTE_WORDS];
    uint16_t secondary=paletteSecondarySlots[colour/COMBAT_PALETTE_WORDS][colour%COMBAT_PALETTE_WORDS];
    bool alternating=secondary!=spanPrimaryPattern;
    if (alternating) spanSecondaryPattern=secondary;
    uint8_t parity=row&1;
    for (;;)
    {
        uint16_t left=span->endpoints.left,right=span->endpoints.right;++span;
        if ((int16_t)right>=(int16_t)left)
        {
            if (row>=COMBAT_SCREEN_HEIGHT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: row>=COMBAT_SCREEN_HEIGHT"); /* Unmapped adjacent row-table reads. */
            uint16_t width=right-left+1;
            uint16_t firstByte=graphicsRowOffsets[row]+(left>>3);
            uint16_t pattern=alternating && parity?spanSecondaryPattern:spanPrimaryPattern;
            if (alternating) {egaSpanHighColour=(uint8_t)(pattern>>8);egaSpanLowColour=(uint8_t)pattern;}
            uint16_t firstPixel=left&7,endPixel=(uint16_t)(firstPixel+width);
            uint8_t leading=egaPolygonLeadingMasks[firstPixel];
            for (uint16_t phase=0;phase<(alternating?2:1);++phase)
            {
                uint8_t columns=alternating?(phase?0x55:0xAA):0xFF;
                uint8_t paint=alternating?(phase?egaSpanLowColour:egaSpanHighColour):(uint8_t)pattern;
                uint16_t cursor=firstByte;
                if ((int16_t)endPixel<8)
                {
                    if ((uint8_t)endPixel>=8) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (uint8_t)endPixel>=8"); /* Adjacent mask/scratch aliases unsupported. */
                    uint8_t mask=leading&egaPolygonTrailingMasks[(uint8_t)endPixel]&columns;
                    ega.setMask(mask);ega.write(cursor,paint);
                    continue;
                }
                uint16_t remaining=width-(8-firstPixel);
                ega.setMask((uint8_t)(leading&columns));ega.write(cursor++,paint);
                uint16_t bulk=remaining>>3;
                if (bulk)
                {
                    ega.setMask(alternating?columns:0xFF);
                    do {ega.write(cursor++,paint);} while (--bulk);
                }
                uint16_t tail=remaining&7;
                if (tail)
                {
                    uint8_t mask=egaPolygonTrailingMasks[tail]&columns;
                    ega.setMask(mask);ega.write(cursor,paint);
                }
            }
        }
        uint16_t oldCount=egaSpanRowsRemaining--;
        if ((int16_t)oldCount<=1) break;
        ++row;parity^=1;
    }
}
