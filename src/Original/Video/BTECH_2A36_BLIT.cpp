#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_video.h"
#include "BTECH_2A36_CANVAS.h"
#include <stdlib.h>

/*2A36:083F. Eight native shift continuations share this one FAR boundary.
 * Descriptor snapshots precede clipping. The mask pass sets the image's opaque
 * pixels to colour 0; then, for each colour bit the live 2E5F selection enables,
 * the image's bits are ORed into the pixels' colours (one plane at a time in
 * the original; on the canvas, G4). DF-clear and ordinary RAM payload owners
 * only; no invented shifted-blit helper. */
void BtechEgaDrawBitmap(const BtechBitmap *bitmap,uint16_t x,uint16_t y)
{
    if (!bitmap) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !bitmap");
    const BtechBitmap image=*bitmap;
    CombatPage page=combatVideo.drawPageSelector;uint16_t rows=image.heightPixels;
    uint16_t rowStart,sourceSkip=0;
    uint8_t rowBytes;
    if (!combatVideo.screenPolygonNeedsClip)
    {
        if (y>=COMBAT_SCREEN_HEIGHT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: y>=COMBAT_SCREEN_HEIGHT");
        rowStart=(uint16_t)(combatVideo.graphicsRowOffsets[y]+(x>>3));
        combatVideo.egaBitmapShiftPhase=x&7;
        rowBytes=(uint8_t)(image.widthPixels>>3);
        combatVideo.egaBitmapPlaneStride=(uint16_t)(rowBytes*(uint8_t)rows);
        combatVideo.egaBitmapSourceRowSkip=0;combatVideo.egaBitmapClipSides=0;
        combatVideo.egaBitmapDestinationRowSkip=(uint8_t)(COMBAT_SCREEN_ROW_BYTES-rowBytes);
    }
    else
    {
        uint16_t packedWidth=image.widthPixels>>3;
        combatVideo.egaBitmapPlaneStride=(uint16_t)((uint8_t)packedWidth*(uint8_t)rows);
        uint16_t rowY=y;
        if ((int16_t)rowY<combatVideo.clipMinimumY)
        {
            uint16_t skippedRows=combatVideo.clipMinimumY-rowY;
            if ((int16_t)rows<=(int16_t)skippedRows) return;
            rows-=skippedRows;sourceSkip=(uint16_t)((uint8_t)skippedRows*(uint8_t)packedWidth);
            rowY=(uint16_t)combatVideo.clipMinimumY;
        }
        if ((uint16_t)(rowY+rows)>(uint16_t)combatVideo.clipMaximumY)
        {
            rows=(uint16_t)(combatVideo.clipMaximumY-rowY+1);
            if (!rows || rows>0x8000) return; /* INC/JLE admits8000. */
        }
        if (rowY>=COMBAT_SCREEN_HEIGHT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: rowY>=COMBAT_SCREEN_HEIGHT");
        rowStart=combatVideo.graphicsRowOffsets[rowY];
        uint16_t clippedWidth=(uint16_t)(packedWidth<<3),leftX=x;
        uint16_t minimumX=(uint16_t)combatVideo.clipMinimumX&0xFFF8;
        if ((int16_t)leftX<(int16_t)minimumX)
        {
            uint16_t skippedPixels=minimumX-leftX;
            if ((int16_t)clippedWidth<=(int16_t)skippedPixels) return;
            clippedWidth-=skippedPixels;combatVideo.egaBitmapShiftPhase=clippedWidth&7;
            sourceSkip+=(uint16_t)((uint16_t)((clippedWidth&7)+skippedPixels)>>3);
            leftX=(uint16_t)combatVideo.clipMinimumX&0xFFF8;combatVideo.egaBitmapClipSides=1;
        }
        else {combatVideo.egaBitmapShiftPhase=leftX&7;combatVideo.egaBitmapClipSides=0;}
        rowStart+=(leftX>>3);
        combatVideo.egaBitmapRoundedEndX=(uint16_t)(combatVideo.clipMaximumX+1)&0xFFF8;
        if ((uint16_t)(leftX+clippedWidth)>=combatVideo.egaBitmapRoundedEndX)
        {
            if ((int16_t)combatVideo.egaBitmapRoundedEndX<=(int16_t)leftX) return;
            clippedWidth=(uint16_t)(combatVideo.egaBitmapRoundedEndX-leftX);combatVideo.egaBitmapClipSides|=2;
        }
        rowBytes=(uint8_t)(clippedWidth>>3);
        if (combatVideo.egaBitmapClipSides==3 && (x&7)) --rowBytes;
        uint16_t originalRowBytes=image.widthPixels>>3;
        combatVideo.egaBitmapSourceRowSkip=(originalRowBytes&0xFF00)|(uint8_t)((uint8_t)originalRowBytes-rowBytes);
        combatVideo.egaBitmapDestinationRowSkip=(uint8_t)(COMBAT_SCREEN_ROW_BYTES-rowBytes);
    }
    uint8_t phase=combatVideo.egaBitmapShiftPhase;
    uint16_t maskCursor=sourceSkip,destination=rowStart;
    uint8_t rowsRemaining=rows;
    BtechEgaCanvasWriter ega(page);
    do
    {
        uint8_t bytesRemaining=rowBytes;
        if (!phase)
        {
            do
            {
                if (maskCursor>=image.maskStorageBytes) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: maskCursor>=image.maskStorageBytes");
                uint8_t preserve=image.preserveMask[maskCursor++];
                ega.setMask((uint8_t)~preserve);ega.write(destination++,0);
            } while (--bytesRemaining);
        }
        else
        {
            uint8_t previous=0xFF;
            if (combatVideo.egaBitmapClipSides&1)
            {
                uint16_t prior=maskCursor-1;if (prior>=image.maskStorageBytes) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: prior>=image.maskStorageBytes");
                previous=image.preserveMask[prior];
            }
            while ((int8_t)bytesRemaining>0)
            {
                --bytesRemaining;
                if (maskCursor>=image.maskStorageBytes) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: maskCursor>=image.maskStorageBytes");
                uint8_t current=image.preserveMask[maskCursor++];
                uint8_t preserve=(previous<<(8-phase))|(current>>phase);previous=current;
                ega.setMask((uint8_t)~preserve);ega.write(destination++,0);
            }
            uint8_t current=0xFF;
            if (combatVideo.egaBitmapClipSides&2)
            {if (maskCursor>=image.maskStorageBytes) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: maskCursor>=image.maskStorageBytes");current=image.preserveMask[maskCursor];}
            uint8_t preserve=(previous<<(8-phase))|(current>>phase);
            ega.setMask((uint8_t)~preserve);ega.write(destination,0);
        }
        if (!--rowsRemaining) break;
        maskCursor+=combatVideo.egaBitmapSourceRowSkip;destination+=combatVideo.egaBitmapDestinationRowSkip;
    } while (1);
    const int canvasPage=BtechEgaCanvasPage(page);
    uint8_t clipSides=combatVideo.egaBitmapClipSides;
    uint16_t planeStart=sourceSkip;
    for (uint16_t plane=0;plane<COMBAT_EGA_PLANES;++plane)
    {
        if (plane) planeStart+=combatVideo.egaBitmapPlaneStride;
        if (!(combatVideo.egaBitmapEnabledPlanes&(1<<plane))) continue;
        uint16_t cursor=planeStart;destination=rowStart;rowsRemaining=(uint8_t)rows;
        do
        {
            uint8_t previous=0;
            if (phase && (clipSides&1))
            {
                uint16_t prior=cursor-1;if (prior>=image.pixelStorageBytes) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: prior>=image.pixelStorageBytes");
                previous=image.pixelPlanes[prior];
            }
            uint16_t count=!phase?(rowBytes?rowBytes:0x100):((int8_t)rowBytes>0?rowBytes:0);
            while (count--)
            {
                if (cursor>=image.pixelStorageBytes) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: cursor>=image.pixelStorageBytes");
                uint8_t pixels=image.pixelPlanes[cursor++];
                uint8_t ink=phase?(uint8_t)((previous<<(8-phase))|(pixels>>phase)):pixels;previous=pixels;
                BtechEgaCanvasAddColourBits(canvasPage,destination++,plane,ink);
            }
            if (phase)
            {
                uint8_t pixels=0;
                if (clipSides&2) {if (cursor>=image.pixelStorageBytes) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: cursor>=image.pixelStorageBytes");pixels=image.pixelPlanes[cursor];}
                BtechEgaCanvasAddColourBits(canvasPage,destination,plane,(uint8_t)((previous<<(8-phase))|(pixels>>phase)));
            }
            if (!--rowsRemaining) break;
            cursor+=combatVideo.egaBitmapSourceRowSkip;destination+=combatVideo.egaBitmapDestinationRowSkip;
        } while (1);
    }
}
