#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_installation.h"
#include "mechwarrior/btech_start_image.h"
#include "mechwarrior/combat_input.h"
#include "mechwarrior/combat_damage.h"
#include "mechwarrior/combat_cockpit.h"
CombatInputState combatInput;
CombatStartupState combatStartup;
CombatDamagePanelState combatDamagePanels;

/* Fresh-child installation IO, not a replacement runtime reset. In particular
 * the saved-adapter FF sentinel must survive errors before video22D8 runs. */
bool BtechImportStartupInitialData(void)
{
    const BtechStartRowData combatAiFlagsRow("combat.ai_flags");
    const BtechStartRowData startupCriticalErrorHandlerRow("startup.critical_error_handler");
    const BtechStartRowData videoAdapterRow("video.adapter");
    const BtechStartRowData startupDivideHookRow("startup.divide_hook");
    const BtechStartRowData startupMouseAndDelayRow("startup.mouse_and_delay");
    const BtechStartRowData startupPitDivisorRow("startup.pit_divisor");
    const BtechStartRowData startupSchedulerRow("startup.scheduler");
    const BtechStartRowData keyboardStateRow("keyboard.state");
    const BtechStartRowData keyboardCharactersRow("keyboard.characters");
    const BtechStartRowData keyboardScanTranslationsRow("keyboard.scan_translations");
    const BtechStartRowData keyboardDirectionAndCompatibilityRow("keyboard.direction_and_compatibility");
    combatInput.terminationFlag=0; /*0050*/
    combatStartup.aiUpdatesSuppressed=combatAiFlagsRow.byte();combatStartup.unidentifiedField22=combatAiFlagsRow.byte(1);
    combatInput.combatInputReplayCancelLatched=0; /*08AA*/
    for (uint16_t group=0;group<2;++group) combatInput.heldDirectionKeyMasks[group]=keyboardStateRow.byte(4+group);
    for (uint16_t mask=0;mask<sizeof(combatInput.heldDirectionMasks);++mask) combatInput.heldDirectionMasks[mask]=keyboardDirectionAndCompatibilityRow.byte(mask);
    combatStartup.criticalErrorHandlerInstalled=startupCriticalErrorHandlerRow.byte();
    combatStartup.keyboardHandlersInstalled=keyboardStateRow.byte();
    combatStartup.mouseDriverInstalled=startupMouseAndDelayRow.byte();
    combatStartup.divideErrorHookMarkedActive=startupDivideHookRow.byte();
    combatStartup.videoAdapter=videoAdapterRow.byte();
    combatStartup.savedDivideErrorOwner=MwDivideOwner::System; /*4165: 0000:0000*/
    combatStartup.pitChannel0DivisorShadow=startupPitDivisorRow.word();
    combatStartup.schedulerPeriod=0; /*41E9*/combatStartup.schedulerCountdown=0; /*41EB*/
    combatStartup.schedulerActiveSlots=0; /*41ED*/
    for (uint16_t slot=0;slot<COMBAT_SCHEDULER_SLOTS;++slot)
    {
        /* Fresh EXEC only;2540 deliberately leaves these stale on reinstall. */
        if (startupSchedulerRow.word(slot*4) || startupSchedulerRow.word(2+slot*4)) return false;
        combatStartup.schedulerCallbacks[slot]=NULL;
        combatStartup.schedulerTimings[slot].ticksRemaining=0; /*422F*/
        combatStartup.schedulerTimings[slot].period=0; /*4231*/
    }
    combatStartup.keyboardCapsLockRequested=keyboardStateRow.byte(1);combatStartup.keyboardBiosIntercept=keyboardDirectionAndCompatibilityRow.byte(0x20);
    combatInput.keyboardLastKey=0; /*4276*/
    /* Not in the image: the BIOS key ring and shift flags, empty and zero at
     * each combat's start as the port's BIOS data was (H2). */
    for (uint16_t slot=0;slot<COMBAT_KEY_QUEUE_SLOTS;++slot) combatInput.combatKeyQueue[slot]=0;
    combatInput.combatKeyQueueHead=combatInput.combatKeyQueueTail=0;combatInput.keyboardShiftFlags=0;
    for (uint16_t scan=0;scan<COMBAT_KEYBOARD_SCAN_CODES;++scan)
    {combatInput.keyboardUnshiftedCharacters[scan]=keyboardCharactersRow.byte(scan);combatInput.keyboardShiftedCharacters[scan]=keyboardCharactersRow.byte(0x59+scan);}
    for (uint16_t scan=0;scan<COMBAT_KEYBOARD_COMPATIBILITY_CODES;++scan)
    {combatInput.keyboardCompatibilityScans[scan]=keyboardDirectionAndCompatibilityRow.byte(0xA+scan);combatInput.keyboardCompatibilityReplacements[scan]=keyboardDirectionAndCompatibilityRow.byte(0x15+scan);}
    for (uint16_t scan=0;scan<sizeof(combatStartup.keyboardScanTranslations);++scan) combatStartup.keyboardScanTranslations[scan]=keyboardScanTranslationsRow.byte(scan);
    return true;
}

/* EXEC/DFCE data projection only, not206D or a new game procedure. Clear
 * converted owners in the native4EDC..D5DF BSS interval; reload authored
 * scalars separately. Host bindings and campaign owners are untouched. */
void BtechInitializeFreshStartupOwners(void)
{
    const BtechStartRowData combatStartHeadingRadarSelectedRow("combat.start.heading_radar_selected");
    const BtechStartRowData combatStartViewShiftsRow("combat.start.view_shifts");
    const BtechStartRowData combatStartMountGroupModeRow("combat.start.mount_group_mode");
    const BtechStartRowData combatStartFiringIndicatorsPendingRow("combat.start.firing_indicators_pending");
    combatInput.fireCycleStartMount=combatInput.requestedTorsoAngle=combatInput.previousAimButtonState=0;
    for (uint16_t i=0;i<COMBAT_DECIMAL_FIELD_BYTES;++i) decimalFieldText[i]=0;
    for (uint16_t page=0;page<COMBAT_RADAR_CACHE_PAGES;++page)
    {
        combatCockpit.radarMarkerCounts[page]=0;
        for (uint16_t marker=0;marker<COMBAT_RADAR_MARKERS;++marker)
            combatCockpit.radarMarkerX[marker][page]=combatCockpit.radarMarkerY[marker][page]=combatCockpit.radarMarkerBackground[marker][page]=0;
    }
    combatCockpit.radarPlayerObject=NULL;combatCockpit.radarRectangle=NULL;
    combatCockpit.radarCentreX=combatCockpit.radarCentreY=combatCockpit.radarCachePage=combatCockpit.radarMapOriginX=combatCockpit.radarMapOriginY=0;
    combatInput.previousFireButtonState=combatInput.fireCycleActive=combatInput.requestedAimPitch=combatInput.unidentifiedField24=0;
    combatInput.combatInputScanCode=combatInput.combatInputDirectionMask=combatInput.combatInputButton1State=combatInput.combatInputButton0State=0;
    combatWorld.selectedObjectCount=0;
    for (uint16_t object=0;object<COMBAT_SELECTED_WORLD_OBJECTS;++object) combatWorld.selectedWorldObjects[object]=NULL;
    for (uint16_t mount=0;mount<COMBAT_WEAPON_MOUNTS;++mount) combatStartup.playerMountFireFlags[mount]=0;
    for (uint16_t slot=0;slot<COMBAT_MECH_TEMPLATES;++slot) combatMechs.templateMaximumCombatScores[slot]=0;
    for (uint16_t observer=0;observer<COMBAT_MECH_SLOTS;++observer)
        for (uint16_t target=0;target<COMBAT_MECH_SLOTS;++target) combatMechs.unitVisibility[observer][target]=0;
    combatStartup.combatTickCounter=0;
    combatStartup.playerTargetSlot=0;
    combatStartup.aimReticleScreenY=0;
    combatStartup.playerSpecialTargetLandingLatch=0;
    combatStartup.playerImpactThisTick=0;
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot) combatDamagePanels.targetDamagePanelDirty[slot]=0;
    combatDamagePanels.specialTargetDamagePanelDirty=0;
    for (uint16_t slot=0;slot<2;++slot)
        combatStartup.sideCompletionFlags[slot]=0;
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {combatStartup.unitLastDrawParts[slot]=NULL;combatStartup.heatAimJitterTiers[slot]=0;combatMechs.heatSpeedPenalties[slot]=0;combatMechs.heatInhibitFlags[slot]=0;}
    combatStartup.selectedWeaponMount=0; /*0E24*/
    combatStartup.radarRangeShift=combatStartViewShiftsRow.word();
    combatStartup.viewScaleShift=combatStartViewShiftsRow.word(2);
    combatStartup.requestedPlayerForwardStepScale=0; /*0EB4*/
    combatStartup.unidentifiedField21=0; /*0EB2*/
    combatStartup.mountGroupMode=combatStartMountGroupModeRow.byte();
    combatStartup.headingRadarSelected=combatStartHeadingRadarSelectedRow.byte();
    combatStartup.combatDispatchState=0; /*0E1C*/
    combatStartup.firingIndicatorBank0Pending=combatStartFiringIndicatorsPendingRow.byte(1);
    combatStartup.firingIndicatorBank1Pending=combatStartFiringIndicatorsPendingRow.byte();
    combatStartup.selectedHitObject=NULL;
}
