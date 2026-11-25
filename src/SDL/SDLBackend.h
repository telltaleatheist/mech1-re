#ifndef MECHWARRIOR_SDL_BACKEND_H
#define MECHWARRIOR_SDL_BACKEND_H
#include "mechwarrior/key_events.h"
#include "mechwarrior/joystick.h"
#include "mechwarrior/sdl_host.h"
#include "mechwarrior/campaign_music.h"
#include "mechwarrior/launcher.h"
#include <SDL3/SDL.h>
enum { HOST_PIT_CLOCK_HZ=1193182,HOST_AUDIO_SAMPLE_RATE=48000 };
/* music= (docs/phase5/NO_EMULATION_PLAN.md): the rebuild's songs from the
 * soundfont (src/Audio/Music), the speaker's on the tone voice, or none. The
 * effects play on the tone voice whichever it is. */
typedef enum MwHostMusic {MW_MUSIC_SPEAKER,MW_MUSIC_SOUNDFONT,MW_MUSIC_OFF} MwHostMusic;
extern MwHostMusic mwSdlMusic;
/* Captured DOS machine profile, shared by physical aliases and first-fit MCBs. */
enum { HOST_DOS_COMBAT_CODE_SELECTOR=0x0956 };
[[noreturn]] void MwSdlFail(const char *operation);
void MwSdlInitializeInput(void);
void MwSdlPollEvents(void);
void MwSdlInitializeDebugControl(void);
void MwSdlPollDebugControl(void);
bool MwSdlDebugPinnedBiosTicks(uint32_t *ticks);
void MwSdlDebugNoteInputPoll(bool empty,bool blocking);
bool MwSdlCampaignKeyForScancode(SDL_Scancode scancode,SDL_Keymod mod,MwCampaignKey *key);
void MwSdlDebugNoteKeyRead(uint16_t key);
void MwSdlDebugNoteCombatPoll(void);
void MwSdlShutdownDebugControl(void);
/* Optional host automation, never enabled by build configuration alone. */
bool MwSdlConfigureSupportPilot(const char *directory);
void MwSdlPollSupportPilot(void);
void MwSdlUpdateSupportPilot(void);
void MwSdlStopSupportPilot(void);
void MwSdlQueueCombatKey(uint8_t key,bool released);
void MwSdlQueueCampaignKey(MwCampaignKey key);
void MwSdlInitializeTimer(void);
/* Test clock (MECHWARRIOR_DEBUG_VIRTUAL_CLOCK): time advances only where the
 * host would wait, so the PIT timeline depends on the input alone. */
bool MwSdlVirtualClock(void);
uint64_t MwSdlNowNanoseconds(void);
void MwSdlAudioLogSnap(const char *name);
void MwSdlAudioLogTone(double frequency); /* Both in the tone log (MECHWARRIOR_AUDIO_LOG); a snap is a mark. */
void MwSdlPumpTimer(void);
/* Combat runs one tick per fixed length, combat_tick_ms divided by the speed
 * step (docs/timing/PLAN.md). The logic reads no clock: only the wait changes. */
enum { MW_COMBAT_SPEED_MIN=1,MW_COMBAT_SPEED_MAX=4 };
extern uint32_t mwSdlCombatTickMs;
extern uint8_t mwSdlCombatSpeed;
uint64_t MwSdlCombatTickNanoseconds(void);
void MwSdlStepCombatSpeed(int direction);
bool MwSdlCombatSpeedNotice(char *text,size_t size);
void MwSdlResetCombatTick(void);
void MwSdlPaceCombatTick(void);
bool MwSdlLoadTimingConfiguration(const char *path,bool optional);
/* The player's MechWarrior.ini in the write directory: overrides the shipped
 * one, and holds what the player changes in play (the combat speed). */
bool MwSdlResolveWriteDirectory(char *directory,size_t size);
/* Our data/ folder: --data's, or data beside the program (Contents/Resources on the Mac); "" when unknown. */
const char *MwSdlDataDirectory(void);
bool MwSdlLoadUserConfiguration(void);
void MwSdlSaveUserSetting(const char *key,unsigned value);
void MwSdlSaveUserText(const char *key,const char *value);
/* game_folder: the player's MW1 files, set up once by the setup screen when
 * the program starts without them (an installed copy, double-clicked). */
extern char mwSdlGameFolder[4096];
void MwSdlAllowNestedTicks(void);
void MwSdlInstallCampaignInterrupts(void);
void MwSdlRestoreCampaignInterrupts(void);
uint32_t MwSdlReadBiosTicks(void);
double MwSdlHardwareTime(void);
double MwSdlTimerLagMilliseconds(void);
uint32_t MwSdlQueuedAudioMilliseconds(void);
uint64_t MwSdlLateAudioFrames(void);
uint64_t MwSdlAudioUnderrunFrames(void);
uint64_t MwSdlAudioClockSkippedFrames(void);
bool MwSdlInitializeVideo(const char *captureDirectory);
const char *MwSdlCaptureDirectory(void);
void MwSdlPresentVideo(bool force);
/* Show the current frame again at the next pump (a window event). */
void MwSdlRepeatPresent(void);
void MwSdlShutdownVideo(void);
void MwSdlSetMousePosition(uint16_t x,uint16_t y); /* The frame's pixels. */
void MwSdlReadMousePosition(uint16_t *x,uint16_t *y);
void MwSdlConvertMouseMotion(float windowXMotion,float windowYMotion,
    float *logicalXMotion,float *logicalYMotion);
bool MwSdlInitializeAudio(void);
void MwSdlAdvanceAudio(double pitTime);
void MwSdlPlayTone(double frequency);
void MwSdlSetTimerPeriod(uint32_t pitCycles);
void MwSdlShutdownAudio(void);
/* Combat is running (its frames are on the canvas like the campaign's). */
extern bool mwSdlCombatActive;
void MwSdlRunDue(void);
void MwSdlWaitIdle(void);
MwMusicTimerHandler MwSdlReadTimerHandler(void);
void MwSdlWriteTimerHandler(MwMusicTimerHandler handler);
void MwSdlEnableDeviceIrqs(MwDeviceIrq irqs);
void MwSdlInitializeCombatHardware(void);
void MwSdlResetCombatHardware(void);
bool MwSdlInitializeCombatProcess(const char *directory);
bool MwSdlExecuteCombatChild(const MwCampaignLaunch *launch,uint8_t audioCode,uint8_t *exitCode);
bool MwSdlCombatKeyboardPending(void);
uint16_t MwSdlCombatDosAllocationCount(void);
void MwSdlDeliverCombatKeyEvent(void);
MwJoystickState MwSdlReadJoystick(uint16_t joystick);
bool MwSdlInitializeJoysticks(void);
bool MwSdlHandleJoystickEvent(const SDL_Event &event);
#endif
