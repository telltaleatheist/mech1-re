#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_input.h"
#include <stdlib.h>

/*1BF5:2F58. PUSHF/CLI/POPF, not an unconditional STI. The next key word
 * from the key queue (the BIOS ring in the original; H2), or 0 if none. */
uint16_t BtechPopKey(void)
{
    if (!btechHostSetInterrupts || !btechHostRunDue) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostSetInterrupts || !btechHostRunDue");
    btechHostRunDue(); /* Keeps when the due ticks and key events land: the original took them as it reached the CLI. */
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
    if (!btechHostSetInterrupts || !btechHostRunDue || scanCode>=sizeof(keyboardScanTranslations)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostSetInterrupts || !btechHostRunDue || scanCode>=sizeof(keyboardScanTranslations)");
    btechHostRunDue(); /* Keeps when the due ticks and key events land: the original took them as it reached the CLI. */
    btechHostSetInterrupts(0);
    uint16_t held=keyboardScanTranslations[scanCode]&KEYBOARD_ATTRIBUTE_HELD;
    btechHostSetInterrupts(1);
    return held;
}

/*1BF5:21C7. An axis's reading as a value from its calibration: the distance
 * from the centre times the axis's reciprocal, in 256ths (the low word of the
 * product), at most 127, none under 8, negative below the centre. Only the
 * low byte is tested, as the original's AL. */
static int16_t ScaleJoystickAxis(uint16_t timing,uint8_t centre,uint16_t reciprocal)
{
    int16_t distance=(int16_t)(timing-centre);
    bool below=distance<0;
    uint16_t value=(uint16_t)(below?-distance:distance);
    if (reciprocal) value=(uint16_t)((uint16_t)(value*reciprocal)>>8);
    uint8_t low=(uint8_t)value;
    if (low>JOYSTICK_AXIS_VALUE_LIMIT) low=JOYSTICK_AXIS_VALUE_LIMIT;
    if (low<JOYSTICK_AXIS_DEAD_ZONE) low=0;
    return below?(int16_t)-low:(int16_t)low;
}

/*1BF5:22F9. Joystick A's (0) or B's X and Y values, when it is installed;
 * the X value waits in 41D9 while Y is read. */
static void ReadJoystickAxes(uint16_t joystick,int16_t *x,int16_t *y)
{
    uint16_t pair=joystick!=0;
    if (!joystickPairInstalled[pair]) return;
    /* H3b, a project decision: a joystick taken out since calibration reads
     * centred. The original's game port would time out to the limit, full
     * right and down, a held turn. */
    if (!btechHostReadJoystick) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostReadJoystick");
    if (!btechHostReadJoystick(pair).connected) {joystickFirstAxisValue=*x=*y=0;return;}
    uint16_t xAxis=pair*JOYSTICK_AXES_PER_PAIR,yAxis=xAxis+1;
    joystickFirstAxisValue=ScaleJoystickAxis(BtechReadJoystickAxisTiming((uint8_t)(1<<xAxis)),
        joystickAxisCentres[xAxis],joystickAxisReciprocals[xAxis]);
    int16_t second=ScaleJoystickAxis(BtechReadJoystickAxisTiming((uint8_t)(1<<yAxis)),
        joystickAxisCentres[yAxis],joystickAxisReciprocals[yAxis]);
    *y=second;*x=joystickFirstAxisValue;
}

/*1BF5:2376. An installed joystick's direction bits from its values; zero
 * when it is not installed. */
uint8_t BtechReadJoystickDirectionMask(uint16_t joystick)
{
    if (!joystickPairInstalled[joystick!=0]) return 0;
    ReadJoystickAxes(joystick,&joystickDirectionX,&joystickDirectionY);
    uint8_t direction=0;
    if (joystickDirectionX<JOYSTICK_DIRECTION_LOW) direction|=COMBAT_DIRECTION_LEFT;
    else if (joystickDirectionX>=JOYSTICK_DIRECTION_HIGH) direction|=COMBAT_DIRECTION_RIGHT;
    if (joystickDirectionY<JOYSTICK_DIRECTION_LOW) direction|=COMBAT_DIRECTION_UP;
    else if (joystickDirectionY>=JOYSTICK_DIRECTION_HIGH) direction|=COMBAT_DIRECTION_DOWN;
    return direction;
}

/*1BF5:23D6. 1 while an installed joystick's button is held (H3: the host's
 * buttons, where the original read the game port's active-low bits 4-7). AH
 * residue on absent pairs is zero in the actual 026B/2A17 callers. */
uint16_t BtechReadJoystickPresenceState(uint16_t button)
{
    uint16_t pair=(button&JOYSTICK_BUTTON_PAIR_MASK)!=0;
    if (!joystickPairInstalled[pair]) return 0;
    if (!btechHostReadJoystick) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostReadJoystick");
    MwJoystickState joystick=btechHostReadJoystick(pair);
    return (button&JOYSTICK_BUTTON_SECOND_MASK)?joystick.buttonTwo:joystick.buttonOne;
}
