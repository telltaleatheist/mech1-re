#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_installation.h"
#include "mechwarrior/native_address.h"
#include "mechwarrior/combat_input.h"
#include "mechwarrior/combat_damage.h"
#include "mechwarrior/combat_cockpit.h"

uint16_t terminationFlag,combatInputScanCode,combatInputDirectionMask;
uint8_t aiUpdatesSuppressed,unidentifiedField22;
uint16_t combatInputButton1State,combatInputButton0State,combatInputReplayCancelLatched;
uint8_t heldDirectionKeyMasks[2],heldDirectionMasks[0x10];
uint16_t fireCycleStartMount,requestedTorsoAngle,previousAimButtonState;
uint16_t previousFireButtonState,fireCycleActive,requestedAimPitch,unidentifiedField24;

uint16_t combatTickCounter,playerTargetSlot,selectedWeaponMount,aimReticleScreenY;
int16_t heatAimJitterTiers[COMBAT_MECH_SLOTS];
uint8_t targetDamagePanelDirty[COMBAT_MECH_SLOTS],specialTargetDamagePanelDirty;
uint16_t radarRangeShift,viewScaleShift,requestedPlayerForwardStepScale,unidentifiedField21;
uint16_t playerSpecialTargetLandingLatch;
uint16_t playerMountFireFlags[COMBAT_WEAPON_MOUNTS];
uint8_t mountGroupMode,headingRadarSelected,playerImpactThisTick;
uint16_t combatDispatchState;
uint8_t firingIndicatorBank0Pending,firingIndicatorBank1Pending,sideCompletionFlags[2];
BtechPooledWorldObject *unitLastDrawParts[COMBAT_MECH_SLOTS];
BtechWorldObjectPrefix *selectedHitObject;
uint8_t criticalErrorHandlerInstalled,keyboardHandlersInstalled,mouseDriverInstalled;
uint8_t divideErrorHookMarkedActive,videoAdapter;
MwDivideOwner savedDivideErrorOwner;
uint16_t pitChannel0DivisorShadow;
uint16_t schedulerPeriod,schedulerCountdown,schedulerActiveSlots;
BtechTimerCallback schedulerCallbacks[COMBAT_SCHEDULER_SLOTS];
BtechSchedulerTiming schedulerTimings[COMBAT_SCHEDULER_SLOTS];
uint8_t keyboardCapsLockRequested,keyboardBiosIntercept,keyboardScanTranslations[COMBAT_KEYBOARD_SCAN_CODES];
uint16_t keyboardLastKey;
uint16_t combatKeyQueue[COMBAT_KEY_QUEUE_SLOTS];
uint8_t combatKeyQueueHead,combatKeyQueueTail,keyboardShiftFlags;
uint8_t keyboardUnshiftedCharacters[COMBAT_KEYBOARD_SCAN_CODES],keyboardShiftedCharacters[COMBAT_KEYBOARD_SCAN_CODES];
uint8_t keyboardCompatibilityScans[COMBAT_KEYBOARD_COMPATIBILITY_CODES],keyboardCompatibilityReplacements[COMBAT_KEYBOARD_COMPATIBILITY_CODES];

/* Fresh-child installation IO, not a replacement runtime reset. In particular
 * the saved-adapter FF sentinel must survive errors before video22D8 runs. */
bool BtechImportStartupInitialData(const uint8_t *data,size_t bytes)
{
    if (!data || bytes<0x10000) return false;
    terminationFlag=0; /*0050*/
    aiUpdatesSuppressed=data[0x0EBA];unidentifiedField22=data[0x0EBB];
    combatInputReplayCancelLatched=0; /*08AA*/
    for (uint16_t group=0;group<2;++group) heldDirectionKeyMasks[group]=data[0x4278+group];
    for (uint16_t mask=0;mask<sizeof(heldDirectionMasks);++mask) heldDirectionMasks[mask]=data[0x43E3+mask];
    criticalErrorHandlerInstalled=data[0x2860];
    keyboardHandlersInstalled=data[0x4274];
    mouseDriverInstalled=data[0x41E2];
    divideErrorHookMarkedActive=data[0x4162];
    videoAdapter=data[0x3FC4];
    savedDivideErrorOwner=MwDivideOwner::System; /*4165: 0000:0000*/
    pitChannel0DivisorShadow=MwReadNativeWord(data,0x41E7);
    schedulerPeriod=0; /*41E9*/schedulerCountdown=0; /*41EB*/
    schedulerActiveSlots=0; /*41ED*/
    for (uint16_t slot=0;slot<COMBAT_SCHEDULER_SLOTS;++slot)
    {
        /* Fresh EXEC only;2540 deliberately leaves these stale on reinstall. */
        if (MwReadNativeWord(data,0x41EF+slot*4) || MwReadNativeWord(data,0x41F1+slot*4)) return false;
        schedulerCallbacks[slot]=NULL;
        schedulerTimings[slot].ticksRemaining=0; /*422F*/
        schedulerTimings[slot].period=0; /*4231*/
    }
    keyboardCapsLockRequested=data[0x4275];keyboardBiosIntercept=data[0x4403];
    keyboardLastKey=0; /*4276*/
    /* Not in the image: the BIOS key ring and shift flags, empty and zero at
     * each combat's start as the port's BIOS data was (H2). */
    for (uint16_t slot=0;slot<COMBAT_KEY_QUEUE_SLOTS;++slot) combatKeyQueue[slot]=0;
    combatKeyQueueHead=combatKeyQueueTail=0;keyboardShiftFlags=0;
    for (uint16_t scan=0;scan<COMBAT_KEYBOARD_SCAN_CODES;++scan)
    {keyboardUnshiftedCharacters[scan]=data[0x42C2+scan];keyboardShiftedCharacters[scan]=data[0x431B+scan];}
    for (uint16_t scan=0;scan<COMBAT_KEYBOARD_COMPATIBILITY_CODES;++scan)
    {keyboardCompatibilityScans[scan]=data[0x43ED+scan];keyboardCompatibilityReplacements[scan]=data[0x43F8+scan];}
    for (uint16_t scan=0;scan<sizeof(keyboardScanTranslations);++scan) keyboardScanTranslations[scan]=data[0x4374+scan];
    return true;
}

/* EXEC/DFCE data projection only, not206D or a new game procedure. Clear
 * converted owners in the native4EDC..D5DF BSS interval; reload authored
 * scalars separately. Host bindings and campaign owners are untouched. */
void BtechInitializeFreshStartupOwners(const uint8_t *data)
{
    fireCycleStartMount=requestedTorsoAngle=previousAimButtonState=0;
    for (uint16_t i=0;i<COMBAT_DECIMAL_FIELD_BYTES;++i) decimalFieldText[i]=0;
    for (uint16_t page=0;page<COMBAT_RADAR_CACHE_PAGES;++page)
    {
        radarMarkerCounts[page]=0;
        for (uint16_t marker=0;marker<COMBAT_RADAR_MARKERS;++marker)
            radarMarkerX[marker][page]=radarMarkerY[marker][page]=radarMarkerBackground[marker][page]=0;
    }
    radarPlayerObject=NULL;radarRectangle=NULL;
    radarCentreX=radarCentreY=radarCachePage=radarMapOriginX=radarMapOriginY=0;
    previousFireButtonState=fireCycleActive=requestedAimPitch=unidentifiedField24=0;
    combatInputScanCode=combatInputDirectionMask=combatInputButton1State=combatInputButton0State=0;
    selectedObjectCount=0;
    for (uint16_t object=0;object<COMBAT_SELECTED_WORLD_OBJECTS;++object) selectedWorldObjects[object]=NULL;
    for (uint16_t mount=0;mount<COMBAT_WEAPON_MOUNTS;++mount) playerMountFireFlags[mount]=0;
    for (uint16_t slot=0;slot<COMBAT_MECH_TEMPLATES;++slot) templateMaximumCombatScores[slot]=0;
    for (uint16_t observer=0;observer<COMBAT_MECH_SLOTS;++observer)
        for (uint16_t target=0;target<COMBAT_MECH_SLOTS;++target) unitVisibility[observer][target]=0;
    combatTickCounter=0;
    playerTargetSlot=0;
    aimReticleScreenY=0;
    playerSpecialTargetLandingLatch=0;
    playerImpactThisTick=0;
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot) targetDamagePanelDirty[slot]=0;
    specialTargetDamagePanelDirty=0;
    for (uint16_t slot=0;slot<2;++slot)
        sideCompletionFlags[slot]=0;
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {unitLastDrawParts[slot]=NULL;heatAimJitterTiers[slot]=0;heatSpeedPenalties[slot]=0;heatInhibitFlags[slot]=0;}
    selectedWeaponMount=0; /*0E24*/
    radarRangeShift=MwReadNativeWord(data,0x0E20);
    viewScaleShift=MwReadNativeWord(data,0x0E22);
    requestedPlayerForwardStepScale=0; /*0EB4*/
    unidentifiedField21=0; /*0EB2*/
    mountGroupMode=data[0x0E26];
    headingRadarSelected=data[0x0E1E];
    combatDispatchState=0; /*0E1C*/
    firingIndicatorBank0Pending=data[0x0EAF];
    firingIndicatorBank1Pending=data[0x0EAE];
    selectedHitObject=NULL;
}
