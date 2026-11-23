#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_platform.h"
#include "mechwarrior/combat_input.h"
#include <stdlib.h>

/*1BF5:1EB6. Sentinel returns BX paragraphs even on DOS success. Ordinary
 * quotient truncates BEFORE rounding. Host pointers replace selector:0000. */
BtechDosAllocation BtechAllocateDosMemoryBlock(uint32_t bytes)
{
    if (!btechHostAllocateParagraphs) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostAllocateParagraphs");
    uint16_t paragraphs=bytes==UINT32_MAX?UINT16_MAX:(uint16_t)((bytes>>4)+((bytes&0x0F)!=0));
    uint16_t largestFreeParagraphs=0;
    uint8_t *allocation=btechHostAllocateParagraphs(paragraphs,&largestFreeParagraphs);
    BtechDosAllocation result={allocation,(uint32_t)largestFreeParagraphs<<4};
    return result;
}

/*1BF5:1F10. DOS49 boundary; native reads the selector WORD at BP+08 only,
 * preserving ES. BP+06 offset is unused. A typed allocation replaces that
 * unobserved address transport, not a one-WORD native signature. */
void BtechFreeDosMemorySelector(uint8_t *allocation)
{
    if (!btechHostFreeDosAllocation) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostFreeDosAllocation");
    btechHostFreeDosAllocation(allocation);
}

/*1BF5:218B. One axis's reading (H3: the host's joystick, where the original
 * counted game-port reads between delay loops), or the timing limit when its
 * joystick is not connected. PUSHF/CLI/POPF stay until H4. */
uint16_t BtechReadJoystickAxisTiming(uint8_t axisMask)
{
    if (!btechHostSetInterrupts || !btechHostReadJoystick || !btechHostChargeGamePortWait) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostSetInterrupts || !btechHostReadJoystick || !btechHostChargeGamePortWait");
    uint8_t previousInterrupts=btechHostSetInterrupts(0);
    btechHostChargeGamePortWait(BTECH_GAME_PORT_WAIT_AXIS);
    btechHostSetInterrupts(previousInterrupts);
    MwJoystickState joystick=btechHostReadJoystick((axisMask&JOYSTICK_B_AXES_MASK)?JOYSTICK_B:JOYSTICK_A);
    if (!joystick.connected) return JOYSTICK_AXIS_TIMING_LIMIT;
    return MwJoystickAxisReading((axisMask&JOYSTICK_X_AXES_MASK)?joystick.x:joystick.y);
}

/*1BF5:21FF. Each axis's centre and scale from where the joysticks are now,
 * and which are installed: a joystick whose X axis reads (is connected). The
 * original first timed 1,000 game-port reads against the PIT to calibrate its
 * delay loops, and last checked that each joystick's axes were still charging
 * after 20 more; that hardware timing and presence test are gone (H3). */
void BtechCalibrateJoystickAxisPairs(void)
{
    if (!btechHostSetInterrupts || !btechHostChargeGamePortWait) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostSetInterrupts || !btechHostChargeGamePortWait");
    btechHostSetInterrupts(0);
    btechHostChargeGamePortWait(BTECH_GAME_PORT_WAIT_CALIBRATION);
    btechHostSetInterrupts(1);
    for (uint16_t axis=0;axis<JOYSTICK_AXIS_COUNT;++axis)
    {
        uint16_t timing=BtechReadJoystickAxisTiming((uint8_t)(1<<axis));
        joystickAxisCentres[axis]=(uint8_t)timing;
        if (!(axis&JOYSTICK_AXIS_PAIR_POSITION_MASK)) joystickPairInstalled[axis/JOYSTICK_AXES_PER_PAIR]=timing!=JOYSTICK_AXIS_TIMING_LIMIT;
        if (timing) joystickAxisReciprocals[axis]=JOYSTICK_AXIS_RECIPROCAL_NUMERATOR/timing;
    }
    btechHostChargeGamePortWait(BTECH_GAME_PORT_WAIT_PRESENCE);
}
