#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_polygons.h"
#include <stdlib.h>
#include <string.h>

/*1BF5:0006. Authored default graphics dispatch is a genuine single RETF,
 * not a null pointer or a success-returning substitute renderer. This C
 * signature adapts the register glyph call only; Reko remains name-only. */
void BtechGraphicsNoOperation(uint16_t character,uint16_t x,uint16_t y,uint16_t page)
{
    (void)character;(void)x;(void)y;(void)page;
}

/*1BF5:0007. The EGA branch of the original renderer installer. Only the
 * converted glyph/font/palette entries of its23-entry dispatch are callable. Other
 * renderers must not be mapped onto EGA or claimed to initialize successfully. */
uint16_t BtechSelectCombatRenderer(uint8_t adapterOverride,uint8_t allowedMask)
{
    static const uint8_t overrideMasks[]={0,1,2,4,8,0x10,0x20,0x40,0x80};
    if (adapterOverride>=sizeof(overrideMasks)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: adapterOverride>=sizeof(overrideMasks)");
    combatVideo.videoAdapterAllowedMask=adapterOverride?overrideMasks[adapterOverride]:allowedMask;
    combatStartup.videoAdapter=(uint8_t)BtechGetBiosDisplayAdapterBits();
    combatVideo.combatRendererKind=(uint8_t)BtechDetectVideoAdapter();
    if (combatVideo.combatRendererKind==COMBAT_VIDEO_EGA)
    {
        combatVideo.graphicsDrawGlyph=BtechEgaDrawGlyph;
        combatVideo.graphicsLoadFont=BtechEgaLoadFont;
        combatVideo.graphicsLoadPalette=BtechEgaLoadPalette;
        combatVideo.graphicsSelectPalette=BtechEgaSelectPalette;
        combatVideo.graphicsBitmapStorageSize=BtechEgaBitmapStorageSize;
        combatVideo.graphicsPrepareBitmapList=BtechEgaPrepareBitmapList;
        combatVideo.graphicsDrawBitmap=BtechEgaDrawBitmap;
        combatVideo.graphicsCaptureStorageSize=BtechEgaGetCaptureStorageSize;
        combatVideo.graphicsCaptureRectangle=BtechEgaCaptureScreenRectangle;
        combatVideo.graphicsRestoreRectangle=BtechEgaRestoreScreenRectangle;
        combatVideo.graphicsUploadPackedScreenRegion=BtechEgaUploadPackedScreenRegion;
        combatVideo.graphicsUploadAlternateScreenRegion=BtechEgaNoOperation;
        combatVideo.graphicsPresentRectangle=BtechEgaNoOperation;
        combatVideo.graphicsFillSolidSpan=BtechEgaFillSolidSpan;
        combatVideo.graphicsFillAlternatingSpan=BtechEgaFillAlternatingSpan;
        combatPolygons.graphicsFillPolygonSpans=BtechEgaFillPolygonSpans;
        combatPolygons.graphicsDrawLine=BtechEgaDrawLine;
        combatVideo.graphicsWritePixel=BtechEgaWritePixel;combatVideo.graphicsReadPixel=BtechEgaReadPixel;
        combatVideo.graphicsSetOverscanColour=BtechEgaSetOverscanColour;
        combatVideo.graphicsCopyScreenRectangle=BtechEgaCopyScreenRectangle;
        combatVideo.graphicsSwapScreenPages=BtechEgaSwapScreenPages;
        BtechInitializeEgaVideo(0); /* Native dispatch copy exhaustsCX. */
    }
    if (!btechHostWriteBiosWord) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostWriteBiosWord");
    btechHostWriteBiosWord(0xF0,0x304E); /* Physical0000:04F0, not game state. */
    combatVideo.copySourcePageSelector=combatVideo.frontPageSelector;
    combatVideo.drawPageSelector=combatVideo.backPageSelector;
    return combatVideo.combatRendererKind;
}

/*1BF5:0455. FAR tail through the installed backend; DX:AX is a byte count. */
uint32_t BtechGetCaptureStorageSize(uint16_t width,uint16_t height)
{
    if (!combatVideo.graphicsCaptureStorageSize) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !graphicsCaptureStorageSize");
    return combatVideo.graphicsCaptureStorageSize(width,height);
}

/*1BF5:0451. FAR tail dispatch. Typed paragraph-aligned allocation replaces
 * the original offset0/selector pair, not the remaining four native WORDs. */
void BtechCaptureScreenRectangle(BtechResourceAllocation buffer,uint16_t x,uint16_t y,uint16_t width,uint16_t height)
{
    if (!combatVideo.graphicsCaptureRectangle) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !graphicsCaptureRectangle");
    combatVideo.graphicsCaptureRectangle(buffer,x,y,width,height);
}

/*1BF5:0459. Same native buffer, but destination is live drawPage. */
void BtechRestoreScreenRectangle(BtechResourceAllocation buffer,uint16_t x,uint16_t y,uint16_t width,uint16_t height)
{
    if (!combatVideo.graphicsRestoreRectangle) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !graphicsRestoreRectangle");
    combatVideo.graphicsRestoreRectangle(buffer,x,y,width,height);
}

/*1BF5:0449. FAR tail to the selected bitmap backend, three native WORDs. */
void BtechDrawBitmap(const BtechBitmap *bitmap,int16_t x,int16_t y)
{
    if (!combatVideo.graphicsDrawBitmap) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !graphicsDrawBitmap");
    combatVideo.graphicsDrawBitmap(bitmap,(uint16_t)x,(uint16_t)y);
}

/*1BF5:00FD. Shipped22D8 mask7/forced-EGA path. Preserve this original NEAR
 * boundary; advanced adapters and successful CGA/Tandy detection stay gated.
 * Incidental BX/DX results are unobserved by the supported EGA installer. */
uint16_t BtechDetectVideoAdapter(void)
{
    if (combatVideo.videoAdapterAllowedMask&~COMBAT_VIDEO_ALLOWED) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: videoAdapterAllowedMask&~COMBAT_VIDEO_ALLOWED");
    if (!btechHostQueryEgaInfo || !btechHostReadBiosByte) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostQueryEgaInfo || !btechHostReadBiosByte");
    if ((combatVideo.videoAdapterAllowedMask&COMBAT_VIDEO_EGA) && btechHostQueryEgaInfo()!=0x0010)
    {
        if (btechHostReadBiosByte(0x87)&8) BtechSetBiosTextModeAndAdapterBits(2);
        return COMBAT_VIDEO_EGA;
    }
    else if (combatVideo.videoAdapterAllowedMask&5)
    {
        /*00FD: without an EGA, the original writes 66h to CRTC register 0Fh,
         * reads it back and restores 0Fh, to tell a CGA or Tandy adapter from
         * none. The game runs only on its EGA path (the BIOS's EGA reply
         * above), so this check and the CGA/Tandy set-up are not converted. */
        /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("CGA/Tandy adapter check and initialization not converted");
    }
    return 0;
}

/*1BF5:02AA. Equipment display bits, not a modern adapter classification. */
uint16_t BtechGetBiosDisplayAdapterBits(void)
{
    if (!btechHostReadBiosByte) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostReadBiosByte");
    return (btechHostReadBiosByte(BIOS_VIDEO_EQUIPMENT)&0x30)>>4;
}

/*1BF5:02C2. Shared text policy remains above the glyph hardware backend.
 * Vertical bounds add height to AL only. Rejection happens BEFORE CLD;
 * the compiled glyph path is consequently always forward. */
void BtechDrawNearText(const char *text,int16_t x,int16_t y)
{
    uint16_t page=combatVideo.drawPageSelector;
    combatVideo.textBackgroundPattern=(uint8_t)combatVideo.graphicsColourPatterns[combatVideo.textBackgroundColour];
    combatVideo.textForegroundPattern=(uint8_t)combatVideo.graphicsColourPatterns[combatVideo.textForegroundColour];
    if (combatVideo.screenPolygonNeedsClip)
    {
        if (y<combatVideo.clipMinimumY) return;
        int16_t bottom=((uint16_t)y&0xFF00)|(uint8_t)((uint8_t)y+combatVideo.combatFontHeights[0]);
        if (bottom>combatVideo.clipMaximumY) return;
    }
    while (*text)
    {
        uint8_t character=(uint8_t)*text++;
        if (!combatVideo.screenPolygonNeedsClip || x>=combatVideo.clipMinimumX)
        {
            if (combatVideo.screenPolygonNeedsClip
                && (int16_t)(x+combatVideo.fontAdvanceWidths[0]-1)>combatVideo.clipMaximumX) return;
            if (!combatVideo.graphicsDrawGlyph) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !graphicsDrawGlyph");
            combatVideo.graphicsDrawGlyph(character,(uint16_t)x,(uint16_t)y,page);
        }
        x=(int16_t)(x+combatVideo.fontAdvanceWidths[0]);
    }
}

/*1BF5:1A62. Original shared loader, not a font manager. The same native
 * filename/header buffer is lent to2111 for glyph staging. */
uint16_t BtechLoadFontSlot(const char *filename,BtechRuntimeStream *stream)
{
    uint16_t slot=1;
    while (slot<COMBAT_FONT_SLOTS && combatVideo.fonts[slot].bytes) ++slot;
    if (slot==COMBAT_FONT_SLOTS) return 0;
    union {uint8_t bytes[COMBAT_FONT_READ_BYTES];BtechFontMetrics metrics;} scratch;
    uint16_t openedHere=0;
    if (!stream || !BtechIsRegisteredTaggedStream(stream))
    {
        if (!filename || strlen(filename)+(strchr(filename,'.')?1:5)>sizeof(scratch.bytes)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !filename || strlen(filename)+(strchr(filename,'.')?1:5)>sizeof(scratch.bytes)"); // Native filename spill is outside this port.
        strcpy((char *)scratch.bytes,filename);
        if (!strchr((char *)scratch.bytes,'.')) strcat((char *)scratch.bytes,".fnt");
        stream=BtechOpenTaggedResourceStream((char *)scratch.bytes);openedHere=1;
    }
    if (BtechFindResourceTagPosition(stream,"FNT:",0)==UINT32_MAX
        || BtechReadStreamElements(scratch.bytes,1,sizeof(scratch.metrics),stream->file)!=sizeof(scratch.metrics)) slot=0;
    else
    {
        combatVideo.fontAdvanceWidths[slot]=scratch.metrics.advancePixels;
        combatVideo.combatFontHeights[slot]=scratch.metrics.heightPixels;
        combatVideo.fontFirstCharacters[slot]=scratch.metrics.firstCharacter;
        combatVideo.fontCharacterCounts[slot]=scratch.metrics.characterCount;
        if (!combatVideo.graphicsLoadFont) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !graphicsLoadFont");
        slot=combatVideo.graphicsLoadFont(stream,slot,scratch.bytes);
    }
    if (openedHere) BtechCloseTaggedResourceStream(stream);
    return slot;
}

/*1BF5:1B86. Slot zero is an active-selector query. Selection is a snapshot
 * of four metrics and the allocation reference, not a live slot alias. */
uint16_t BtechSelectFontSlot(int16_t slot)
{
    uint16_t selected=0;
    if (!slot)
    {
        if (combatVideo.fonts[0].bytes)
            for (selected=1;selected<COMBAT_FONT_SLOTS;++selected)
                if (combatVideo.fonts[selected].bytes==combatVideo.fonts[0].bytes) break;
    }
    else if (BtechIsFontSlotLoaded(slot))
    {
        selected=(uint16_t)slot;
        combatVideo.fontAdvanceWidths[0]=combatVideo.fontAdvanceWidths[selected];
        combatVideo.combatFontHeights[0]=combatVideo.combatFontHeights[selected];
        combatVideo.fontFirstCharacters[0]=combatVideo.fontFirstCharacters[selected];
        combatVideo.fontCharacterCounts[0]=combatVideo.fontCharacterCounts[selected];
        combatVideo.fonts[0]=combatVideo.fonts[selected];
    }
    return selected;
}

/*1BF5:1CE0. Signed bounds, excluding active slot zero. */
uint16_t BtechIsFontSlotLoaded(int16_t slot)
{
    return slot>0 && slot<COMBAT_FONT_SLOTS && combatVideo.fonts[slot].bytes!=NULL;
}

/*1BF5:1FA8. The EGA startup path; native adapter-indexed tag selection is
 * not permission to substitute an EGA loader for another adapter. */
uint16_t BtechLoadPaletteSlot(const char *filename,BtechRuntimeStream *stream)
{
    uint16_t slot=1;
    while (slot<COMBAT_PALETTE_SLOTS && combatVideo.paletteSlotIds[slot]) ++slot;
    if (slot==COMBAT_PALETTE_SLOTS) slot=0;
    else
    {
        char name[COMBAT_FONT_READ_BYTES];
        uint16_t openedHere=0;
        if (!stream || !BtechIsRegisteredTaggedStream(stream))
        {
            if (!filename || strlen(filename)+(strchr(filename,'.')?1:5)>sizeof(name)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !filename || strlen(filename)+(strchr(filename,'.')?1:5)>sizeof(name)");
            strcpy(name,filename);
            if (!strchr(name,'.')) strcat(name,".pal");
            stream=BtechOpenTaggedResourceStream(name);openedHere=1;
        }
        if (combatVideo.combatRendererKind!=COMBAT_VIDEO_EGA) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: combatRendererKind!=COMBAT_VIDEO_EGA");
        if (BtechFindResourceTagPosition(stream,"PAL:EGA:",0)==UINT32_MAX) slot=0;
        else
        {
            if (!combatVideo.graphicsLoadPalette) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !graphicsLoadPalette");
            slot=combatVideo.graphicsLoadPalette(stream,slot);
        }
        if (openedHere) BtechCloseTaggedResourceStream(stream);
    }
    if (slot>=COMBAT_PALETTE_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: slot>=COMBAT_PALETTE_SLOTS"); // Foreign callback's wrapped registration aliases remain gated.
    combatVideo.paletteSlotIds[slot]=(uint8_t)slot; // Failure clears active ID, not palette contents.
    return slot;
}

/*1BF5:2092. Query sign-extends the active BYTE. Backend precedes ID store. */
uint16_t BtechSelectPaletteSlot(int16_t slot)
{
    if (!slot) return (int8_t)combatVideo.paletteSlotIds[0];
    if (!BtechIsPaletteSlotLoaded(slot)) return 0;
    if (!combatVideo.graphicsSelectPalette) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !graphicsSelectPalette");
    combatVideo.graphicsSelectPalette((uint16_t)slot);
    combatVideo.paletteSlotIds[0]=(uint8_t)slot;
    return (uint16_t)slot;
}

/*1BF5:20FC. */
uint16_t BtechIsPaletteSlotLoaded(int16_t slot)
{
    return slot>0 && slot<COMBAT_PALETTE_SLOTS && combatVideo.paletteSlotIds[slot]!=0;
}
