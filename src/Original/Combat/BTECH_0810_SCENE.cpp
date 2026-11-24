#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_pose.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_scene.h"
#include "mechwarrior/combat_render.h"
#include <stdlib.h>

/*0810:02D1. Native pose, world ordering, aim/probe and CF5C object rendering.
 * Selection pixels restore and0886 removes temporary Mech body headings.
 * HUD retains its documented gates; final text/page handling is native. */
void BtechRenderCockpitSceneAndHud(void)
{
    if (!playerUnitBinding) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerUnitBinding");
    BtechPooledWorldObject *torso=playerUnitBinding->torso;
    BtechPooledWorldObject *positionedBody=playerUnitBinding->positionedLeftLeg;
    if (!playerPoseCameraContext) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerPoseCameraContext");
    if (combatStartup.playerImpactThisTick && playerPoseCameraContext->fillPaletteA!=COMBAT_COCKPIT_IMPACT_COLOUR)
        playerPoseCameraContext->fillPaletteA=COMBAT_COCKPIT_IMPACT_COLOUR;
    else
    {
        if (combatVideo.playerCameraFillASuppressed) playerPoseCameraContext->fillPaletteA=0;
        else
        {
            if (combatImport.combatLandscapeIndex>=COMBAT_LANDSCAPES || combatImport.hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: combatLandscapeIndex>=COMBAT_LANDSCAPES || hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES");
            playerPoseCameraContext->fillPaletteA=combatVideo.landscapeCameraColourRows[combatImport.hudColourSchemeIndex][combatImport.combatLandscapeIndex].playerFillPaletteA;
        }
    }
    BtechApplyBodyHeadingToMechParts();
    if (!torso) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !torso");
    BtechUpdatePlayerMechPose(&torso->object.prefix);
    combatVideo.drawPolygonFilled=true;
    cockpitWorldObjectCount=(uint16_t)BtechFilterNearbyWorldObjects(playerPoseCameraContext,worldObjects,
        (int16_t)worldObjectCount,cockpitWorldObjects,(int16_t)cockpitWorldObjectCount);
    BtechSortFarWorldObjectsForView(playerPoseCameraContext,cockpitWorldObjects,
        (int16_t)cockpitWorldObjectCount,&combatScene.objectSortOrSelectionScratch);
    BtechOrderMechRenderParts(playerPoseCameraContext,cockpitWorldObjects,(int16_t)cockpitWorldObjectCount);
    if (!playerMechState) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerMechState");
    uint32_t aimCosine=(int16_t)BtechLookupCosine(playerMechState->aimPitch);
    uint32_t aimSine=(int16_t)BtechLookupSine(playerMechState->aimPitch);
    uint32_t projectedAim=BtechDivideSignedDword(BtechShiftDwordLeft(aimSine,(uint8_t)combatStartup.viewScaleShift),aimCosine);
    uint16_t viewportYSum=playerPoseCameraContext->clipMaximumB+playerPoseCameraContext->clipMinimumB;
    combatStartup.aimReticleScreenY=(uint16_t)((uint16_t)projectedAim+(uint16_t)((int16_t)viewportYSum>>1));
    combatVideo.drawPageSelector=combatVideo.backPageSelector;
    uint16_t probeY=BtechRandomSignedSpread(combatStartup.heatAimJitterTiers[0])+combatStartup.aimReticleScreenY;
    uint16_t probeX=BtechRandomSignedSpread(combatStartup.heatAimJitterTiers[0])+COMBAT_SCREEN_WIDTH/2;
    BtechBeginSelectionPixelProbe(probeX,probeY);
    BtechRenderObjectList(playerPoseCameraContext,cockpitWorldObjects,(int16_t)cockpitWorldObjectCount);
    BtechRemoveBodyHeadingFromMechParts();
    if (!positionedBody) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !positionedBody");
    BtechDrawCockpitHeadingStrip(positionedBody->object.prefix.orientationC,(int16_t)torso->object.prefix.orientationC);
    BtechRefreshCockpitAndSensorTarget();
    BtechDrawPendingFiringIndicator();
    BtechDrawCockpitBitmapPlacements();
    combatVideo.textTransparentBackground=true;
    combatVideo.textForegroundColour=combatVideo.landscapeCameraColourRows[combatImport.hudColourSchemeIndex][combatImport.combatLandscapeIndex].hudReticleColour;
    uint16_t zoomX=COCKPIT_RIGHT_LABEL_X,automaticWeaponsX=COCKPIT_LEFT_LABEL_X;
    if (playerMechState->templateIndex>=COMBAT_MECH_TEMPLATES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: playerMechState->templateIndex>=COMBAT_MECH_TEMPLATES");
    if (combatCockpit.mechCockpitLayoutIndices[playerMechState->templateIndex]==2)
    {zoomX=COCKPIT_LEFT_LABEL_X;automaticWeaponsX=COCKPIT_RIGHT_LABEL_X;}
    BtechDrawNearText(combatCockpit.zoomLabel,zoomX,COCKPIT_STATUS_TEXT_Y);
    BtechDrawNearText(BtechFormatSignedDecimalField((int16_t)(combatStartup.viewScaleShift-COCKPIT_ZOOM_SHIFT_BASE),1,' '),zoomX+COCKPIT_ZOOM_VALUE_X_OFFSET,COCKPIT_STATUS_TEXT_Y);
    if (combatStartup.mountGroupMode) BtechDrawNearText(combatCockpit.automaticWeaponsLabel,automaticWeaponsX,COCKPIT_STATUS_TEXT_Y);
    if (combatMechs.heatInhibitFlags[0] && (combatStartup.combatTickCounter&COCKPIT_WARNING_FLASH_MASK))
    {
        combatVideo.textForegroundColour=COCKPIT_REACTOR_WARNING_COLOUR;
        BtechDrawNearText(combatCockpit.reactorShutdownText,COCKPIT_REACTOR_WARNING_X,COCKPIT_REACTOR_WARNING_Y);
    }
    combatVideo.textTransparentBackground=false;
    BtechSwapScreenPages(false);
    if (combatCockpit.cockpitFullRedrawPending)
    {
        BtechSetOverscanColour(Combat_Pal_EGA_Black);
        BtechPresentScreenRectangle(0,0,COMBAT_SCREEN_WIDTH,COMBAT_SCREEN_HEIGHT);
        combatCockpit.cockpitFullRedrawPending=0;
        combatVideo.drawPageSelector=combatVideo.backPageSelector;combatVideo.copySourcePageSelector=combatVideo.frontPageSelector;
        BtechCopyScreenRectangle(0,0,COMBAT_SCREEN_WIDTH,COMBAT_SCREEN_HEIGHT);
    }
    else BtechPresentScreenRectangle(playerPoseCameraContext->clipMinimumA,playerPoseCameraContext->clipMinimumB,
        (uint16_t)(playerPoseCameraContext->clipMaximumA-playerPoseCameraContext->clipMinimumA),
        (uint16_t)(playerPoseCameraContext->clipMaximumB-playerPoseCameraContext->clipMinimumB+1));
    combatVideo.drawPolygonFilled=true;combatVideo.screenPolygonNeedsClip=true;
}
