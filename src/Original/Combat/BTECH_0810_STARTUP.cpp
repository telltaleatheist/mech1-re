#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_orders.h"
#include "mechwarrior/combat_cockpit.h"
#include <stdlib.h>

/*0810:1E84. Original EGA startup through briefing wait and cockpit setup.
 * PSP:005D/005F's resident reference is already
 * bound to campaignCombatExchange_0004 by the existing launcher transport.
 * No fake PSP, source arguments or alternative startup loop is introduced. */
void BtechInitializeCombatRuntime(void)
{
    BtechInstallDosCriticalErrorHandler();
    combatImport.combatContinuationStage=UINT16_MAX;
    BtechResetCombatState();
    BtechInitializeVideoAndInput();
    combatImport.hudColourSchemeIndex=combatVideo.combatRendererKind==1;
    if (combatImport.hudColourSchemeIndex || combatVideo.combatRendererKind==3) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: hudColourSchemeIndex || combatRendererKind==3"); /* Non-EGA scratch ownership not converted. */
    desertPaletteSlot=BtechLoadPaletteSlot("desert.pal",NULL);
    arcticPaletteSlot=BtechLoadPaletteSlot("arctic.pal",NULL);
    tropicalPaletteSlot=BtechLoadPaletteSlot("tropic.pal",NULL);
    combatVideo.damagePaletteSlot=BtechLoadPaletteSlot("dmgpal.pal",NULL);
    BtechSelectFontSlot((int16_t)BtechLoadFontSlot("6x6b.fnt",NULL));
    BtechSelectPaletteSlot((int16_t)arcticPaletteSlot);
    combatVideo.promptBitmaps=BtechLoadBitmapList("prompt.bmp",NULL);
    BtechSelectAndInstallAudioBackends(0);
    BtechRegisterTimerCallback(BtechSelectTandyOrFallbackAudioCallback(),1);
    uint16_t inheritedSoundStreamIdentity;
    uint16_t soundBank=BtechLoadSoundBank("btechsnd.snd",NULL,&inheritedSoundStreamIdentity);
    if (!soundBank) BtechShowFatalErrorAndExit("the combat sound bank could not be loaded");
    else BtechSelectActiveSoundBank(soundBank);
    BtechPrepareCampaignCombatStage(inheritedSoundStreamIdentity);
    BtechInitializeUnitControlRecords();
    selectedObjectCount=0;
    for (uint16_t object=0;object<COMBAT_WORLD_OBJECTS;++object)
    {
        BtechWorldObjectPrefix *registered=worldObjects[object];
        if (!registered) break;
        if ((int16_t)registered->resourceId<(int16_t)combatResources.surfaceResourceBank.resourceCount
            && (int16_t)registered->resourceId>=0)
        {
            if (selectedObjectCount>=COMBAT_SELECTED_WORLD_OBJECTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: selectedObjectCount>=COMBAT_SELECTED_WORLD_OBJECTS"); /* Next native store aliases7A2E. */
            selectedWorldObjects[selectedObjectCount++]=worldObjects[object];
        }
    }
    combatVideo.playerCameraFillASuppressed=BtechRandomBelowBound(3)<1;
    BtechApplyLandscapeCameraColours();
    BtechLoadCockpitHudResources();
    BtechInitializeOrderPanelState();
    BtechResetMechInspectionState();
    if (!playerMechTemplate) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerMechTemplate");
    for (int16_t mount=0;mount<(int16_t)playerMechTemplate->weaponMountCount;++mount)
    {
        if (mount>=COMBAT_WEAPON_MOUNTS || !playerMechState) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mount>=COMBAT_WEAPON_MOUNTS || !playerMechState");
        combatStartup.playerMountFireFlags[mount]=(int8_t)playerMechState->weapons[mount].operational;
    }
    combatImport.activeUnitControl=&unitControls[0];
    BtechSynchronizeMechRenderParts();
    BtechShowCombatStageBriefing();
    BtechRefreshCockpitViewportAndGauges();
}

/*0810:22D8. Unknown first-argument characters leaveBP-2 uninitialized in the
 * original; there is deliberately no invented auto-detect default here. */
void BtechInitializeVideoAndInput(void)
{
    uint16_t adapterOverride=COMBAT_VIDEO_AUTO;
    if ((int16_t)combatImport.savedArgumentCount>1)
    {
        uint8_t option=combatImport.savedArguments[1][0];
        /*EAA0 lowercases ASCII upper-case in the shipped character table. */
        if (option>='A' && option<='Z') option+='a'-'A';
        switch (option)
        {
        case 'c':adapterOverride=1;break;
        case 'e':adapterOverride=2;break;
        case 't':adapterOverride=3;break;
        default:/* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Fatal path under condition: (int16_t)savedArgumentCount>1"); /* Unrecovered native stack residue, not an argument. */
        }
    }
    if (!BtechSelectCombatRenderer((uint8_t)adapterOverride,COMBAT_VIDEO_ALLOWED))
        BtechShowFatalErrorAndExit(errorCardNotFound);
    /* The native failure call falls through here if it unexpectedly returns. */
    combatVideo.drawPolygonFilled=true;combatVideo.textTransparentBackground=false;combatVideo.egaBitmapEnabledPlanes=COMBAT_EGA_PLANE_MASK_ALL;
    BtechInstallKeyboardAndTickHandlers();
    BtechInstallDivideErrorHandler();
    BtechInstallTimerScheduler(COMBAT_SCHEDULER_PERIOD);
    BtechResetCameraContextsAndReferenceVector();
}

/*0810:DE52. Fatal is NOT1337: no delay, sound loop or critical-vector restore.
 *02C2 draws to the existing draw page; no invented flip/present is inserted. */
void BtechShowFatalErrorAndExit(const char *errorText)
{
    combatVideo.textForegroundColour=Combat_Pal_EGA_White;combatVideo.textBackgroundColour=Combat_Pal_EGA_Black;combatVideo.screenPolygonNeedsClip=false;
    BtechDrawNearText(errorText,COMBAT_FATAL_TEXT_X,COMBAT_FATAL_TEXT_Y);
    BtechRestoreTextVideoMode();
    BtechShutdownPlatformHandlers();
    BtechRuntimeExitAndCloseHandles(1);
}

/*0810:1337. Normal exit differs fromDE52's text-first fatal path. */
void BtechTerminateToLauncher(uint16_t quitWholeGame)
{
    BtechWaitScaledRuntimeDelay(COMBAT_EXIT_DELAY_UNITS);
    for (uint16_t soundEvent=0;soundEvent<COMBAT_SHUTDOWN_SOUND_EVENTS;++soundEvent) BtechStopSoundEventAndTimer(soundEvent);
    BtechRestoreDosCriticalErrorHandler();
    BtechShutdownPlatformHandlers();
    BtechRestoreTextVideoMode();
    BtechRuntimeExitAndCloseHandles(quitWholeGame<1);
}

/*0810:206D. Invalidate ONLY the native state and positioned-left-leg entries;
 * this is not memset(bindings).7752 and remaining part links stay untouched. */
void BtechResetCombatState(void)
{
    combatStartup.combatTickCounter=0;
    combatStartup.mountGroupMode=1;
    combatStartup.headingRadarSelected=0;
    combatStartup.combatDispatchState=COMBAT_SCREEN_COCKPIT;
    combatStartup.playerImpactThisTick=0;
    combatStartup.playerTargetSlot=0;
    combatStartup.selectedWeaponMount=0;
    combatStartup.aimReticleScreenY=0;
    combatStartup.headingRadarSelected=0;
    combatStartup.radarRangeShift=0x0A;
    combatStartup.viewScaleShift=7;
    combatStartup.firingIndicatorBank0Pending=0;
    combatStartup.firingIndicatorBank1Pending=0;
    combatStartup.requestedPlayerForwardStepScale=0;
    combatStartup.unidentifiedField21=0;
    combatStartup.selectedHitObject=NULL;
    combatStartup.sideCompletionFlags[1]=0;combatStartup.sideCompletionFlags[0]=0;
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot) combatStartup.unitLastDrawParts[slot]=NULL;
    for (uint16_t chassis=0;chassis<COMBAT_MECH_TEMPLATES;++chassis) mechResourceBaseIds[chassis]=0;
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {unitBindings[slot].mech=NULL;unitBindings[slot].positionedLeftLeg=NULL;}
    combatImport.specialTargetInteractionBlocked=0;
    otherShapeResourceBase=0;firstMechShapeResourceId=0;
    combatStartup.playerSpecialTargetLandingLatch=0;
}

/*0810:CE9E. The flag is written where DOS's INT 24 query and install were.
 * The host raises no DOS critical error, so the hook never ran: the vector
 * save and install are gone (H4c). */
void BtechInstallDosCriticalErrorHandler(void)
{
    combatStartup.criticalErrorHandlerInstalled=1;
}

/*0810:CED5. No installed-flag test; the INT 24 restore went with the hook (H4c). */
void BtechRestoreDosCriticalErrorHandler(void)
{
    combatStartup.criticalErrorHandlerInstalled=0;
}
