#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_input.h"
#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_installation.h"
#include <stdlib.h>

/*0810:0010. FAR termination callback reached by keyboard Ctrl-Escape. */
void BtechRequestTermination(void)
{
    combatInput.terminationFlag=1;
}

/* A pass of a wait for a key: the host sleeps if nothing is due (H4c). The key
 * reads take no time, so without it a wait would spin. Unbound in pure C fixtures. */
static void WaitIdle(void)
{
    if (btechHostWaitIdle) btechHostWaitIdle();
}

static bool CombatInputHeld(void)
{
    return combatInput.combatInputScanCode || BtechReadJoystickPresenceState(JOYSTICK_A_BUTTON_ONE) || BtechReadJoystickPresenceState(JOYSTICK_A_BUTTON_TWO);
}

/*0810:026B. Keep native release phase, then press phase and their original
 * short-circuit order. Timer/key interrupts arrive through hardware bindings.
 * The 1990-01-22 build's routine (0810:02CB in that build) adds a third phase:
 * it saves the pressed scan, waits for the release again and returns the scan.
 * That costs one poll after a press, so combat's first tick comes one poll
 * later after the briefing. Returns the scan seen at the press (both). */
uint16_t BtechWaitForCombatInputReleaseThenPress(void)
{
    while (CombatInputHeld())
    {BtechPollCombatCommandInput();WaitIdle();}
    while (!combatInput.combatInputScanCode && !BtechReadJoystickPresenceState(JOYSTICK_A_BUTTON_ONE) && !BtechReadJoystickPresenceState(JOYSTICK_A_BUTTON_TWO))
    {
        if (combatInput.terminationFlag) BtechTerminateToLauncher(1);
        BtechPollCombatCommandInput();WaitIdle();
    }
    uint16_t scanCode=combatInput.combatInputScanCode;
    if (btechLoadedRevision==BTECH_REVISION_1990_01_22)
        while (CombatInputHeld()) {BtechPollCombatCommandInput();WaitIdle();}
    return scanCode;
}

/* The two-line dialog's key (0810:27F4 in the reference). The reference polls
 * on until the scan changes, after the wait. The 1990-01-22 build's wait has
 * already seen the release (call at 0810:29B2 in that build); its dialog then
 * clears the scan and both button words (DS:C9AC, D22C, D230 there) and keeps
 * the direction mask. Returns the scan seen at the press. */
uint16_t BtechWaitForDialogKey(void)
{
    if (btechLoadedRevision==BTECH_REVISION_1990_01_22)
    {
        uint16_t scanCode=BtechWaitForCombatInputReleaseThenPress();
        combatInput.combatInputScanCode=combatInput.combatInputButton1State=combatInput.combatInputButton0State=0;
        return scanCode;
    }
    BtechWaitForCombatInputReleaseThenPress();
    uint16_t scanCode=combatInput.combatInputScanCode;
    if (scanCode)
        while (scanCode==combatInput.combatInputScanCode) {BtechPollCombatCommandInput();WaitIdle();}
    return scanCode;
}

/*0810:2A17. Native SI/DI are preserved machine registers, not arguments or
 * source outputs. Publish the four WORDs only after the complete poll. */
void BtechPollCombatCommandInput(void)
{
    if (btechHostNoteCombatInputPoll) btechHostNoteCombatInputPoll(); /* Host test feed; unbound normally. */
    uint16_t scanCode=BtechPopKey()>>BIOS_KEY_SCAN_SHIFT;
    uint16_t direction=BtechReadJoystickDirectionMask(JOYSTICK_A);
    if (!direction)
    {
        direction=BtechReadHeldDirectionMask(0);
        if (!direction)
            switch (scanCode)
            {
            case COMBAT_KEY_HOME:direction=COMBAT_DIRECTION_UP|COMBAT_DIRECTION_LEFT;break;
            case COMBAT_KEY_UP:direction=COMBAT_DIRECTION_UP;break;
            case COMBAT_KEY_PAGE_UP:direction=COMBAT_DIRECTION_UP|COMBAT_DIRECTION_RIGHT;break;
            case COMBAT_KEY_LEFT:direction=COMBAT_DIRECTION_LEFT;break;
            case COMBAT_KEY_RIGHT:direction=COMBAT_DIRECTION_RIGHT;break;
            case COMBAT_KEY_END:direction=COMBAT_DIRECTION_DOWN|COMBAT_DIRECTION_LEFT;break;
            case COMBAT_KEY_DOWN:direction=COMBAT_DIRECTION_DOWN;break;
            case COMBAT_KEY_PAGE_DOWN:direction=COMBAT_DIRECTION_DOWN|COMBAT_DIRECTION_RIGHT;break;
            }
    }
    uint16_t button1=BtechReadJoystickPresenceState(JOYSTICK_A_BUTTON_TWO);
    uint16_t button0=BtechReadJoystickPresenceState(JOYSTICK_A_BUTTON_ONE);
    switch (scanCode)
    {
    case COMBAT_KEY_N:direction=COMBAT_DIRECTION_DOWN;button1=1;break;
    case COMBAT_KEY_M:direction=COMBAT_DIRECTION_UP;button1=1;break;
    case COMBAT_KEY_COMMA:direction=COMBAT_DIRECTION_LEFT;button1=1;break;
    case COMBAT_KEY_PERIOD:direction=COMBAT_DIRECTION_RIGHT;button1=1;break;
    }
    if (combatInputReplayMode==COMBAT_INPUT_PLAYBACK)
    {
        if (scanCode|button1|button0|combatInput.combatInputReplayCancelLatched)
        {scanCode=COMBAT_KEY_ESCAPE;combatInput.combatInputReplayCancelLatched=1;}
        else /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Native replay IO/short-read stack residue is not converted."); /* Native replay IO/short-read stack residue is not converted. */
    }
    else if (combatInputReplayMode==COMBAT_INPUT_RECORD) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: combatInputReplayMode==COMBAT_INPUT_RECORD"); /* Native append-stream writer dependency. */
    combatInput.combatInputScanCode=scanCode;
    combatInput.combatInputDirectionMask=direction;
    combatInput.combatInputButton1State=button1;
    combatInput.combatInputButton0State=button0;
}
