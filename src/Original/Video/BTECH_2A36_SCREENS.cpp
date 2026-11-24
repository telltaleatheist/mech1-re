#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_video.h"
#include "BTECH_2A36_CANVAS.h"
#include <stdlib.h>

/*2A36:1B4C. Same-coordinate aligned byte copy (the original's latch copy, write
 * mode 1), eight pixels at a time on the canvas (G4). No clip, page swap or
 * invented logical-pixel blit. */
void BtechEgaCopyScreenRectangle(uint16_t x,uint16_t y,uint16_t width,uint16_t height)
{
    uint16_t end=(uint16_t)(x+width+7)&0xFFF8,first=x&0xFFF8;
    uint16_t bytes=(uint16_t)(end-first)>>3,row=(uint16_t)(y*2)>>1;
    if (row>=COMBAT_SCREEN_HEIGHT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: row>=COMBAT_SCREEN_HEIGHT");
    uint16_t cursor=combatVideo.graphicsRowOffsets[row]+(first>>3);
    const int destinationPage=BtechEgaCanvasPage(combatVideo.drawPageSelector),sourcePage=BtechEgaCanvasPage(combatVideo.copySourcePageSelector);
    do
    {
        uint16_t nextRow=cursor+COMBAT_SCREEN_ROW_BYTES;
        for (uint16_t remaining=bytes;remaining;--remaining)
            BtechEgaCanvasCopy(sourcePage,destinationPage,cursor++);
        cursor=nextRow;
    } while (--height);
}

/*2A36:000B. Installed alternate SCR:VGA target really is a single RETF. */
void BtechEgaNoOperation(void)
{
    /* Authored CB. Not a success-returning placeholder for another adapter. */
}

/*2A36:23AA..24DA (epilogue aliases2C83:0000). Forward packed IO to EGA planes.
 * No clipping/alignment policy added. Each two-WORD read precedes its eight
 * pixels (written one plane at a time in the original; their colours on the
 * canvas, G4). This typed source covers forward, non-video IO buffers, not
 * physical aliases. */
void BtechEgaUploadPackedScreenRegion(BtechResourceAllocation source,uint16_t left,uint16_t top,uint16_t width,uint16_t height)
{
    if (top>=COMBAT_SCREEN_HEIGHT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: top>=COMBAT_SCREEN_HEIGHT"); /* Native row-table adjacent-owner alias not represented. */
    uint16_t destination=combatVideo.graphicsRowOffsets[top]+(left>>3);
    const int page=BtechEgaCanvasPage(combatVideo.drawPageSelector);
    combatVideo.egaScreenUploadRowsRemaining=height;
    combatVideo.egaScreenUploadColumns=width>>3;
    uint16_t columnsRemaining=combatVideo.egaScreenUploadColumns;
    combatVideo.egaScreenUploadRowSkip=COMBAT_SCREEN_ROW_BYTES-columnsRemaining;
    uint32_t sourceCursor=0; /* Host extent check gates normalized-segment wrap, rather than guessing a paragraph alias. */
    for (;;)
    {
        uint8_t colours[8];
        for (uint16_t sourceWord=0;sourceWord<2;++sourceWord)
        {
            if (!source.bytes || sourceCursor+2>source.byteCount || sourceCursor+2>0x10000) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !source.bytes || sourceCursor+2>source.byteCount || sourceCursor+2>0x10000");
            uint16_t packedPixels=source.bytes[sourceCursor]|((uint16_t)source.bytes[sourceCursor+1]<<8);
            sourceCursor+=2;
            for (uint16_t sourceByte=0;sourceByte<2;++sourceByte)
            {
                uint8_t packedPair=packedPixels>>(sourceByte*8);
                colours[sourceWord*4+sourceByte*2]=packedPair>>4;
                colours[sourceWord*4+sourceByte*2+1]=packedPair&0x0F;
            }
        }
        BtechEgaCanvasSetPixels(page,destination,colours);
        destination++;
        uint16_t columnsBeforeDecrement=columnsRemaining--;
        if ((int16_t)columnsBeforeDecrement>1) continue;
        if ((int16_t)combatVideo.egaScreenUploadRowsRemaining<=1) break;
        combatVideo.egaScreenUploadRowsRemaining--;
        columnsRemaining=combatVideo.egaScreenUploadColumns;
        destination+=combatVideo.egaScreenUploadRowSkip;
    }
}
