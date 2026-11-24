#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_input.h"
#include "mechwarrior/combat_video.h"
#include <stdlib.h>

/*1BF5:24D3. First clear occupancy bit, not first NULL callback. Publish the
 * timing/callback before the mask update (the original's CLI/STI around
 * that update are gone, H4c). */
uint16_t BtechRegisterTimerCallback(BtechTimerCallback callback,uint16_t period)
{
    if (!runtimeDelayActive || combatStartup.schedulerActiveSlots==UINT16_MAX) return 0;
    uint16_t slot=0,slotMask=1,pendingSlots=combatStartup.schedulerActiveSlots;
    while (pendingSlots&1) {++slot;slotMask<<=1;pendingSlots>>=1;}
    combatStartup.schedulerTimings[slot].period=period;
    combatStartup.schedulerTimings[slot].ticksRemaining=period;
    combatStartup.schedulerCallbacks[slot]=callback;
    combatStartup.schedulerActiveSlots|=slotMask;
    return slot+1;
}

/*1BF5:251D. Native DEC/TEST CL,F0 examines CL only; handles with nonzero
 * high BYTE alias1..16. Removal leaves callback and both timing WORDs stale. */
uint16_t BtechUnregisterTimerCallback(uint16_t callbackHandle)
{
    --callbackHandle;
    if ((uint8_t)callbackHandle&COMBAT_SCHEDULER_INVALID_SLOT_BITS) return 0;
    combatStartup.schedulerActiveSlots&=(uint16_t)~(1u<<(uint8_t)callbackHandle);
    return 1;
}

/*1BF5:25E2. IRQ0 selects304E, snapshots occupancy once, scans low-to-high.
 * Typed callbacks replace FAR pairs, not native scheduling decisions. Bound
 * callbacks must preserve the original scheduler's SI/BP/DS register owners. */
void BtechTimerSchedulerInterrupt(void)
{
    uint16_t remaining=runtimeDelayRemaining-1;
    runtimeDelayRemaining=remaining^((remaining&NATIVE_WORD_SIGN_BIT)?UINT16_MAX:0);
    uint16_t pendingSlots=combatStartup.schedulerActiveSlots;
    for (uint16_t slot=0;pendingSlots;++slot,pendingSlots>>=1)
    {
        if (!(pendingSlots&1)) continue;
        remaining=combatStartup.schedulerTimings[slot].ticksRemaining-1;
        if (!remaining)
        {
            if (!combatStartup.schedulerCallbacks[slot]) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !schedulerCallbacks[slot]"); /* Native NULL FAR fault. */
            combatStartup.schedulerCallbacks[slot]();
            remaining=combatStartup.schedulerTimings[slot].period; /* Live AFTER callback. */
        }
        combatStartup.schedulerTimings[slot].ticksRemaining=remaining;
    }
    remaining=combatStartup.schedulerCountdown-1;
    if (remaining) combatStartup.schedulerCountdown=remaining; /* Its EOI is gone (H4c). */
    else
    {
        /* The original chained to IRQ0's saved owner here, every
         * schedulerPeriod-th tick (13 in combat: 13 x 5041 = 65533 PIT
         * cycles, the BIOS's 18.2 Hz). The host runs that owner's tick (H4c):
         * the campaign's timer handler, whose slot 0 is the BIOS tick. Only
         * the previous owner was ever saved; combat's music timer saved
         * under the scheduler is not converted. */
        combatStartup.schedulerCountdown=combatStartup.schedulerPeriod;
        if (combatHandlers.savedTimerOwner!=MwTimerOwner::Previous) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: savedTimerOwner!=MwTimerOwner::Previous");
        if (!btechHostRunPreviousTimerOwner) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostRunPreviousTimerOwner");
        btechHostRunPreviousTimerOwner();
    }
}

/*1BF5:3449. Compatibility is latched; a later mismatch does not clear it. */
int16_t BtechProbePcjrTandyRomSignatures(void)
{
    if (!btechHostReadRomByte) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostReadRomByte");
    if (btechHostReadRomByte(BIOS_ROM_SELECTOR,BIOS_MODEL_OFFSET)==BIOS_PCJR_MODEL_BYTE && btechHostReadRomByte(BIOS_ROM_SELECTOR,TANDY_ROM_SIGNATURE_OFFSET)==TANDY_ROM_SIGNATURE_BYTE)
        pcjrTandyCompatible=1;
    return (int8_t)pcjrTandyCompatible;
}

/*1BF5:2BC8. Repeated calls still adjust BIOS shift state. The compatibility
 * branch patches two CS immediates and four scan-table bytes in native order.
 * The original's argument chose an INT 1Ch hook; its only caller passed 0,
 * so the argument and INT 1Ch's save and install are gone (H4c). */
void BtechInstallKeyboardAndTickHandlers(void)
{
    if (!btechHostGetKeyboardOwner || !btechHostSetKeyboardOwner
        || !btechHostReadBiosByte || !btechHostWriteBiosByte) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostGetKeyboardOwner || !btechHostSetKeyboardOwner || !btechHostReadBiosByte || !btechHostWriteBiosByte");
    if (!combatStartup.keyboardHandlersInstalled)
    {
        combatHandlers.savedKeyboardOwner=btechHostGetKeyboardOwner();
        btechHostSetKeyboardOwner(MwKeyboardOwner::Combat);
        combatStartup.keyboardBiosIntercept=0;
        int16_t compatibility=BtechProbePcjrTandyRomSignatures();
        if (compatibility)
        {
            if (!btechHostQueryKeyboardIntercept) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostQueryKeyboardIntercept");
            /* NEG AX leavesFFFF when3449 returns1; it is not BIOS C0. */
            if (btechHostQueryKeyboardIntercept((uint16_t)-compatibility)
                && btechHostReadBiosByte(BIOS_KEYBOARD_COMPATIBILITY_STATE)==KEYBOARD_COMPATIBILITY_INTERCEPT_STATE) combatStartup.keyboardBiosIntercept=1;
            else
            {
                combatHandlers.keyboardScanMask=KEYBOARD_ATTRIBUTE_CLEAR_HELD;combatHandlers.keyboardExtendedScanMask=KEYBOARD_ATTRIBUTE_CLEAR_HELD;
                combatStartup.keyboardScanTranslations[COMBAT_KEY_GRAVE]=combatStartup.keyboardScanTranslations[COMBAT_KEY_UP];
                combatStartup.keyboardScanTranslations[COMBAT_KEY_BACKSLASH]=combatStartup.keyboardScanTranslations[COMBAT_KEY_LEFT];
                combatStartup.keyboardScanTranslations[COMBAT_KEY_KEYPAD_PLUS]=combatStartup.keyboardScanTranslations[COMBAT_KEY_RIGHT];
                combatStartup.keyboardScanTranslations[COMBAT_KEY_KEYPAD_MINUS]=combatStartup.keyboardScanTranslations[COMBAT_KEY_DOWN];
            }
        }
        combatStartup.keyboardHandlersInstalled=1;
    }
    combatInput.keyboardShiftFlags&=BIOS_SHIFT_CLEAR_NUM_LOCK;
    if (combatStartup.keyboardCapsLockRequested) combatInput.keyboardShiftFlags|=BIOS_SHIFT_CAPS_LOCK;
}

/*1BF5:1F20. Saved offset/selector are captured first (the CLI/STI around
 * the install are gone, H4c); reinstallation
 * overwrites them even if INT00 already points at this handler. */
void BtechInstallDivideErrorHandler(void)
{
    if (!btechHostGetDivideOwner || !btechHostSetDivideOwner) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostGetDivideOwner || !btechHostSetDivideOwner");
    combatStartup.divideErrorHookMarkedActive=1;
    combatStartup.savedDivideErrorOwner=btechHostGetDivideOwner();
    btechHostSetDivideOwner(MwDivideOwner::Combat);
}

/*1BF5:2540. JG is SIGNED: high-bit periods are accepted. Zero/positive>FF
 * still probe ROM/save vector before returning. Never impose an unsigned255
 * check or a guard that changes those preceding effects. */
void BtechInstallTimerScheduler(uint16_t period)
{
    if (runtimeDelayActive) return;
    combatStartup.schedulerActiveSlots=0;
    BtechProbePcjrTandyRomSignatures();
    if (!btechHostGetTimerOwner) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostGetTimerOwner");
    combatHandlers.savedTimerOwner=btechHostGetTimerOwner();
    if ((int16_t)period>COMBAT_SCHEDULER_MAX_POSITIVE_PERIOD || !period) return;
    combatStartup.schedulerPeriod=period;combatStartup.schedulerCountdown=period;
    combatStartup.pitChannel0DivisorShadow=UINT16_MAX/period;
    if (!btechHostEnableDeviceIrqs || !btechHostSetTimerOwner || !btechHostSetTimerPeriod) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostEnableDeviceIrqs || !btechHostSetTimerOwner || !btechHostSetTimerPeriod");
    /* The PIT programming (43h, 40h low, 40h high) is the host's timer period since H4b. */
    btechHostSetTimerPeriod(combatStartup.pitChannel0DivisorShadow);
    btechHostEnableDeviceIrqs(MwDeviceIrq::All); /* Mask 28h: off only IRQ3 and 5, which the host has no device on. */
    btechHostSetTimerOwner(MwTimerOwner::CombatScheduler);
    runtimeDelayActive=1;
}
