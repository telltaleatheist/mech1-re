// Controllers through the real SDL host (docs/phase5/NO_EMULATION_PLAN.md,
// H3b): SDL3 virtual devices, a gamepad and plain joysticks, plugged in and
// taken out as events the host's loop handles, read by MwSdlReadJoystick and
// through it by the campaign's and combat's own joystick code. No window and
// no video driver. No physical controller was available to try by hand, so
// this is the controller path's check. Plain checks, no framework: exit 0
// when all hold, 77 (skipped) when a real controller is connected, since it
// would take a slot.
#include "SDLBackend.h"
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

// The host's loop: every queued event, the joystick ones handled as it does.
void Pump() {
  SDL_PumpEvents();
  SDL_Event event;
  while (SDL_PollEvent(&event)) MwSdlHandleJoystickEvent(event);
}

// A virtual device and the test's own handle to move it (values show after
// SDL_UpdateJoysticks, which the host's read calls).
struct Device {
  SDL_JoystickID id = 0;
  SDL_Joystick *handle = nullptr;
};
Device Attach(SDL_JoystickType type, int axes, int buttons, const char *name) {
  SDL_VirtualJoystickDesc desc;
  SDL_INIT_INTERFACE(&desc);
  desc.type = (Uint16)type, desc.naxes = (Uint16)axes, desc.nbuttons = (Uint16)buttons, desc.name = name;
  Device device;
  device.id = SDL_AttachVirtualJoystick(&desc);
  if (device.id) device.handle = SDL_OpenJoystick(device.id);
  Pump();
  return device;
}
Device AttachGamepad() {
  return Attach(SDL_JOYSTICK_TYPE_GAMEPAD, SDL_GAMEPAD_AXIS_COUNT, SDL_GAMEPAD_BUTTON_COUNT, "mw1 test pad");
}
// A flight stick's four axes: X, Y, twist, throttle.
Device AttachStick() { return Attach(SDL_JOYSTICK_TYPE_FLIGHT_STICK, 4, 2, "mw1 test stick"); }
void Detach(Device &device) {
  SDL_CloseJoystick(device.handle);
  SDL_DetachVirtualJoystick(device.id);
  device = Device{};
  Pump();
}
void Set(Device &device, int axisX, int axisY, Sint16 x, Sint16 y, int buttonOne, int buttonTwo, bool one, bool two) {
  SDL_SetJoystickVirtualAxis(device.handle, axisX, x);
  SDL_SetJoystickVirtualAxis(device.handle, axisY, y);
  SDL_SetJoystickVirtualButton(device.handle, buttonOne, one);
  SDL_SetJoystickVirtualButton(device.handle, buttonTwo, two);
}
void Pad(Device &pad, Sint16 x, Sint16 y, bool south = false, bool east = false) {
  Set(pad, SDL_GAMEPAD_AXIS_LEFTX, SDL_GAMEPAD_AXIS_LEFTY, x, y, SDL_GAMEPAD_BUTTON_SOUTH, SDL_GAMEPAD_BUTTON_EAST, south, east);
}
void Stick(Device &stick, Sint16 x, Sint16 y, bool one = false, bool two = false) { Set(stick, 0, 1, x, y, 0, 1, one, two); }

bool Reads(uint16_t joystick, bool connected, int x, int y, bool one, bool two) {
  MwJoystickState state = MwSdlReadJoystick(joystick);
  return state.connected == connected && state.x == x && state.y == y && state.buttonOne == one && state.buttonTwo == two;
}
bool ReadsAxes(uint16_t joystick, uint8_t x, uint8_t y) {
  MwJoystickState state = MwSdlReadJoystick(joystick);
  return MwJoystickAxisReading(state.x) == x && MwJoystickAxisReading(state.y) == y;
}

// The games' hosts, on the SDL host's read.
MwJoystickState CampaignJoystick() { return MwSdlReadJoystick(JOYSTICK_A); }
bool MousePresent() { return false; }
uint8_t SetInterrupts(uint8_t) { return 1; }
void Charge(uint8_t) {}

}  // namespace

int main() {
  if (!MwSdlInitializeJoysticks()) {
    std::printf("FAIL the gamepad subsystem: %s\n", SDL_GetError());
    return 1;
  }
  Pump();
  int count = 0;
  SDL_JoystickID *present = SDL_GetJoysticks(&count);
  for (int index = 0; index < count; ++index)
    if (!SDL_IsJoystickVirtual(present[index])) {
      std::printf("sdl_joystick: skipped, a real controller is connected\n");
      SDL_free(present);
      return 77;
    }
  SDL_free(present);
  mwCampaignInput.readJoystick = CampaignJoystick;
  mwCampaignPlatform.mousePresent = MousePresent;
  btechHostReadJoystick = MwSdlReadJoystick;
  btechHostSetInterrupts = SetInterrupts;
  btechHostChargeGamePortWait = Charge;

  Check(Reads(JOYSTICK_A, false, 0, 0, false, false) && Reads(JOYSTICK_B, false, 0, 0, false, false), "none plugged in");

  // A gamepad: SDL's own mapping, the left stick and the south and east buttons.
  Device pad = AttachGamepad();
  Check(pad.id && SDL_IsGamepad(pad.id), "the virtual pad is a gamepad");
  Check(Reads(JOYSTICK_A, true, 0, 0, false, false) && ReadsAxes(JOYSTICK_A, 127, 127), "pad as A, centred: 127");
  Check(Reads(JOYSTICK_B, false, 0, 0, false, false), "B empty");
  Pad(pad, -32768, 0);
  Check(Reads(JOYSTICK_A, true, -127, 0, false, false) && ReadsAxes(JOYSTICK_A, 0, 127), "pad full left: 0");
  Pad(pad, 32767, 0);
  Check(Reads(JOYSTICK_A, true, 127, 0, false, false) && ReadsAxes(JOYSTICK_A, 254, 127), "pad full right: 254");
  Pad(pad, 0, -32768);
  Check(ReadsAxes(JOYSTICK_A, 127, 0), "pad up: 0");
  Pad(pad, 0, 32767);
  Check(ReadsAxes(JOYSTICK_A, 127, 254), "pad down: 254");
  Pad(pad, 0, 0, true, false);
  Check(Reads(JOYSTICK_A, true, 0, 0, true, false), "south: button one");
  Pad(pad, 0, 0, false, true);
  Check(Reads(JOYSTICK_A, true, 0, 0, false, true), "east: button two");

  // The campaign on the pad.
  Pad(pad, 0, 0);
  MwDetectMouseAndJoystick();
  Check(joystickInstalled == 1, "campaign: the pad installed");
  MwCalibrateJoystickA();
  Check(joystickCentreX == 127 && joystickCentreY == 127, "campaign: calibrated at 127");
  Pad(pad, -32768, 0);
  MwPollJoystickA();
  Check(joystickDirection == JOYSTICK_LEFT && joystickDeflection == JOYSTICK_DEFLECTION_FAR, "campaign: left, far");
  Pad(pad, 0, 32767, true);
  MwPollJoystickA();
  Check(joystickDirection == JOYSTICK_DOWN && joystickButtonOnePressed == 1, "campaign: down, button one pressed");

  // A second device, a stick with four axes, as B; only its X and Y drive us.
  Device stick = AttachStick();
  Check(stick.id && !SDL_IsGamepad(stick.id), "the virtual stick is a plain joystick");
  Check(Reads(JOYSTICK_B, true, 0, 0, false, false), "stick as B");
  SDL_SetJoystickVirtualAxis(stick.handle, 2, 32767);   // twist full right
  SDL_SetJoystickVirtualAxis(stick.handle, 3, -32768);  // throttle at one end
  Check(Reads(JOYSTICK_B, true, 0, 0, false, false), "the stick's twist and throttle drive nothing");
  Stick(stick, 32767, -32768, false, true);
  Check(Reads(JOYSTICK_B, true, 127, -127, false, true), "the stick's X, Y and second button");
  Check(Reads(JOYSTICK_A, true, 0, 127, true, false), "A still the pad");

  // Combat on both: calibration installs both, then each reads on its own.
  Pad(pad, 0, 0);
  Stick(stick, 0, 0);
  BtechCalibrateJoystickAxisPairs();
  Check(joystickPairInstalled[0] == 1 && joystickPairInstalled[1] == 1, "combat: A and B installed");
  Pad(pad, 32767, 0, true);
  Stick(stick, 0, -32768, false, true);
  Check(BtechReadJoystickDirectionMask(JOYSTICK_A) == COMBAT_DIRECTION_RIGHT && joystickDirectionX == 127,
        "combat: A right");
  Check(BtechReadJoystickDirectionMask(JOYSTICK_B) == COMBAT_DIRECTION_UP && joystickDirectionY == -127, "combat: B up");
  Check(BtechReadJoystickPresenceState(JOYSTICK_A_BUTTON_ONE) == 1 && BtechReadJoystickPresenceState(JOYSTICK_A_BUTTON_TWO) == 0,
        "combat: A's first button held");
  Check(BtechReadJoystickPresenceState(JOYSTICK_BUTTON_PAIR_MASK) == 0
            && BtechReadJoystickPresenceState(JOYSTICK_BUTTON_PAIR_MASK | JOYSTICK_BUTTON_SECOND_MASK) == 1,
        "combat: B's second button held");

  // The window losing focus changes nothing: the host never acquires on focus.
  SDL_Event focus{};
  focus.type = SDL_EVENT_WINDOW_FOCUS_LOST;
  SDL_PushEvent(&focus);
  Pump();
  Check(Reads(JOYSTICK_A, true, 127, 0, true, false), "focus lost: the pad still reads");

  // Taken out mid-play, turning hard with fire held: released and centred, and
  // B goes on.
  Detach(pad);
  Check(Reads(JOYSTICK_A, false, 0, 0, false, false), "pad taken out: A not connected, centred, released");
  Check(BtechReadJoystickDirectionMask(JOYSTICK_A) == 0 && joystickDirectionX == 0 && joystickDirectionY == 0,
        "combat: no held turn once the pad is gone");
  Check(BtechReadJoystickPresenceState(JOYSTICK_A_BUTTON_ONE) == 0, "combat: no held fire once the pad is gone");
  MwPollJoystickA();
  Check(joystickDirection == JOYSTICK_CENTRED && joystickButtonOnePressed == 0, "campaign: centred once the pad is gone");
  Check(Reads(JOYSTICK_B, true, 0, -127, false, true), "B unchanged");

  // Plugged in again while B is in: it takes the free slot, A.
  pad = AttachGamepad();
  Check(Reads(JOYSTICK_A, true, 0, 0, false, false), "pad back as A");
  Pad(pad, -32768, 0);
  Check(BtechReadJoystickDirectionMask(JOYSTICK_A) == COMBAT_DIRECTION_LEFT, "combat: the pad back drives A");

  // Hot-plug order: with none in, the first plugged is A, the next B.
  Detach(pad);
  Detach(stick);
  Check(Reads(JOYSTICK_A, false, 0, 0, false, false) && Reads(JOYSTICK_B, false, 0, 0, false, false), "both out");
  stick = AttachStick();
  pad = AttachGamepad();
  Stick(stick, 32767, 0);
  Pad(pad, 0, 32767);
  Check(Reads(JOYSTICK_A, true, 127, 0, false, false) && Reads(JOYSTICK_B, true, 0, 127, false, false),
        "stick plugged first is A, the pad then B");
  Detach(stick);
  Check(Reads(JOYSTICK_A, false, 0, 0, false, false) && Reads(JOYSTICK_B, true, 0, 127, false, false),
        "A taken out: the pad stays B");
  Detach(pad);

  SDL_Quit();
  std::printf("sdl_joystick: %s\n", failures ? "FAILED" : "ok");
  return failures ? 1 : 0;
}
