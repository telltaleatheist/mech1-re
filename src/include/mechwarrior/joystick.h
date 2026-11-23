#ifndef MECHWARRIOR_JOYSTICK_H
#define MECHWARRIOR_JOYSTICK_H
#include <stdbool.h>
#include <stdint.h>

/* A joystick as the games read it (docs/phase5/NO_EMULATION_PLAN.md, H3): two
 * axes, centred and scaled to -127..127 (left or up negative), and two
 * buttons. The host reads it from SDL3; there is no game port. */
typedef struct MwJoystickState
{
    bool connected;
    int8_t x,y;
    bool buttonOne,buttonTwo;
} MwJoystickState;
enum { MW_JOYSTICK_AXIS_FULL=127 };

/* An axis as the games' calibration rules take it, where the original counted
 * the game port's discharge: the centre reads 127, full deflection 0 or 254.
 * Both games keep their own rules on top of it: the centre each calibration
 * takes, and the thresholds and scale they derive from that centre. */
static inline uint8_t MwJoystickAxisReading(int8_t axis)
{
    int value=axis<-MW_JOYSTICK_AXIS_FULL?-MW_JOYSTICK_AXIS_FULL:axis;
    return (uint8_t)(MW_JOYSTICK_AXIS_FULL+value);
}
#endif
