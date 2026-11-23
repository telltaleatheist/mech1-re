#include "SDLBackend.h"
#include "mechwarrior/campaign_input.h"
#include "mechwarrior/combat_input.h"
#include <stdio.h>
#include <stdlib.h>

/* Keys reach the games as typed events (H2): the campaign's presses wait in a
 * queue of 16 slots (15 keys, as the BIOS ring its key reads used held), and
 * combat's presses and releases in one of 16 events until its handler takes
 * them. No BIOS keyboard data, scan port or IRQ1 is modelled. */
enum { HOST_KEY_QUEUE_SLOTS=16,HOST_DEFAULT_DRIVE_C=2 };
static MwCampaignKey keys[HOST_KEY_QUEUE_SLOTS];
static uint8_t readIndex,writeIndex;
static bool lastControl,quitRequested;
static uint32_t captureNumber;
static uint16_t mouseReleaseCounts[MOUSE_BUTTON_COUNT];
static float mouseXMotion,mouseYMotion;
static MwKeyEvent combatKeys[HOST_KEY_QUEUE_SLOTS];
static uint8_t combatKeyRead,combatKeyWrite;
bool MwSdlCombatKeyboardPending(void) {return mwSdlCombatActive && combatKeyRead!=combatKeyWrite;}
/* One combat key event to the game, where the IRQ1 was delivered before H2. */
void MwSdlDeliverCombatKeyEvent(void)
{
    if (!MwSdlCombatKeyboardPending()) MwSdlFail("No combat key event to deliver");
    if (!btechHostGetInterruptVector) MwSdlFail("Missing keyboard vector binding");
    MwKeyEvent event=combatKeys[combatKeyRead];
    uint32_t vector=btechHostGetInterruptVector(KEYBOARD_INTERRUPT);
    if (vector==BTECH_KEYBOARD_HANDLER_VECTOR) BtechHandleKeyEvent(event);
    else if (!vector)
    {
        /* After 2C8D restores the system handler, an ordinary release changes
         * nothing the game sees and is dropped. Makes and modifier, lock and
         * Insert releases would need the BIOS's keyboard; none is converted. */
        if (!event.released || !event.key || event.key>=COMBAT_KEYBOARD_SCAN_CODES
            || event.key==COMBAT_KEY_CONTROL || event.key==COMBAT_KEY_LEFT_SHIFT || event.key==COMBAT_KEY_RIGHT_SHIFT || event.key==COMBAT_KEY_ALT
            || event.key==COMBAT_KEY_CAPS_LOCK || event.key==COMBAT_KEY_NUM_LOCK || event.key==COMBAT_KEY_SCROLL_LOCK || event.key==COMBAT_KEY_INSERT || event.key==COMBAT_KEY_SYSTEM_REQUEST)
            MwSdlFail("Unconverted key event without combat's keyboard handler");
    }
    else MwSdlFail("Unmapped native keyboard vector");
    combatKeyRead=(combatKeyRead+1)%HOST_KEY_QUEUE_SLOTS;
}
static void QueueCombatKey(uint8_t key,bool released)
{
    uint8_t next=(combatKeyWrite+1)%HOST_KEY_QUEUE_SLOTS;
    if (next!=combatKeyRead) {combatKeys[combatKeyWrite]=MwKeyEvent{key,released};combatKeyWrite=next;}
}
void MwSdlQueueCombatKey(uint8_t key,bool released) {if(mwSdlCombatActive)QueueCombatKey(key,released);}

void MwSdlQueueCampaignKey(MwCampaignKey key)
{
    if (getenv("MECHWARRIOR_DIAGNOSTICS"))
    {fprintf(stderr,"Key=%04X control=%d time=%llu\n",MwCampaignKeyWord(key),key.control,(unsigned long long)SDL_GetTicks());fflush(stderr);}
    uint8_t next=(writeIndex+1)%HOST_KEY_QUEUE_SLOTS;
    if (next==readIndex) return; /* The queue is full; the key is dropped. */
    keys[writeIndex]=key;writeIndex=next;
}
void MwSdlRequestQuit(void) {quitRequested=true;}
static uint8_t KeyNumber(SDL_Scancode scan)
{
    static const struct {SDL_Scancode scan;uint8_t bios;} map[]={
        {SDL_SCANCODE_ESCAPE,COMBAT_KEY_ESCAPE},{SDL_SCANCODE_RETURN,COMBAT_KEY_ENTER},{SDL_SCANCODE_KP_ENTER,COMBAT_KEY_ENTER},
        {SDL_SCANCODE_SPACE,COMBAT_KEY_SPACE},{SDL_SCANCODE_TAB,COMBAT_KEY_TAB},{SDL_SCANCODE_BACKSPACE,COMBAT_KEY_BACKSPACE},
        {SDL_SCANCODE_LCTRL,COMBAT_KEY_CONTROL},{SDL_SCANCODE_RCTRL,COMBAT_KEY_CONTROL},{SDL_SCANCODE_LSHIFT,COMBAT_KEY_LEFT_SHIFT},{SDL_SCANCODE_RSHIFT,COMBAT_KEY_RIGHT_SHIFT},
        {SDL_SCANCODE_LALT,COMBAT_KEY_ALT},{SDL_SCANCODE_RALT,COMBAT_KEY_ALT},{SDL_SCANCODE_CAPSLOCK,COMBAT_KEY_CAPS_LOCK},{SDL_SCANCODE_NUMLOCKCLEAR,COMBAT_KEY_NUM_LOCK},
        {SDL_SCANCODE_MINUS,COMBAT_KEY_MINUS},{SDL_SCANCODE_EQUALS,COMBAT_KEY_EQUALS},{SDL_SCANCODE_KP_MINUS,COMBAT_KEY_KEYPAD_MINUS},{SDL_SCANCODE_KP_PLUS,COMBAT_KEY_KEYPAD_PLUS},
        {SDL_SCANCODE_COMMA,COMBAT_KEY_COMMA},{SDL_SCANCODE_PERIOD,COMBAT_KEY_PERIOD},{SDL_SCANCODE_SLASH,COMBAT_KEY_SLASH},
        {SDL_SCANCODE_UP,COMBAT_KEY_UP},{SDL_SCANCODE_DOWN,COMBAT_KEY_DOWN},{SDL_SCANCODE_LEFT,COMBAT_KEY_LEFT},{SDL_SCANCODE_RIGHT,COMBAT_KEY_RIGHT},
        {SDL_SCANCODE_HOME,COMBAT_KEY_HOME},{SDL_SCANCODE_END,COMBAT_KEY_END},{SDL_SCANCODE_PAGEUP,COMBAT_KEY_PAGE_UP},{SDL_SCANCODE_PAGEDOWN,COMBAT_KEY_PAGE_DOWN},
        {SDL_SCANCODE_KP_7,COMBAT_KEY_HOME},{SDL_SCANCODE_KP_8,COMBAT_KEY_UP},{SDL_SCANCODE_KP_9,COMBAT_KEY_PAGE_UP},
        {SDL_SCANCODE_KP_4,COMBAT_KEY_LEFT},{SDL_SCANCODE_KP_5,COMBAT_KEY_KEYPAD_STOP},{SDL_SCANCODE_KP_6,COMBAT_KEY_RIGHT},
        {SDL_SCANCODE_KP_1,COMBAT_KEY_END},{SDL_SCANCODE_KP_2,COMBAT_KEY_DOWN},{SDL_SCANCODE_KP_3,COMBAT_KEY_PAGE_DOWN},
        {SDL_SCANCODE_A,COMBAT_KEY_A},{SDL_SCANCODE_B,COMBAT_KEY_B},{SDL_SCANCODE_C,COMBAT_KEY_C},{SDL_SCANCODE_D,COMBAT_KEY_D},
        {SDL_SCANCODE_E,COMBAT_KEY_E},{SDL_SCANCODE_F,COMBAT_KEY_F},{SDL_SCANCODE_G,COMBAT_KEY_G},{SDL_SCANCODE_H,COMBAT_KEY_H},
        {SDL_SCANCODE_I,COMBAT_KEY_I},{SDL_SCANCODE_J,COMBAT_KEY_J},{SDL_SCANCODE_K,COMBAT_KEY_K},{SDL_SCANCODE_L,COMBAT_KEY_L},
        {SDL_SCANCODE_M,COMBAT_KEY_M},{SDL_SCANCODE_N,COMBAT_KEY_N},{SDL_SCANCODE_O,COMBAT_KEY_O},{SDL_SCANCODE_P,COMBAT_KEY_P},
        {SDL_SCANCODE_Q,COMBAT_KEY_Q},{SDL_SCANCODE_R,COMBAT_KEY_R},{SDL_SCANCODE_S,COMBAT_KEY_S},{SDL_SCANCODE_T,COMBAT_KEY_T},
        {SDL_SCANCODE_U,COMBAT_KEY_U},{SDL_SCANCODE_V,COMBAT_KEY_V},{SDL_SCANCODE_W,COMBAT_KEY_W},{SDL_SCANCODE_X,COMBAT_KEY_X},
        {SDL_SCANCODE_Y,COMBAT_KEY_Y},{SDL_SCANCODE_Z,COMBAT_KEY_Z},{SDL_SCANCODE_1,COMBAT_KEY_1},{SDL_SCANCODE_2,COMBAT_KEY_2},
        {SDL_SCANCODE_3,COMBAT_KEY_3},{SDL_SCANCODE_4,COMBAT_KEY_4},{SDL_SCANCODE_5,COMBAT_KEY_5},{SDL_SCANCODE_6,COMBAT_KEY_6},
        {SDL_SCANCODE_7,COMBAT_KEY_7},{SDL_SCANCODE_8,COMBAT_KEY_8},{SDL_SCANCODE_9,COMBAT_KEY_9},{SDL_SCANCODE_0,COMBAT_KEY_0},
        {SDL_SCANCODE_F1,COMBAT_KEY_F1},{SDL_SCANCODE_F2,COMBAT_KEY_F2},{SDL_SCANCODE_F3,COMBAT_KEY_F3},{SDL_SCANCODE_F4,COMBAT_KEY_F4},
        {SDL_SCANCODE_F5,COMBAT_KEY_F5},{SDL_SCANCODE_F6,COMBAT_KEY_F6},{SDL_SCANCODE_F7,COMBAT_KEY_F7},{SDL_SCANCODE_F8,COMBAT_KEY_F8},
        {SDL_SCANCODE_F9,COMBAT_KEY_F9},{SDL_SCANCODE_F10,COMBAT_KEY_F10}};
    for (uint16_t i=0;i<sizeof(map)/sizeof(map[0]);++i) if (map[i].scan==scan) return map[i].bios;
    return 0;
}
/* The campaign key a press gives: its number, character and Control. */
static bool CampaignKey(SDL_Scancode scancode,SDL_Keycode keycode,SDL_Keymod mod,MwCampaignKey *key)
{
    uint8_t scan=KeyNumber(scancode);if (!scan) return false;
    uint8_t ascii=keycode<ASCII_CHARACTER_LIMIT?(uint8_t)keycode:0;
    if ((mod&SDL_KMOD_NUM) && scancode>=SDL_SCANCODE_KP_1 && scancode<=SDL_SCANCODE_KP_9)
        ascii=(uint8_t)('1'+scancode-SDL_SCANCODE_KP_1);
    if (scan==COMBAT_KEY_ENTER) ascii='\r';
    if (mod&SDL_KMOD_CTRL) {if (ascii>='a' && ascii<='z') ascii-=('a'-1);}
    else if (((mod&SDL_KMOD_SHIFT)!=0)^((mod&SDL_KMOD_CAPS)!=0))
    {if (ascii>='a' && ascii<='z') ascii-=ASCII_CASE_DISTANCE;}
    if (mod&SDL_KMOD_ALT) ascii=0;
    *key=MwCampaignKey{scan,ascii,(mod&SDL_KMOD_CTRL)!=0};
    return true;
}
bool MwSdlCampaignKeyForScancode(SDL_Scancode scancode,SDL_Keymod mod,MwCampaignKey *key)
{return CampaignKey(scancode,SDL_GetKeyFromScancode(scancode,mod,false),mod,key);}
/* Joysticks A and B (H3): the first two connected, in the order they came.
 * A gamepad gives its left stick and its south and east buttons; another
 * joystick its first two axes and buttons. The test clock's runs open none,
 * so the traces never see a controller the machine happens to have. */
static SDL_Joystick *joysticks[2];
static SDL_Gamepad *gamepads[2];
static bool joysticksEnabled;
static void OpenJoystick(SDL_JoystickID id)
{
    for (unsigned slot=0;slot<2;++slot)
        if ((joysticks[slot] && SDL_GetJoystickID(joysticks[slot])==id) || (gamepads[slot] && SDL_GetGamepadID(gamepads[slot])==id)) return;
    for (unsigned slot=0;slot<2;++slot)
    {
        if (joysticks[slot] || gamepads[slot]) continue;
        if (SDL_IsGamepad(id)) gamepads[slot]=SDL_OpenGamepad(id);
        else
        {
            joysticks[slot]=SDL_OpenJoystick(id);
            if (joysticks[slot] && (SDL_GetNumJoystickAxes(joysticks[slot])<2 || SDL_GetNumJoystickButtons(joysticks[slot])<1))
            {SDL_CloseJoystick(joysticks[slot]);joysticks[slot]=NULL;} /* Not a joystick the games could read. */
        }
        return;
    }
}
static void CloseJoystick(SDL_JoystickID id)
{
    for (unsigned slot=0;slot<2;++slot)
    {
        if (gamepads[slot] && SDL_GetGamepadID(gamepads[slot])==id) {SDL_CloseGamepad(gamepads[slot]);gamepads[slot]=NULL;}
        if (joysticks[slot] && SDL_GetJoystickID(joysticks[slot])==id) {SDL_CloseJoystick(joysticks[slot]);joysticks[slot]=NULL;}
    }
}
static int8_t AxisValue(Sint16 value)
{
    int scaled=(int)value*MW_JOYSTICK_AXIS_FULL/32767;
    return (int8_t)(scaled<-MW_JOYSTICK_AXIS_FULL?-MW_JOYSTICK_AXIS_FULL:scaled);
}
/* The joystick events of the event loop: a device plugged in takes the first
 * free slot, one taken out frees its slot (and reads as not connected). */
bool MwSdlHandleJoystickEvent(const SDL_Event &event)
{
    if (event.type==SDL_EVENT_JOYSTICK_ADDED) {OpenJoystick(event.jdevice.which);return true;}
    if (event.type==SDL_EVENT_JOYSTICK_REMOVED) {CloseJoystick(event.jdevice.which);return true;}
    return false;
}
/* Off under the test clock; otherwise the gamepad subsystem, its devices
 * arriving as events. */
bool MwSdlInitializeJoysticks(void)
{
    joysticksEnabled=!MwSdlVirtualClock() && SDL_InitSubSystem(SDL_INIT_GAMEPAD);
    return joysticksEnabled;
}
MwJoystickState MwSdlReadJoystick(uint16_t joystick)
{
    MwJoystickState state={false,0,0,false,false};
    if (joystick>1 || !joysticksEnabled) return state;
    SDL_UpdateJoysticks();
    if (SDL_Gamepad *gamepad=gamepads[joystick])
    {
        state=MwJoystickState{true,AxisValue(SDL_GetGamepadAxis(gamepad,SDL_GAMEPAD_AXIS_LEFTX)),
            AxisValue(SDL_GetGamepadAxis(gamepad,SDL_GAMEPAD_AXIS_LEFTY)),
            SDL_GetGamepadButton(gamepad,SDL_GAMEPAD_BUTTON_SOUTH),SDL_GetGamepadButton(gamepad,SDL_GAMEPAD_BUTTON_EAST)};
    }
    else if (SDL_Joystick *stick=joysticks[joystick])
    {
        state=MwJoystickState{true,AxisValue(SDL_GetJoystickAxis(stick,0)),AxisValue(SDL_GetJoystickAxis(stick,1)),
            SDL_GetJoystickButton(stick,0),SDL_GetNumJoystickButtons(stick)>1 && SDL_GetJoystickButton(stick,1)};
    }
    return state;
}
void MwSdlPollEvents(void)
{
    MwSdlPollDebugControl();
    MwSdlPollSupportPilot();
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if(event.type==SDL_EVENT_KEY_DOWN && event.key.scancode==SDL_SCANCODE_F11)
        {MwSdlStopSupportPilot();continue;}
        if(event.type==SDL_EVENT_KEY_DOWN && (event.key.scancode==SDL_SCANCODE_P || event.key.scancode==SDL_SCANCODE_ESCAPE))
            MwSdlStopSupportPilot(); /* Explicit manual pause/retreat always takes over. */
        if (event.type==SDL_EVENT_QUIT) {MwSdlRequestQuit();continue;}
        if (MwSdlHandleJoystickEvent(event)) continue;
        if (event.type==SDL_EVENT_WINDOW_EXPOSED) {MwSdlPresentVideo(true);continue;}
        if (event.type==SDL_EVENT_WINDOW_SHOWN || event.type==SDL_EVENT_WINDOW_RESIZED
            || event.type==SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED || event.type==SDL_EVENT_WINDOW_MAXIMIZED
            || event.type==SDL_EVENT_WINDOW_RESTORED || event.type==SDL_EVENT_WINDOW_FOCUS_GAINED
            || event.type==SDL_EVENT_WINDOW_DISPLAY_CHANGED || event.type==SDL_EVENT_WINDOW_ENTER_FULLSCREEN
            || event.type==SDL_EVENT_WINDOW_LEAVE_FULLSCREEN)
        {MwSdlRepeatPresent();continue;}
        if (event.type==SDL_EVENT_MOUSE_BUTTON_UP)
        {
            uint16_t button=event.button.button==SDL_BUTTON_LEFT?MOUSE_BUTTON_LEFT:
                event.button.button==SDL_BUTTON_RIGHT?MOUSE_BUTTON_RIGHT:UINT16_MAX;
            if (button<MOUSE_BUTTON_COUNT && mouseReleaseCounts[button]!=UINT16_MAX) ++mouseReleaseCounts[button];
            continue;
        }
        if (event.type==SDL_EVENT_MOUSE_MOTION)
        {
            float logicalXMotion,logicalYMotion;
            MwSdlConvertMouseMotion(event.motion.xrel,event.motion.yrel,&logicalXMotion,&logicalYMotion);
            mouseXMotion+=logicalXMotion;mouseYMotion+=logicalYMotion;
            continue;
        }
        if (mwSdlCombatActive && (event.type==SDL_EVENT_KEY_DOWN || event.type==SDL_EVENT_KEY_UP)
            && (event.key.scancode==SDL_SCANCODE_MINUS || event.key.scancode==SDL_SCANCODE_EQUALS))
        {
            /* The host's speed keys: main-row - and =/+ (BTECH reads neither;
             * its keypad - and + select weapons and stay the game's). */
            if (event.type==SDL_EVENT_KEY_DOWN && !event.key.repeat)
                MwSdlStepCombatSpeed(event.key.scancode==SDL_SCANCODE_EQUALS?1:-1);
            continue;
        }
        if (mwSdlCombatActive && (event.type==SDL_EVENT_KEY_DOWN || event.type==SDL_EVENT_KEY_UP) && event.key.scancode!=SDL_SCANCODE_F12)
        {
            uint8_t key=KeyNumber(event.key.scancode);
            if (key) QueueCombatKey(key,event.type==SDL_EVENT_KEY_UP);
            continue;
        }
        if (event.type!=SDL_EVENT_KEY_DOWN) continue;
        if (event.key.scancode==SDL_SCANCODE_F12)
        {
            char stem[48];snprintf(stem,sizeof(stem),"capture-%u",captureNumber++);
            if (!MwSdlCapture(stem)) fprintf(stderr,"Capture failed or file already exists.\n");
            continue;
        }
        MwCampaignKey key;
        if (CampaignKey(event.key.scancode,event.key.key,event.key.mod,&key)) MwSdlQueueCampaignKey(key);
    }
    if (quitRequested)
    {
        if (mwSdlCombatActive) {QueueCombatKey(COMBAT_KEY_CONTROL,false);QueueCombatKey(COMBAT_KEY_ESCAPE,false);quitRequested=false;}
        else if (readIndex==writeIndex)
            MwSdlQueueCampaignKey(MwCampaignKey{(uint8_t)(CAMPAIGN_KEY_ESCAPE>>8),(uint8_t)CAMPAIGN_KEY_ESCAPE,true});
    }
}
static bool Available(void) {MwSdlPump();MwSdlDebugNoteInputPoll(readIndex==writeIndex,false);return readIndex!=writeIndex;}
static MwCampaignKey ReadKey(void)
{
    while (readIndex==writeIndex) {MwSdlPump();MwSdlDebugNoteInputPoll(true,true);}
    MwCampaignKey key=keys[readIndex];lastControl=key.control;readIndex=(readIndex+1)%HOST_KEY_QUEUE_SLOTS;
    MwSdlDebugNoteKeyRead(MwCampaignKeyWord(key));
    return key;
}
static bool ControlHeld(void)
{return lastControl || (SDL_GetModState()&SDL_KMOD_CTRL)!=0;}
static uint16_t MouseRelease(uint16_t button)
{
    MwSdlPump();
    if (button>=MOUSE_BUTTON_COUNT) return 0;
    uint16_t count=mouseReleaseCounts[button];mouseReleaseCounts[button]=0;return count;
}
static void MouseReset(void)
{mouseReleaseCounts[MOUSE_BUTTON_LEFT]=mouseReleaseCounts[MOUSE_BUTTON_RIGHT]=0;mouseXMotion=mouseYMotion=0;}
static void MouseMotion(uint16_t *x,uint16_t *y)
{
    MwSdlPump();
    int16_t xMotion=(int16_t)mouseXMotion,yMotion=(int16_t)mouseYMotion;
    mouseXMotion-=xMotion;mouseYMotion-=yMotion;
    *x=(uint16_t)xMotion;*y=(uint16_t)yMotion;
}
static MwJoystickState ReadJoystickA(void) {return MwSdlReadJoystick(JOYSTICK_A);}
static uint8_t CurrentDrive(void) {return HOST_DEFAULT_DRIVE_C;}
void MwSdlInitializeInput(void)
{
    readIndex=writeIndex=0;lastControl=quitRequested=false;captureNumber=0;MouseReset();
    combatKeyRead=combatKeyWrite=0;
    MwSdlInitializeDebugControl();
    MwSdlInitializeJoysticks();
    mwCampaignInput=MwCampaignInputCalls{MouseRelease,Available,ReadKey,ReadJoystickA,
        MwSdlSetInterrupts,MwSdlPump,CurrentDrive,MouseReset,MouseMotion,ControlHeld,
        MwSdlSetMousePosition,MwSdlReadMousePosition};
}
