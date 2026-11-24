#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_world.h"
#include "mechwarrior/combat_cockpit.h"
#include <stdlib.h>

/*0810:2678. Asymmetric native bounds: Y includes y+height, X subtracts1. */
void BtechSetCockpitCameraViewport(BtechCameraContext *camera)
{
    if (combatImport.activeCockpitLayoutIndex>=COMBAT_COCKPIT_LAYOUTS || !camera) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: activeCockpitLayoutIndex>=COMBAT_COCKPIT_LAYOUTS || !camera");
    const BtechCockpitLayout *layout=&combatCockpit.cockpitLayouts[combatImport.activeCockpitLayoutIndex];
    camera->clipMinimumB=layout->viewport.y;
    camera->clipMaximumB=(int16_t)((uint16_t)layout->viewport.y+(uint16_t)layout->viewport.height);
    camera->clipMinimumA=layout->viewport.x;
    camera->clipMaximumA=(int16_t)((uint16_t)layout->viewport.width+(uint16_t)layout->viewport.x-1);
}

/*0810:6094. The third cache WORD has no proved consumer/meaning yet. */
void BtechResetHudGaugeCaches(void)
{
    combatCockpit.unidentifiedField25=0;
    combatCockpit.cachedHeatGaugeLevel=0;
    combatCockpit.cachedJumpChargeGaugeLevel=0;
}

/*0810:26C5. Ignore screen-loader result; restore only the saved clip BYTE.
 * Draw page, gauge caches, viewport and redraw latch retain native ordering. */
void BtechRefreshCockpitViewportAndGauges(void)
{
    uint8_t savedClipping=combatVideo.screenPolygonNeedsClip;
    if (!combatWorld.playerMechState || combatWorld.playerMechState->templateIndex>=COMBAT_MECH_TEMPLATES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerMechState || playerMechState->templateIndex>=COMBAT_MECH_TEMPLATES");
    combatImport.activeCockpitLayoutIndex=combatCockpit.mechCockpitLayoutIndices[combatWorld.playerMechState->templateIndex];
    combatVideo.screenPolygonNeedsClip=false;
    combatVideo.drawPage=combatVideo.backPage;
    if (combatImport.activeCockpitLayoutIndex>=COMBAT_COCKPIT_LAYOUTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: activeCockpitLayoutIndex>=COMBAT_COCKPIT_LAYOUTS");
    BtechLoadScreenImage(combatCockpit.cockpitScreenFilenames[combatImport.activeCockpitLayoutIndex],NULL);
    combatVideo.screenPolygonNeedsClip=savedClipping;
    BtechResetHudGaugeCaches();
    BtechSetCockpitCameraViewport(combatWorld.playerPoseCameraContext);
    combatCockpit.cockpitFullRedrawPending=1;
}

/*0810:2938. Genuine FAR veneer, no source arguments or BP result. */
void BtechRefreshCockpitViewportAndGaugesFar(void)
{
    BtechRefreshCockpitViewportAndGauges();
}

/*0810:139C. BYTE latch beside independent WORD; not a packed WORD store. */
void BtechResetMechInspectionState(void)
{
    combatVideo.inspectionReloadPending=1;
    combatVideo.inspectionSetupComplete=0;
}

/*0810:2716. Keep the first player store, then the shared map/player store,
 * and finally palette selection. Map fillA and neighbouring fields survive. */
void BtechApplyLandscapeCameraColours(void)
{
    if (combatImport.combatLandscapeIndex>=COMBAT_LANDSCAPES || combatImport.hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES
        || !combatWorld.playerPoseCameraContext || !combatWorld.orderMapCameraContext) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: combatLandscapeIndex>=COMBAT_LANDSCAPES || hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES || !playerPoseCameraContext || !orderMapCameraContext"); /* Unrecovered unrelated-owner aliases. */
    combatWorld.playerPoseCameraContext->fillPaletteA=combatVideo.playerCameraFillASuppressed?0:
        combatVideo.landscapeCameraColourRows[combatImport.hudColourSchemeIndex][combatImport.combatLandscapeIndex].playerFillPaletteA;
    combatWorld.playerPoseCameraContext->fillPaletteB=combatWorld.orderMapCameraContext->fillPaletteB=
        combatVideo.landscapeCameraColourRows[combatImport.hudColourSchemeIndex][combatImport.combatLandscapeIndex].sharedFillPaletteB;
    BtechSelectPaletteSlot(combatVideo.landscapePaletteSlots[combatImport.combatLandscapeIndex]);
}

/*0810:2372. Native five-bank loading order. EGA always uses HUD_NUMS;
 * non-EGA cyan HUD selection additionally requires landscape zero. */
void BtechLoadCockpitHudResources(void)
{
    combatVideo.pointPictures=BtechLoadBitmapList("point.bmp",NULL);
    combatVideo.smallMechPictures=BtechLoadBitmapList("sm_mechs.bmp",NULL);
    combatVideo.strutPictures=BtechLoadBitmapList("struts.bmp",NULL);
    combatVideo.cockpitPictures=BtechLoadBitmapList("cockpit.bmp",NULL);
    const char *filename=combatVideo.combatRendererKind!=COMBAT_VIDEO_EGA && combatImport.combatLandscapeIndex==0?"hud_cyan.bmp":"hud_nums.bmp";
    combatVideo.hudGlyphPictures=BtechLoadBitmapList(filename,NULL);
}

/*0810:23D1. Live handle reads across each release; clear in reverse order. */
void BtechUnloadCockpitHudResources(void)
{
    BtechFreeResourceIfLoaded(combatVideo.pointPictures);
    BtechFreeResourceIfLoaded(combatVideo.smallMechPictures);
    BtechFreeResourceIfLoaded(combatVideo.strutPictures);
    BtechFreeResourceIfLoaded(combatVideo.cockpitPictures);
    BtechFreeResourceIfLoaded(combatVideo.hudGlyphPictures);
    BtechFreeResourceIfLoaded(combatVideo.inspectionDamagePictures);
    combatVideo.inspectionDamagePictures=NULL;
    combatVideo.hudGlyphPictures=NULL;
    combatVideo.cockpitPictures=NULL;
    combatVideo.strutPictures=NULL;
    combatVideo.smallMechPictures=NULL;
    combatVideo.pointPictures=NULL;
}

/*0810:2422. The native nullable guard belongs here, not inside1336. */
void BtechFreeResourceIfLoaded(BtechBitmapList *list)
{
    if (list) BtechFreeRegisteredBitmapList(list);
}
