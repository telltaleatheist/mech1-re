#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/campaign_continuations.h"
#include "mechwarrior/campaign_input.h"
#include "mechwarrior/campaign_startup.h"
#include "mechwarrior/ega_campaign.h"
#include <stdlib.h>
#include <string.h>

/* Host-installed device entry points, not game input decisions. */
MwCampaignInputCalls mwCampaignInput;

/* MW_MAIN081B:0E10..0E8E. INT33 release, then relative motion; X wins ties. */
bool MwPollMouseNavigationKey(uint16_t *navigationKey)
{
    if (mwCampaignInput.mouseReleaseCount==NULL) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mwCampaignInput.mouseReleaseCount==NULL");
    if (mwCampaignInput.mouseReleaseCount(MOUSE_BUTTON_LEFT)!=0)
    { *navigationKey=CAMPAIGN_KEY_ENTER;return true; }
    if (mwCampaignInput.readMouseMotion==NULL) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mwCampaignInput.readMouseMotion==NULL");
    uint16_t motionX,motionY;
    mwCampaignInput.readMouseMotion(&motionX,&motionY);
    campaignInput.mouseMotionX+=motionX;campaignInput.mouseMotionY+=motionY;
    if ((int16_t)campaignInput.mouseMotionX>MOUSE_NAVIGATION_THRESHOLD) *navigationKey=CAMPAIGN_KEY_RIGHT;
    else if ((int16_t)campaignInput.mouseMotionX<-MOUSE_NAVIGATION_THRESHOLD) *navigationKey=CAMPAIGN_KEY_LEFT;
    else if ((int16_t)campaignInput.mouseMotionY>MOUSE_NAVIGATION_THRESHOLD) *navigationKey=CAMPAIGN_KEY_DOWN;
    else if ((int16_t)campaignInput.mouseMotionY<-MOUSE_NAVIGATION_THRESHOLD) *navigationKey=CAMPAIGN_KEY_UP;
    else { *navigationKey=campaignInput.mouseMotionY;return false; } /* Native AX residue. */
    campaignInput.mouseMotionX=0;campaignInput.mouseMotionY=0;
    return true;
}
/* MW_MAIN081B:0DF7..0E0F and shared0E8F..0EE6 joystick tail. CF/AX projection. */
bool MwPollNavigationKeyFromMouseOrJoystick(uint16_t *navigationKey)
{
    if (campaignInput.mouseInstalled!=0 && MwPollMouseNavigationKey(navigationKey)) return true;
    if (campaignInput.joystickInstalled==0) return false;
    MwPollJoystickA();
    if ((campaignInput.joystickButtonOnePressed|campaignInput.joystickButtonTwoPressed)!=0)
    { *navigationKey=CAMPAIGN_KEY_ENTER;return true; }
    if (campaignInput.joystickDirection==JOYSTICK_CENTRED)
    { campaignInput.joystickRepeatActive=0;return false; }
    if (campaignInput.joystickDeflection==JOYSTICK_DEFLECTION_NEAR) return false;
    if (campaignInput.joystickRepeatActive!=0 && campaignInput.joystickRepeatDeadline>campaignInput.campaignTimerTicks) return false;
    campaignInput.joystickRepeatDeadline=campaignInput.campaignTimerTicks+JOYSTICK_REPEAT_TICKS;
    campaignInput.joystickRepeatActive=1;
    uint16_t byteOffset=campaignInput.joystickDirection*2;
    if (byteOffset>=sizeof(campaignInput.joystickNavigationKeys)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: byteOffset>=sizeof(joystickNavigationKeys)"); /* Unmapped adjacent input storage. */
    *navigationKey=campaignInput.joystickNavigationKeys[byteOffset/2];
    return *navigationKey!=0;
}
/* MW_MAIN081B:03F7..04AC. The centre is where joystick A is now (H3: its
 * axes, where the original counted game-port reads with interrupts off and
 * retried if a tick came between). The thresholds are the original's rules. */
void MwCalibrateJoystickA(void)
{
    if (!mwCampaignInput.readJoystick) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mwCampaignInput.readJoystick");
    MwJoystickState joystick=mwCampaignInput.readJoystick();
    campaignInput.joystickCentreX=MwJoystickAxisReading(joystick.x);
    campaignInput.joystickCentreY=MwJoystickAxisReading(joystick.y);
    campaignInput.joystickXDeflectionStep=(uint16_t)(campaignInput.joystickCentreX*2)/JOYSTICK_CALIBRATION_DIVISOR;
    campaignInput.joystickXLow=campaignInput.joystickCentreX-(campaignInput.joystickXDeflectionStep>>1);
    campaignInput.joystickXFarLow=campaignInput.joystickXLow-campaignInput.joystickXDeflectionStep;
    campaignInput.joystickXHigh=campaignInput.joystickCentreX+(campaignInput.joystickXDeflectionStep>>1);
    campaignInput.joystickXFarHigh=campaignInput.joystickXHigh+campaignInput.joystickXDeflectionStep;
    campaignInput.joystickYDeflectionStep=(uint16_t)(campaignInput.joystickCentreY*2)/JOYSTICK_CALIBRATION_DIVISOR;
    campaignInput.joystickYLow=campaignInput.joystickCentreY-(campaignInput.joystickYDeflectionStep>>1);
    campaignInput.joystickYFarLow=campaignInput.joystickYLow-campaignInput.joystickYDeflectionStep;
    campaignInput.joystickYHigh=campaignInput.joystickCentreY+(campaignInput.joystickYDeflectionStep>>1);
    campaignInput.joystickYFarHigh=campaignInput.joystickYHigh+campaignInput.joystickYDeflectionStep;
}

/* MW_MAIN081B:0273..02A3. Host delivery replaces only STI/save/IRET plumbing;
 * all game-owned BYTE/WORD increments and inhibition are retained. */
void MwCampaignTimerTick(void)
{
    if (campaignInput.campaignTimerInhibited != 0) return;
    campaignInput.campaignTimerByte++;
    campaignInput.campaignTimerTicks++;
    campaignInput.campaignTimerPhaseTicks++;
    if (campaignInput.campaignTimerPhaseTicks < CAMPAIGN_TIMER_PHASE_PERIOD) return;
    campaignInput.campaignTimerPhaseTicks = 0;
    campaignInput.campaignTimerAlternatingPhase ^= 1;
}

/* MW_MAIN081B:02F6..02FA falls through02FB: this WAITS for an event despite
 * the historical "DiscardPendingInput" name. Do not replace it with a drain. */
void MwWaitForInputAcknowledgment(void)
{ campaignInput.inputSinglePoll = 0; (void)MwPollInputUntilIdleOrEvent(); }

/* MW_MAIN081B:02FB..034C. A mouse release or new joystick press returns before
 * RNG/keyboard. Otherwise every pass consumes a real0C17 draw, even if idle. */
bool MwPollInputUntilIdleOrEvent(void)
{
    do
    {
        if (campaignInput.mouseInstalled != 0)
        {
            if (mwCampaignInput.mouseReleaseCount == NULL) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mwCampaignInput.mouseReleaseCount == NULL");
            if (mwCampaignInput.mouseReleaseCount(MOUSE_BUTTON_LEFT) != 0) return true;
            if (mwCampaignInput.mouseReleaseCount(MOUSE_BUTTON_RIGHT) != 0) return true;
        }
        if (campaignInput.joystickInstalled != 0)
        {
            MwPollJoystickA();
            if ((campaignInput.joystickButtonOnePressed | campaignInput.joystickButtonTwoPressed) != 0) return true;
        }
        (void)MwRandomUpTo(UINT8_MAX);
        if (mwCampaignInput.keyboardAvailable == NULL) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mwCampaignInput.keyboardAvailable == NULL");
        if (mwCampaignInput.keyboardAvailable())
        {
            if (mwCampaignInput.readKeyboard == NULL) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mwCampaignInput.readKeyboard == NULL");
            (void)mwCampaignInput.readKeyboard();
            return true;
        }
    } while (campaignInput.inputSinglePoll == 0);
    return false;
}
/* MW_MAIN081B:034D..0353 JMP02FB, no nested native CALL. */
bool MwPollInputOnce(void)
{ campaignInput.inputSinglePoll = 1; return MwPollInputUntilIdleOrEvent(); }

/* MW_MAIN081B:070C..0724. */
void MwEraseTextInputCursor(void)
{
    uint16_t cursorY=campaignInput.textInputCursorY;
    (void)EgaDrawCharacter(campaignInput.textInputCursorX,&cursorY,
        ' ',campaignInput.textInputForeground,campaignInput.textInputBackground);
}

/* MW_MAIN081B:064E..075D. The second lowercase test is retained even though
 * the first subtraction makes it unreachable for ordinary ASCII letters. */
uint16_t MwReadCampaignTextInput(uint16_t x,uint16_t y,uint16_t capacity,
    uint8_t foreground,uint8_t background)
{
    campaignInput.textInputCursorX=x;campaignInput.textInputCursorY=y;campaignInput.textInputMaximumIndex=(uint16_t)(capacity-1);
    campaignInput.textInputForeground=foreground;campaignInput.textInputBackground=background;
    memset(campaignTextInputBuffer,0,sizeof(campaignTextInputBuffer));campaignInput.textInputLength=0;
    for (;;)
    {
        if (!mwCampaignInput.keyboardAvailable || !mwCampaignInput.readKeyboard) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mwCampaignInput.keyboardAvailable || !mwCampaignInput.readKeyboard");
        if (!mwCampaignInput.keyboardAvailable())
        {
            if (campaignInput.campaignTimerAlternatingPhase!=campaignInput.textInputLastPhase)
            {
                campaignInput.textInputLastPhase=campaignInput.campaignTimerAlternatingPhase;
                uint16_t cursorY=campaignInput.textInputCursorY;
                (void)EgaDrawCharacter(campaignInput.textInputCursorX,&cursorY,
                    campaignInput.campaignTimerAlternatingPhase?'*':' ',campaignInput.textInputForeground,campaignInput.textInputBackground);
            }
            if (!mwCampaignInput.waitForInterrupt) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mwCampaignInput.waitForInterrupt");
            mwCampaignInput.waitForInterrupt();continue;
        }
        uint16_t key=MwCampaignKeyWord(mwCampaignInput.readKeyboard());
        if (key==CAMPAIGN_KEY_ENTER)
        {
            if (campaignInput.mouseInstalled!=0) {if (!mwCampaignInput.resetMouse) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mwCampaignInput.resetMouse");mwCampaignInput.resetMouse();}
            MwEraseTextInputCursor();return 0;
        }
        if (key==CAMPAIGN_KEY_BACKSPACE)
        {
            if (campaignInput.textInputLength==0) continue;
            campaignTextInputBuffer[--campaignInput.textInputLength]=0;MwEraseTextInputCursor();
            campaignInput.textInputCursorX -= selectedGlyphAdvance;continue;
        }
        if (key==CAMPAIGN_KEY_ESCAPE)
        {
            if (!mwCampaignInput.controlHeld) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mwCampaignInput.controlHeld");
            if (mwCampaignInput.controlHeld()) return CAMPAIGN_CONTINUE_CLEANUP;
            continue;
        }
        uint8_t character=key;
        if (character<' ' || character>'z') continue;
        if (character>='a') character -= ASCII_CASE_DISTANCE;
        if (campaignInput.textInputLength==campaignInput.textInputMaximumIndex) continue;
        if (character>='a') character -= CAMPAIGN_TEXT_SECOND_CASE_ADJUSTMENT;
        campaignTextInputBuffer[campaignInput.textInputLength++]=character;
        MwEraseTextInputCursor();
        campaignInput.textInputCursorX=EgaDrawCharacter(campaignInput.textInputCursorX,&campaignInput.textInputCursorY,
            character,campaignInput.textInputForeground,campaignInput.textInputBackground);
    }
}

/* MW_MAIN081B:04AD..05FD. Read joystick A (H3: its axes and buttons, where
 * the original counted game-port reads), detect new button presses, then
 * classify the direction by the calibration's thresholds. */
void MwPollJoystickA(void)
{
    if (mwCampaignInput.readJoystick == NULL) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mwCampaignInput.readJoystick == NULL");
    campaignInput.joystickDirection = JOYSTICK_CENTRED;
    campaignInput.joystickDeflection = JOYSTICK_DEFLECTION_NEAR;
    campaignInput.joystickButtonOnePressed = 0; campaignInput.joystickButtonTwoPressed = 0;
    MwJoystickState joystick = mwCampaignInput.readJoystick();
    campaignInput.joystickXCount = MwJoystickAxisReading(joystick.x);
    campaignInput.joystickYCount = MwJoystickAxisReading(joystick.y);
    uint8_t buttons = (uint8_t)((joystick.buttonOne ? 0 : JOYSTICK_BUTTON_ONE_UP) | (joystick.buttonTwo ? 0 : JOYSTICK_BUTTON_TWO_UP));
    if (!(buttons & JOYSTICK_BUTTON_ONE_UP) && (campaignInput.joystickPreviousButtons & JOYSTICK_BUTTON_ONE_UP)) campaignInput.joystickButtonOnePressed = 1;
    if (!(buttons & JOYSTICK_BUTTON_TWO_UP) && (campaignInput.joystickPreviousButtons & JOYSTICK_BUTTON_TWO_UP)) campaignInput.joystickButtonTwoPressed = 1;
    campaignInput.joystickPreviousButtons = buttons;
    uint16_t y = campaignInput.joystickYCount;
    if (campaignInput.joystickXCount < campaignInput.joystickXLow)
    {
        if (y < campaignInput.joystickYLow)
        { campaignInput.joystickDirection = JOYSTICK_UP_LEFT; if (y < campaignInput.joystickYFarLow) goto farDeflection; }
        else if (y > campaignInput.joystickYHigh)
        { campaignInput.joystickDirection = JOYSTICK_DOWN_LEFT; if (y > campaignInput.joystickYFarHigh) goto farDeflection; }
        else campaignInput.joystickDirection = JOYSTICK_LEFT;
        if (campaignInput.joystickXCount < campaignInput.joystickXFarLow) goto farDeflection;
    }
    else if (campaignInput.joystickXCount > campaignInput.joystickXHigh)
    {
        if (y < campaignInput.joystickYLow)
        { campaignInput.joystickDirection = JOYSTICK_UP_RIGHT; if (y < campaignInput.joystickYFarLow) goto farDeflection; }
        else if (y > campaignInput.joystickYHigh)
        { campaignInput.joystickDirection = JOYSTICK_DOWN_RIGHT; if (y > campaignInput.joystickYFarHigh) goto farDeflection; }
        else campaignInput.joystickDirection = JOYSTICK_RIGHT;
        if (campaignInput.joystickXCount > campaignInput.joystickXFarHigh) goto farDeflection;
    }
    else if (y < campaignInput.joystickYLow)
    { campaignInput.joystickDirection = JOYSTICK_UP; if (y < campaignInput.joystickYFarLow) goto farDeflection; }
    else if (y > campaignInput.joystickYHigh)
    { campaignInput.joystickDirection = JOYSTICK_DOWN; if (y > campaignInput.joystickYFarHigh) goto farDeflection; }
    return;
farDeflection:
    campaignInput.joystickDeflection = JOYSTICK_DEFLECTION_FAR;
}
