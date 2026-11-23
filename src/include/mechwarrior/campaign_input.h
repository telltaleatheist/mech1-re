#ifndef MECHWARRIOR_CAMPAIGN_INPUT_H
#define MECHWARRIOR_CAMPAIGN_INPUT_H
#include "mechwarrior/campaign.h"
#include "mechwarrior/input_hardware.h"
#include "mechwarrior/key_events.h"
#include "mechwarrior/joystick.h"

/* Lowest BIOS/port boundaries only; decoding and polling stay original code.
 * Backends must drive the original0273 tick when delivering a timer interrupt.
 * An unbound operation is unsupported, never a successful/no-input stub. */
typedef struct MwCampaignInputCalls
{
    uint16_t (*mouseReleaseCount)(uint16_t button);
    bool (*keyboardAvailable)(void);
    MwCampaignKey (*readKeyboard)(void); /* The next key press (H2). */
    MwJoystickState (*readJoystick)(void); /* Joystick A (H3). */
    void (*waitForInterrupt)(void);
    uint8_t (*currentDefaultDrive)(void);
    void (*resetMouse)(void);
    void (*readMouseMotion)(uint16_t *xMotion,uint16_t *yMotion);
    bool (*controlHeld)(void); /* Control, with the last key read or held now. */
    /* The pointer's position in the frame's pixels (H3; INT 33h's x was twice). */
    void (*setMousePosition)(uint16_t x,uint16_t y);
    void (*readMousePosition)(uint16_t *x,uint16_t *y);
} MwCampaignInputCalls;
extern MwCampaignInputCalls mwCampaignInput;
extern uint8_t campaignTimerInhibited, campaignTimerByte;
extern uint16_t campaignTimerTicks;
extern uint8_t campaignTimerAlternatingPhase, campaignTimerPhaseTicks;
extern uint8_t mouseInstalled, joystickInstalled, inputSinglePoll;
extern uint16_t joystickDirection, joystickButtonOnePressed, joystickButtonTwoPressed;
extern uint8_t joystickPreviousButtons;
extern uint16_t joystickXCount, joystickYCount, joystickDeflection;
extern uint16_t joystickXLow, joystickXHigh, joystickYLow, joystickYHigh;
extern uint16_t joystickXFarLow, joystickXFarHigh, joystickYFarLow, joystickYFarHigh;
/* joystickPreviousButtons keeps the buttons as the game port showed them:
 * a bit clear while its button is held. */
enum { JOYSTICK_BUTTON_ONE_UP = 0x10, JOYSTICK_BUTTON_TWO_UP = 0x20,
    JOYSTICK_DEFLECTION_NEAR = 1, JOYSTICK_DEFLECTION_FAR = 4 };
enum { JOYSTICK_CENTRED, JOYSTICK_UP, JOYSTICK_UP_RIGHT, JOYSTICK_RIGHT,
    JOYSTICK_DOWN_RIGHT, JOYSTICK_DOWN, JOYSTICK_DOWN_LEFT, JOYSTICK_LEFT,
    JOYSTICK_UP_LEFT, CAMPAIGN_TIMER_PHASE_PERIOD = 2 };
void MwCampaignTimerTick(void);
void MwPollJoystickA(void);
void MwWaitForInputAcknowledgment(void);
bool MwPollInputUntilIdleOrEvent(void);
bool MwPollInputOnce(void);
bool MwPollMouseNavigationKey(uint16_t *navigationKey);
bool MwPollNavigationKeyFromMouseOrJoystick(uint16_t *navigationKey);
void MwCalibrateJoystickA(void);
uint16_t MwReadCampaignTextInput(uint16_t x,uint16_t y,uint16_t capacity,
    uint8_t foreground,uint8_t background);
void MwEraseTextInputCursor(void);
extern uint16_t mouseMotionX,mouseMotionY;
extern uint8_t joystickRepeatActive;
extern uint16_t joystickRepeatDeadline,joystickNavigationKeys[9];
extern uint16_t joystickCentreX,joystickCentreY;
extern uint16_t joystickXDeflectionStep,joystickYDeflectionStep;
extern uint16_t textInputCursorX,textInputCursorY,textInputMaximumIndex;
extern uint8_t textInputForeground,textInputBackground,textInputLastPhase;
extern uint16_t textInputLength;
enum { CAMPAIGN_KEY_ENTER=0x1C0D,CAMPAIGN_KEY_UP=0x4800,CAMPAIGN_KEY_DOWN=0x5000,
    CAMPAIGN_KEY_LEFT=0x4B00,CAMPAIGN_KEY_RIGHT=0x4D00,CAMPAIGN_KEY_CALIBRATE=0x3B00,
    CAMPAIGN_KEY_UP_LEFT=0x4700,CAMPAIGN_KEY_UP_RIGHT=0x4900,
    CAMPAIGN_KEY_DOWN_LEFT=0x4F00,CAMPAIGN_KEY_DOWN_RIGHT=0x5100,
    CAMPAIGN_KEY_ESCAPE=0x011B,CAMPAIGN_KEY_BACKSPACE=0x0E08,
    MOUSE_NAVIGATION_THRESHOLD=0x21,JOYSTICK_REPEAT_TICKS=6,
    JOYSTICK_CALIBRATION_DIVISOR=5,
    CAMPAIGN_TEXT_SECOND_CASE_ADJUSTMENT=0x14 }; /* Retained second native lowercase branch. */
#endif
