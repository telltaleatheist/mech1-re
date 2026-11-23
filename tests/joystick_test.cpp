// A connected joystick in both games (docs/phase5/NO_EMULATION_PLAN.md, H3):
// the campaign's and combat's calibration and reads driven with fake axes, and
// what each game reports. The original was never run with a joystick here, so
// there is no reference: these hold the games' own calibration rules on the
// axis reading (centre 127, full deflection 0 and 254). Plain checks, no
// framework: exit 0 when all hold.
#include "mechwarrior/campaign_input.h"
#include "mechwarrior/campaign_startup.h"
#include "mechwarrior/combat_input.h"

#include <cstdio>

namespace {

int failures = 0;
void Check(bool ok, const char *what) {
  if (!ok) {
    std::printf("FAIL %s\n", what);
    ++failures;
  }
}

MwJoystickState sticks[2];
unsigned charges[3];
bool interrupts = true;

MwJoystickState ReadA() { return sticks[0]; }
MwJoystickState ReadEither(uint16_t joystick) { return sticks[joystick != 0]; }
bool MousePresent() { return false; }
uint8_t SetInterrupts(uint8_t enabled) {
  uint8_t previous = interrupts;
  interrupts = enabled != 0;
  return previous;
}
void Charge(uint8_t wait) { ++charges[wait]; }

MwJoystickState Stick(int x, int y, bool one = false, bool two = false) {
  return MwJoystickState{true, (int8_t)x, (int8_t)y, one, two};
}

// The campaign's poll of joystick A at a position.
void CampaignAt(int x, int y) {
  sticks[0] = Stick(x, y);
  MwPollJoystickA();
}
uint8_t CombatAt(int x, int y) {
  sticks[0] = Stick(x, y);
  return BtechReadJoystickDirectionMask(JOYSTICK_A);
}

}  // namespace

int main() {
  mwCampaignInput.readJoystick = ReadA;
  mwCampaignPlatform.mousePresent = MousePresent;
  btechHostReadJoystick = ReadEither;
  btechHostSetInterrupts = SetInterrupts;
  btechHostChargeGamePortWait = Charge;

  // The axis reading.
  Check(MwJoystickAxisReading(0) == 127 && MwJoystickAxisReading(-127) == 0 && MwJoystickAxisReading(127) == 254
            && MwJoystickAxisReading(-128) == 0,
        "the reading: centre 127, full deflection 0 and 254");

  // The campaign: detection, then the calibration screen with the stick centred.
  sticks[0] = MwJoystickState{false, 0, 0, false, false};
  MwDetectMouseAndJoystick();
  Check(joystickInstalled == 0 && mouseInstalled == 0, "no joystick connected: none installed");
  sticks[0] = Stick(0, 0);
  MwDetectMouseAndJoystick();
  Check(joystickInstalled == 1, "a connected joystick is installed");
  MwCalibrateJoystickA();
  Check(joystickCentreX == 127 && joystickCentreY == 127, "calibrated centred: the centre is 127");
  Check(joystickXLow == 102 && joystickXHigh == 152 && joystickXFarLow == 52 && joystickXFarHigh == 202,
        "the thresholds the game derives: a fifth of the centre each way, three fifths for far");

  CampaignAt(0, 0);
  Check(joystickDirection == JOYSTICK_CENTRED && joystickDeflection == JOYSTICK_DEFLECTION_NEAR, "centred");
  CampaignAt(-127, 0);
  Check(joystickDirection == JOYSTICK_LEFT && joystickDeflection == JOYSTICK_DEFLECTION_FAR, "full left: left, far");
  CampaignAt(127, 0);
  Check(joystickDirection == JOYSTICK_RIGHT && joystickDeflection == JOYSTICK_DEFLECTION_FAR, "full right: right, far");
  CampaignAt(0, -127);
  Check(joystickDirection == JOYSTICK_UP && joystickDeflection == JOYSTICK_DEFLECTION_FAR, "full up: up, far");
  CampaignAt(127, 127);
  Check(joystickDirection == JOYSTICK_DOWN_RIGHT && joystickDeflection == JOYSTICK_DEFLECTION_FAR, "down right");
  CampaignAt(-30, 0);
  Check(joystickDirection == JOYSTICK_LEFT && joystickDeflection == JOYSTICK_DEFLECTION_NEAR, "a little left: left, near");
  CampaignAt(-25, 25);
  Check(joystickDirection == JOYSTICK_CENTRED, "within a fifth: centred");

  // Buttons: a press is counted once, when it starts.
  sticks[0] = Stick(0, 0);
  MwPollJoystickA();
  sticks[0] = Stick(0, 0, true);
  MwPollJoystickA();
  Check(joystickButtonOnePressed == 1 && joystickButtonTwoPressed == 0, "button one pressed");
  MwPollJoystickA();
  Check(joystickButtonOnePressed == 0, "button one held: no new press");
  sticks[0] = Stick(0, 0, false, true);
  MwPollJoystickA();
  Check(joystickButtonOnePressed == 0 && joystickButtonTwoPressed == 1, "button two pressed");

  // Calibrated while held full right: the game takes that as the centre.
  sticks[0] = Stick(127, 0);
  MwCalibrateJoystickA();
  CampaignAt(0, 0);
  Check(joystickCentreX == 254 && joystickDirection == JOYSTICK_LEFT && joystickDeflection == JOYSTICK_DEFLECTION_NEAR,
        "calibrated full right, then centred: the game reads left, near");
  sticks[0] = Stick(0, 0);
  MwCalibrateJoystickA();

  // Combat: calibration with joystick A centred and no joystick B.
  sticks[0] = Stick(0, 0);
  sticks[1] = MwJoystickState{false, 0, 0, false, false};
  BtechCalibrateJoystickAxisPairs();
  Check(joystickPairInstalled[0] == 1 && joystickPairInstalled[1] == 0, "A installed, B not");
  Check(joystickAxisCentres[0] == 127 && joystickAxisCentres[1] == 127 && joystickAxisReciprocals[0] == 256,
        "A's centres 127, scale 256ths of 7F00h/127");
  Check(charges[BTECH_GAME_PORT_WAIT_CALIBRATION] == 1 && charges[BTECH_GAME_PORT_WAIT_AXIS] == 4
            && charges[BTECH_GAME_PORT_WAIT_PRESENCE] == 1 && interrupts,
        "the test clock charged where the original waited; interrupts back on");

  Check(CombatAt(0, 0) == 0, "combat centred: no direction");
  Check(CombatAt(-127, 0) == COMBAT_DIRECTION_LEFT && joystickDirectionX == -127, "combat full left: left, -127");
  Check(CombatAt(127, 0) == COMBAT_DIRECTION_RIGHT && joystickDirectionX == 127, "combat full right: right, 127");
  Check(CombatAt(0, -127) == COMBAT_DIRECTION_UP && CombatAt(0, 127) == COMBAT_DIRECTION_DOWN, "combat up and down");
  Check(CombatAt(-127, -127) == (COMBAT_DIRECTION_UP | COMBAT_DIRECTION_LEFT), "combat up left");
  Check(CombatAt(-52, 52) == 0 && CombatAt(-53, 53) == (COMBAT_DIRECTION_LEFT | COMBAT_DIRECTION_DOWN),
        "combat's thresholds: past -52, from 53");
  CombatAt(5, -7);
  Check(joystickDirectionX == 0 && joystickDirectionY == 0, "combat's values: none under 8");
  CombatAt(40, -40);
  Check(joystickDirectionX == 40 && joystickDirectionY == -40, "combat's values: the distance from the centre");

  sticks[0] = Stick(0, 0, true, false);
  Check(BtechReadJoystickPresenceState(JOYSTICK_A_BUTTON_ONE) == 1 && BtechReadJoystickPresenceState(JOYSTICK_A_BUTTON_TWO) == 0,
        "combat: A's button one held");
  sticks[1] = Stick(0, 0, true, true);
  Check(BtechReadJoystickPresenceState(2) == 0 && BtechReadJoystickDirectionMask(JOYSTICK_B) == 0,
        "combat: B not installed reads nothing, connected since or not");

  // Calibrated while held full right: the centre moves; centred reads left.
  sticks[0] = Stick(127, 0);
  BtechCalibrateJoystickAxisPairs();
  Check(joystickAxisCentres[0] == 254 && joystickAxisReciprocals[0] == 128, "calibrated full right: centre 254");
  Check(CombatAt(0, 0) == COMBAT_DIRECTION_LEFT && joystickDirectionX == -63,
        "then centred: combat reads left, -63");
  // Calibrated while held full left: a reading of 0 leaves the scale as it was.
  sticks[0] = Stick(-127, 0);
  BtechCalibrateJoystickAxisPairs();
  Check(joystickAxisCentres[0] == 0 && joystickAxisReciprocals[0] == 128, "calibrated full left: centre 0, scale kept");

  // No joystick at all: nothing installed, no direction.
  sticks[0] = MwJoystickState{false, 0, 0, false, false};
  sticks[1] = sticks[0];
  BtechCalibrateJoystickAxisPairs();
  Check(joystickPairInstalled[0] == 0 && joystickPairInstalled[1] == 0 && joystickAxisCentres[0] == 0xFF,
        "none connected: none installed, the timing limit as the centre");
  Check(BtechReadJoystickDirectionMask(JOYSTICK_A) == 0 && BtechReadJoystickPresenceState(JOYSTICK_A_BUTTON_ONE) == 0,
        "none connected: no direction, no button");

  std::printf("joystick: %s\n", failures ? "FAILED" : "ok");
  return failures ? 1 : 0;
}
