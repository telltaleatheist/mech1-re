#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_video.h"
#include "BTECH_2A36_CANVAS.h"
#include <stdlib.h>

/*2A36:1EB0. Truncate each MUL before accumulating DWORD; add one mask plane. */
uint32_t BtechEgaBitmapStorageSize(const BtechBitmapList *list,uint16_t *auxiliaryBytes)
{
    uint32_t packedBytes=0;
    for (uint16_t image=0;image<list->imageCount;++image)
        packedBytes+=(uint16_t)((list->images[image].widthPixels>>1)*list->images[image].heightPixels);
    *auxiliaryBytes=0;
    return packedBytes+(packedBytes>>2);
}

/*2A36:1EF1. Convert the ENTIRE packed stream before associating images.
 * Hardware scratch remains A600; typed RAM references replace selector pairs.
 * Each image retains native WORD carry loss before paragraph normalization. */
void BtechEgaPrepareBitmapList(BtechBitmapList *list,BtechResourceAllocation pixels,uint32_t packedBytes)
{
    uint16_t planeBytes=packedBytes>>2;
    BtechEgaUnpackNibblesToScratchPlanes(pixels,planeBytes);
    uint16_t scratchCursor=0,pixelOffset=0;
    uint32_t paragraphBase=0;
    for (uint16_t image=0;image<list->imageCount;++image)
    {
        BtechBitmap *bitmap=&list->images[image];
        uint16_t imageBytes=(bitmap->widthPixels>>1)*bitmap->heightPixels;
        planeBytes=imageBytes>>2;
        uint16_t maskOffset=pixelOffset+(uint16_t)(planeBytes*COMBAT_EGA_PLANES);
        if (!planeBytes || (uint32_t)pixelOffset+planeBytes*COMBAT_EGA_PLANES>UINT16_MAX
            || paragraphBase+maskOffset+planeBytes>pixels.byteCount) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !planeBytes || (uint32_t)pixelOffset+planeBytes*COMBAT_EGA_PLANES>UINT16_MAX || paragraphBase+maskOffset+planeBytes>pixels.byteCount"); // Zero DEC loops / per-image RAM wrap remain unconverted.
        bitmap->pixelPlanes=pixels.bytes+paragraphBase+pixelOffset;
        bitmap->preserveMask=pixels.bytes+paragraphBase+maskOffset;
        bitmap->pixelStorageBytes=pixels.byteCount-paragraphBase-pixelOffset;
        bitmap->maskStorageBytes=pixels.byteCount-paragraphBase-maskOffset;
        BtechEgaCopyScratchPlanes(scratchCursor,bitmap->pixelPlanes,planeBytes);
        BtechEgaBuildZeroColourMask(scratchCursor,bitmap->preserveMask,planeBytes);
        scratchCursor+=planeBytes;
        uint16_t nextOffset=maskOffset+planeBytes;
        paragraphBase+=(uint32_t)(nextOffset>>4)*0x10;
        pixelOffset=nextOffset&0x0F;
    }
    list->pixels=pixels; // Host ownership metadata; original first-image selector owns this one DOS block.
}

/*2A36:1F98. Actual packed-nibble expansion, original two WORD reads before the
 * eight pixels are written to the scratch page (one plane at a time in the
 * original; their colours on the canvas, G4). Source paragraph carry kept. */
void BtechEgaUnpackNibblesToScratchPlanes(BtechResourceAllocation source,uint16_t planeBytes)
{
    const int scratch=BtechEgaCanvasPage(COMBAT_EGA_BITMAP_SCRATCH_PAGE);
    uint16_t sourceCursor=0,destinationCursor=0,groupsRemaining=planeBytes;
    uint32_t sourceParagraph=0;
    do
    {
        if (sourceParagraph+sourceCursor+4>source.byteCount) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: sourceParagraph+sourceCursor+4>source.byteCount");
        uint16_t firstPixels=source.bytes[sourceParagraph+sourceCursor]|((uint16_t)source.bytes[sourceParagraph+sourceCursor+1]<<8);
        sourceCursor+=2;
        uint16_t secondPixels=source.bytes[sourceParagraph+sourceCursor]|((uint16_t)source.bytes[sourceParagraph+sourceCursor+1]<<8);
        uint8_t sourceWrapped=sourceCursor>=0xFFFE;
        sourceCursor+=2;
        uint8_t colours[8];
        for (uint16_t packedByte=0;packedByte<4;++packedByte)
        {
            uint8_t pixels=(packedByte<2?firstPixels:secondPixels)>>((packedByte&1)*8);
            colours[packedByte*2]=pixels>>4;
            colours[packedByte*2+1]=pixels&0x0F;
        }
        if (sourceWrapped) sourceParagraph+=0x10000;
        BtechEgaCanvasSetPixels(scratch,destinationCursor,colours);
        ++destinationCursor;
    } while (--groupsRemaining);
}

/*2A36:209B. Repeat the same scratch range per colour bit (plane); zero copies
 * nothing. The image keeps the planar form (BTECH_2A36_CANVAS.h). */
void BtechEgaCopyScratchPlanes(uint16_t sourceOffset,uint8_t *destination,uint16_t planeBytes)
{
    const int scratch=BtechEgaCanvasPage(COMBAT_EGA_BITMAP_SCRATCH_PAGE);
    for (uint16_t plane=0;plane<COMBAT_EGA_PLANES;++plane)
    {
        uint16_t sourceCursor=sourceOffset;
        for (uint16_t remaining=planeBytes;remaining;--remaining)
            *destination++=BtechEgaCanvasColourBits(scratch,sourceCursor++,plane);
    }
}

/*2A36:20CE. Zero-colour mask is the inverse OR of the four scratch colour bits:
 * a 1 where the pixel is colour 0. The original DEC loop remains. */
void BtechEgaBuildZeroColourMask(uint16_t sourceOffset,uint8_t *destination,uint16_t planeBytes)
{
    const int scratch=BtechEgaCanvasPage(COMBAT_EGA_BITMAP_SCRATCH_PAGE);
    uint16_t sourceCursor=sourceOffset,remaining=planeBytes;
    do
    {
        uint8_t opaquePixels=0;
        for (uint16_t plane=0;plane<COMBAT_EGA_PLANES;++plane)
            opaquePixels|=BtechEgaCanvasColourBits(scratch,sourceCursor,plane);
        *destination++=(uint8_t)~opaquePixels;
        ++sourceCursor;
    } while (--remaining);
}
