#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_input.h"
#include <stdlib.h>

/*1BF5:2F58. PUSHF/CLI/POPF, not an unconditional STI. The next key word
 * from the key queue (the BIOS ring in the original; H2), or 0 if none. */
uint16_t BtechPopKey(void)
{
    if (!btechHostSetInterrupts) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostSetInterrupts");
    uint8_t savedInterrupts=btechHostSetInterrupts(0);
    uint16_t key=0;
    if (combatKeyQueueHead!=combatKeyQueueTail)
    {
        key=combatKeyQueue[combatKeyQueueHead];
        combatKeyQueueHead=(uint8_t)((combatKeyQueueHead+1)%COMBAT_KEY_QUEUE_SLOTS);
    }
    btechHostSetInterrupts(savedInterrupts);
    return key;
}

/*1BF5:2F87. Any nonzero selector chooses the second held-key mask. */
uint16_t BtechReadHeldDirectionMask(uint16_t keyGroup)
{
    uint8_t mask=heldDirectionKeyMasks[keyGroup!=0];
    if (mask>=sizeof(heldDirectionMasks)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mask>=sizeof(heldDirectionMasks)"); /* Adjacent owner alias, not another direction. */
    return heldDirectionMasks[mask];
}

/*1BF5:2FA1. Unlike2F58, CLI/STI does not restore the incoming IF. */
uint16_t BtechReadHeldKeyState(uint16_t scanCode)
{
    if (!btechHostSetInterrupts || scanCode>=sizeof(keyboardScanTranslations)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostSetInterrupts || scanCode>=sizeof(keyboardScanTranslations)");
    btechHostSetInterrupts(0);
    uint16_t held=keyboardScanTranslations[scanCode]&KEYBOARD_ATTRIBUTE_HELD;
    btechHostSetInterrupts(1);
    return held;
}

/*1BF5:2376. Absent pairs genuinely return zero. Installed calibrated-axis
 * execution is not silently replaced by a neutral joystick. */
uint8_t BtechReadJoystickDirectionMask(uint16_t joystick)
{
    if (joystickPairInstalled[joystick!=0]) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: joystickPairInstalled[joystick!=0]"); /*22F9 calibrated-axis dependency. */
    return 0;
}

/*1BF5:23D6. Button is active-low bit4..7; presence selects pair by bit1.
 * AH residue on absent pairs is zero in the actual026B/2A17 callers. */
uint16_t BtechReadJoystickPresenceState(uint16_t button)
{
    if (!joystickPairInstalled[(button&JOYSTICK_BUTTON_PAIR_MASK)!=0]) return 0;
    if (!btechHostReadPort) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostReadPort");
    return ((btechHostReadPort(GAME_PORT)>>((button&JOYSTICK_BUTTON_INDEX_MASK)+GAMEPORT_BUTTON_SHIFT))&GAMEPORT_BUTTON_LEVEL_MASK)^GAMEPORT_BUTTON_LEVEL_MASK;
}
