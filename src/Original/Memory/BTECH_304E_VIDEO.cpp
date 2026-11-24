#include "mechwarrior/combat_video.h"
#include "mechwarrior/native_address.h"
#include "mechwarrior/ui_layout.h"

const uint8_t glyphTailMasks[8]={0xFF,0x80,0xC0,0xE0,0xF0,0xFF,0xFC,0xFE};
const char errorCardNotFound[]="no supported graphics adapter was found";

CombatVideoState combatVideo;
static_assert(sizeof(BtechLandscapeCameraColourRow)==0x1E,"Native landscape colour row");

/* EXEPACK data projection, not a runtime segment mirror. The shipped font
 * directory/metrics are ZERO: an early fatal message has no default font.
 * Do not borrow the campaign font or manufacture a BIOS font fallback. */
bool BtechImportVideoInitialData(const uint8_t *data,size_t bytes)
{
    if (!data || bytes<0x4EDC) return false;
    for (uint16_t slot=0;slot<COMBAT_FONT_SLOTS;++slot)
    {
        if (MwReadNativeWord(data,0x2ED4+slot*2)) return false;
        combatVideo.fonts[slot]=BtechFont{};
        /* 2E84, 2E98, 2EAC, 2EC0: a slot's metrics, written when its font
         * loads (BtechLoadFontSlot) and zero where the exe reloaded them. */
        combatVideo.fontAdvanceWidths[slot]=0;combatVideo.combatFontHeights[slot]=0;
        combatVideo.fontFirstCharacters[slot]=0;combatVideo.fontCharacterCounts[slot]=0;
    }
    if (MwReadNativeWord(data,0x3F38)!=0x0006 || MwReadNativeWord(data,0x3F3A)!=0x1BF5) return false;
    combatVideo.graphicsDrawGlyph=BtechGraphicsNoOperation;
    /* Other authored RETF dispatch entries have no typed callable adapter
     * before installation. Startup must select a real backend first. */
    combatVideo.graphicsLoadFont=NULL;combatVideo.graphicsLoadPalette=NULL;combatVideo.graphicsSelectPalette=NULL;
    combatVideo.graphicsBitmapStorageSize=NULL;combatVideo.graphicsPrepareBitmapList=NULL;
    combatVideo.graphicsDrawBitmap=NULL;
    combatVideo.egaBitmapRoundedEndX=0; /*46E6*/combatVideo.egaBitmapShiftPhase=0; /*46E8*/
    combatVideo.egaBitmapClipSides=0; /*46EA*/combatVideo.egaBitmapSourceRowSkip=0; /*46EB*/
    combatVideo.egaBitmapDestinationRowSkip=0; /*46ED*/combatVideo.egaBitmapPlaneStride=0; /*46EF*/
    combatVideo.graphicsCaptureStorageSize=NULL;
    combatVideo.graphicsUploadPackedScreenRegion=NULL;combatVideo.graphicsUploadAlternateScreenRegion=NULL;
    combatVideo.graphicsPresentRectangle=NULL;
    combatVideo.egaScreenUploadColumns=0; /*4724*/
    combatVideo.egaScreenUploadRowSkip=0; /*4726*/combatVideo.egaScreenUploadRowsRemaining=0; /*4728*/
    combatVideo.graphicsColourMode=data[0x2E6F];
    combatVideo.graphicsWritePixel=NULL;combatVideo.graphicsReadPixel=NULL;combatVideo.graphicsSetOverscanColour=NULL;
    combatVideo.graphicsCopyScreenRectangle=NULL;
    combatVideo.graphicsSwapScreenPages=NULL;
    for (uint16_t bit=0;bit<8;++bit) combatVideo.egaPixelBitMasks[bit]=data[0x46D4+bit];
    combatVideo.egaLinePixelMask=0; /*46DC*/combatVideo.egaLineRunLength=0; /*46DD*/
    combatVideo.egaLineFractionStep=0; /*46DF*/combatVideo.egaLineRemainingBeforeRun=0; /*46E1*/combatVideo.egaLineFractionAccumulator=0; /*46E3*/
    /* The layout's camera rows (data/ui, src/Ui), at their native offset. */
    uint8_t ui[0x1F3C];
    if (MwUiPieceSize("cockpit.landscape_camera_rows")!=0x1F3C-0x1E88
        || !MwUiCopyNative("cockpit.landscape_camera_rows",ui+0x1E88,0x1F3C-0x1E88)) return false;
    /* Actual image/BSS IO boundary; runtime consumers use typed rows. */
    for (uint16_t scheme=0;scheme<COMBAT_HUD_COLOUR_SCHEMES;++scheme)
        for (uint16_t landscape=0;landscape<COMBAT_LANDSCAPES;++landscape)
        {
            BtechLandscapeCameraColourRow *row=&combatVideo.landscapeCameraColourRows[scheme][landscape];
            uint16_t offset=0x1E88+scheme*0x5A+landscape*0x1E;
            row->playerFillPaletteA=ui[offset];row->unidentifiedField2=ui[offset+1];
            row->sharedFillPaletteB=ui[offset+2];
            row->unidentifiedField3=ui[offset+3];row->unidentifiedField4[0]=ui[offset+4];row->unidentifiedField4[1]=ui[offset+5];
            row->mapBoundaryColour=MwReadNativeWord(ui,offset+6);row->mapRetreatBoundaryColour=MwReadNativeWord(ui,offset+8);
            for (uint16_t unit=0;unit<5;++unit) row->unitMarkerColours[unit]=MwReadNativeWord(ui,offset+0x0A+unit*2);
            row->orderTrackingLineColour=MwReadNativeWord(ui,offset+0x14);
            row->hudReticleColour=ui[offset+0x16];row->unidentifiedField11=ui[offset+0x17];
            row->headingTickPaletteIndex=MwReadNativeWord(ui,offset+0x18);
            for (uint16_t byte=0;byte<sizeof(row->unidentifiedField12);++byte) row->unidentifiedField12[byte]=ui[offset+0x1A+byte];
        }
    for (uint16_t landscape=0;landscape<COMBAT_LANDSCAPES;++landscape) combatVideo.landscapePaletteSlots[landscape]=0;
    combatVideo.damagePaletteSlot=0;
    combatVideo.playerCameraFillASuppressed=0;combatVideo.inspectionReloadPending=0;combatVideo.inspectionSetupComplete=0;
    for (uint16_t slot=0;slot<COMBAT_BITMAP_LISTS;++slot)
    {combatVideo.bitmapRegistryCounts[slot]=0;combatVideo.bitmapRegistryLists[slot]=NULL;}
    combatVideo.promptBitmaps=NULL; // Native fresh-child BSS, not a selective combat reset.
    combatVideo.pointPictures=NULL;combatVideo.smallMechPictures=NULL;combatVideo.strutPictures=NULL;
    combatVideo.cockpitPictures=NULL;combatVideo.hudGlyphPictures=NULL;combatVideo.inspectionDamagePictures=NULL;
    if (MwReadNativeWord(data,0x4714)) return false;
    combatVideo.egaFontBuffer=NULL;combatVideo.egaFontChunkCounter=0; /*4716*/
    for (uint16_t slot=0;slot<COMBAT_PALETTE_SLOTS;++slot) combatVideo.paletteSlotIds[slot]=data[0x3796+slot];
    combatVideo.textForegroundColour=data[0x2E50];combatVideo.textBackgroundColour=data[0x2E51];
    combatVideo.textTransparentBackground=data[0x2E52];combatVideo.screenPolygonNeedsClip=data[0x2E5B];
    combatVideo.drawPolygonFilled=data[0x2E5C];combatVideo.egaBitmapEnabledPlanes=data[0x2E5F];
    combatVideo.drawLineColour=data[0x2E5D];combatVideo.drawFillColour=data[0x2E5E];
    combatVideo.drawColourPattern=0; /*2E72*/combatVideo.spanPrimaryPattern=0; /*2E74*/
    combatVideo.spanSecondaryPattern=0; /*2E76*/combatVideo.spanAlternationState=0; /*2E7A*/
    for (uint16_t i=0;i<8;++i) {combatVideo.egaSpanLeadingMasks[i]=data[0x4702+i];combatVideo.egaSpanTrailingMasks[i]=data[0x470A+i];}
    combatVideo.egaSpanPrimaryColour=data[0x4712];combatVideo.egaSpanSecondaryColour=data[0x4713];
    combatVideo.graphicsFillSolidSpan=NULL;combatVideo.graphicsFillAlternatingSpan=NULL;combatVideo.shapeSpanCallback=BtechShapeSpanCallback{};
    combatVideo.clipMinimumX=0; /*2E53*/combatVideo.clipMaximumX=(int16_t)MwReadNativeWord(data,0x2E55);
    combatVideo.clipMinimumY=0; /*2E57*/combatVideo.clipMaximumY=(int16_t)MwReadNativeWord(data,0x2E59);
    combatVideo.backPage=CombatPage::None; /*2E62*/
    combatVideo.frontPage=CombatPage::None; /*2E64*/combatVideo.copySourcePage=CombatPage::None; /*2E66*/
    combatVideo.drawPage=CombatPage::None; /*2E68*/combatVideo.videoColourCount=0; /*2E71*/
    combatVideo.pcjrTandyCompatible=data[0x2E6C];combatVideo.combatRendererKind=data[0x2E6D];
    combatVideo.textForegroundPattern=data[0x2E78];combatVideo.textBackgroundPattern=data[0x2E79];
    combatVideo.glyphTailMask=data[0x46D2];combatVideo.videoAdapterAllowedMask=data[0x3FC5];
    combatVideo.ellipsePage=CombatPage::None; /*4151*/
    for (uint16_t row=0;row<COMBAT_ELLIPSE_ROWS;++row)
    {combatVideo.ellipseLeftInsets[row]=data[0x401C+row];combatVideo.ellipseBorderWidths[row]=data[0x4083+row];combatVideo.ellipseInteriorWidths[row]=data[0x40EA+row];}
    for (uint16_t row=0;row<COMBAT_SCREEN_HEIGHT;++row) combatVideo.graphicsRowOffsets[row]=0; /*334E*/
    for (uint16_t row=0;row<0x15C;++row) combatVideo.herculesDisplayRowOffsets[row]=0; /*34DE*/
    for (uint16_t slot=0;slot<COMBAT_PALETTE_SLOTS;++slot)
        for (uint16_t colour=0;colour<COMBAT_PALETTE_WORDS;++colour)
        {
            uint16_t offset=(slot*COMBAT_PALETTE_WORDS+colour)*2;
            combatVideo.palettePrimarySlots[slot][colour]=0; /*37B4*/
            combatVideo.paletteSecondarySlots[slot][colour]=0; /*3A34*/
            combatVideo.graphicsColourPatterns[slot*COMBAT_PALETTE_WORDS+colour]=0; /*3CB4*/
        }
    return true;
}
