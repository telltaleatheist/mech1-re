#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_installation.h"
#include "mechwarrior/native_address.h"
#include "mechwarrior/combat_input.h"
#include "mechwarrior/combat_damage.h"
#include "mechwarrior/combat_cockpit.h"
CombatInputState combatInput;
CombatStartupState combatStartup;
CombatDamagePanelState combatDamagePanels;

/* Fresh-child installation IO, not a replacement runtime reset. In particular
 * the saved-adapter FF sentinel must survive errors before video22D8 runs. */
bool BtechImportStartupInitialData(const uint8_t *data,size_t bytes)
{
    if (!data || bytes<0x10000) return false;
    combatInput.terminationFlag=0; /*0050*/
    combatStartup.aiUpdatesSuppressed=data[0x0EBA];combatStartup.unidentifiedField22=data[0x0EBB];
    combatInput.combatInputReplayCancelLatched=0; /*08AA*/
    for (uint16_t group=0;group<2;++group) combatInput.heldDirectionKeyMasks[group]=data[0x4278+group];
    for (uint16_t mask=0;mask<sizeof(combatInput.heldDirectionMasks);++mask) combatInput.heldDirectionMasks[mask]=data[0x43E3+mask];
    combatStartup.criticalErrorHandlerInstalled=data[0x2860];
    combatStartup.keyboardHandlersInstalled=data[0x4274];
    combatStartup.mouseDriverInstalled=data[0x41E2];
    combatStartup.divideErrorHookMarkedActive=data[0x4162];
    combatStartup.videoAdapter=data[0x3FC4];
    combatStartup.savedDivideErrorOwner=MwDivideOwner::System; /*4165: 0000:0000*/
    combatStartup.pitChannel0DivisorShadow=MwReadNativeWord(data,0x41E7);
    combatStartup.schedulerPeriod=0; /*41E9*/combatStartup.schedulerCountdown=0; /*41EB*/
    combatStartup.schedulerActiveSlots=0; /*41ED*/
    for (uint16_t slot=0;slot<COMBAT_SCHEDULER_SLOTS;++slot)
    {
        /* Fresh EXEC only;2540 deliberately leaves these stale on reinstall. */
        if (MwReadNativeWord(data,0x41EF+slot*4) || MwReadNativeWord(data,0x41F1+slot*4)) return false;
        combatStartup.schedulerCallbacks[slot]=NULL;
        combatStartup.schedulerTimings[slot].ticksRemaining=0; /*422F*/
        combatStartup.schedulerTimings[slot].period=0; /*4231*/
    }
    combatStartup.keyboardCapsLockRequested=data[0x4275];combatStartup.keyboardBiosIntercept=data[0x4403];
    combatInput.keyboardLastKey=0; /*4276*/
    /* Not in the image: the BIOS key ring and shift flags, empty and zero at
     * each combat's start as the port's BIOS data was (H2). */
    for (uint16_t slot=0;slot<COMBAT_KEY_QUEUE_SLOTS;++slot) combatInput.combatKeyQueue[slot]=0;
    combatInput.combatKeyQueueHead=combatInput.combatKeyQueueTail=0;combatInput.keyboardShiftFlags=0;
    for (uint16_t scan=0;scan<COMBAT_KEYBOARD_SCAN_CODES;++scan)
    {combatInput.keyboardUnshiftedCharacters[scan]=data[0x42C2+scan];combatInput.keyboardShiftedCharacters[scan]=data[0x431B+scan];}
    for (uint16_t scan=0;scan<COMBAT_KEYBOARD_COMPATIBILITY_CODES;++scan)
    {combatInput.keyboardCompatibilityScans[scan]=data[0x43ED+scan];combatInput.keyboardCompatibilityReplacements[scan]=data[0x43F8+scan];}
    for (uint16_t scan=0;scan<sizeof(combatStartup.keyboardScanTranslations);++scan) combatStartup.keyboardScanTranslations[scan]=data[0x4374+scan];
    return true;
}

/* EXEC/DFCE data projection only, not206D or a new game procedure. Clear
 * converted owners in the native4EDC..D5DF BSS interval; reload authored
 * scalars separately. Host bindings and campaign owners are untouched. */
void BtechInitializeFreshStartupOwners(const uint8_t *data)
{
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
    selectedObjectCount=0;
    for (uint16_t object=0;object<COMBAT_SELECTED_WORLD_OBJECTS;++object) selectedWorldObjects[object]=NULL;
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
    combatStartup.radarRangeShift=MwReadNativeWord(data,0x0E20);
    combatStartup.viewScaleShift=MwReadNativeWord(data,0x0E22);
    combatStartup.requestedPlayerForwardStepScale=0; /*0EB4*/
    combatStartup.unidentifiedField21=0; /*0EB2*/
    combatStartup.mountGroupMode=data[0x0E26];
    combatStartup.headingRadarSelected=data[0x0E1E];
    combatStartup.combatDispatchState=0; /*0E1C*/
    combatStartup.firingIndicatorBank0Pending=data[0x0EAF];
    combatStartup.firingIndicatorBank1Pending=data[0x0EAE];
    combatStartup.selectedHitObject=NULL;
}
