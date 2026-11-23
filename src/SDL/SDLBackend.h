#ifndef MECHWARRIOR_SDL_BACKEND_H
#define MECHWARRIOR_SDL_BACKEND_H
#include "mechwarrior/sdl_host.h"
#include "mechwarrior/campaign_music.h"
#include "mechwarrior/launcher.h"
#include <SDL3/SDL.h>
enum { HOST_PIT_CLOCK_HZ=1193182,HOST_AUDIO_SAMPLE_RATE=48000 };
typedef enum MwHostSoundSystem {MW_SOUND_SPEAKER,MW_SOUND_MT32,MW_SOUND_ADLIB} MwHostSoundSystem;
extern MwHostSoundSystem mwSdlSoundSystem;
extern char mwSdlMt32ControlRom[4096],mwSdlMt32PcmRom[4096];
void MwMpuReset(void);
void MwMpuWritePort(uint16_t port,uint8_t value);
uint8_t MwMpuReadPort(uint16_t port);
/* Captured DOS machine profile, shared by physical aliases and first-fit MCBs. */
enum { HOST_DOS_COMBAT_CODE_SELECTOR=0x0956 };
[[noreturn]] void MwSdlFail(const char *operation);
void MwSdlInitializeInput(void);
void MwSdlPollEvents(void);
void MwSdlInitializeDebugControl(void);
void MwSdlPollDebugControl(void);
bool MwSdlDebugPinnedBiosTicks(uint32_t *ticks);
void MwSdlDebugNoteInputPoll(bool empty,bool blocking);
bool MwSdlBiosKeyForScancode(SDL_Scancode scancode,SDL_Keymod mod,uint16_t *word,uint8_t *shift);
void MwSdlDebugNoteKeyRead(uint16_t key);
void MwSdlDebugNoteCombatPoll(void);
void MwSdlShutdownDebugControl(void);
/* Optional host automation, never enabled by build configuration alone. */
bool MwSdlConfigureSupportPilot(const char *directory);
void MwSdlPollSupportPilot(void);
void MwSdlUpdateSupportPilot(void);
void MwSdlStopSupportPilot(void);
void MwSdlQueueCombatScan(uint8_t scan);
void MwSdlQueueBiosKey(uint16_t key,uint8_t shift);
void MwSdlInitializeTimer(void);
/* Test clock (MECHWARRIOR_DEBUG_VIRTUAL_CLOCK): time advances only where the
 * host would wait, so the PIT timeline depends on the input alone. */
bool MwSdlVirtualClock(void);
uint64_t MwSdlNowNanoseconds(void);
void MwSdlSpendNanoseconds(uint64_t nanoseconds);
void MwSdlAudioLogSnap(const char *name); /* A trace snap, as a mark in the sound-port log. */
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
void MwSdlDelayLoops(uint16_t loops);
bool MwSdlLoadTimingConfiguration(const char *path,bool optional);
/* The player's MechWarrior.ini in the write directory: overrides the shipped
 * one, and holds what the player changes in play (the combat speed). */
bool MwSdlResolveWriteDirectory(char *directory,size_t size);
bool MwSdlLoadUserConfiguration(void);
void MwSdlSaveUserSetting(const char *key,unsigned value);
void MwSdlSaveUserText(const char *key,const char *value);
/* game_folder: the player's MW1 folder, chosen once with a folder dialog when
 * the program starts without one (an installed copy, double-clicked). */
extern char mwSdlGameFolder[4096];
void MwSdlSetInterrupts(bool enabled);
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
void MwSdlUpdateWindowTitle(void);
void MwSdlPresentVideo(bool force);
void MwSdlShutdownVideo(void);
uint8_t MwSdlGetVideoMode(void);
void MwSdlSetVideoMode(uint8_t mode);
void MwSdlSetMousePosition(uint16_t horizontalPosition,uint16_t verticalPosition);
void MwSdlReadMousePosition(uint16_t *horizontalPosition,uint16_t *verticalPosition);
void MwSdlConvertMouseMotion(float windowXMotion,float windowYMotion,
    float *logicalXMotion,float *logicalYMotion);
bool MwSdlInitializeAudio(void);
void MwSdlAdvanceAudio(double pitTime);
void MwSdlWriteSpeakerPort(uint16_t port,uint8_t value);
uint8_t MwSdlReadSpeakerControl(void);
void MwSdlShutdownAudio(void);
/* Shared physical EGA adapter. Game rasterizers remain in Original. */
extern bool mwSdlCombatActive;
void MwSdlBindCombatVideo(void);
void MwSdlResetCombatVideoHardware(void);
bool MwSdlWriteCombatVideoPort(uint16_t port,uint8_t value);
bool MwSdlReadCombatVideoPort(uint16_t port,uint8_t *value);
uint16_t MwSdlCombatDisplayByteOffset(void);
void MwSdlWriteDevicePort(uint16_t port,uint8_t value);
uint8_t MwSdlReadDevicePort(uint16_t port);
uint8_t MwSdlExchangeInterrupts(uint8_t enabled);
uint8_t MwSdlReadInterrupts(void);
void MwSdlDeliverCombatMpuInterrupt(void);
double MwMpuNextEventTime(void);
void MwMpuAdvance(double time);
MwMusicTimerHandler MwSdlReadTimerHandler(void);
void MwSdlWriteTimerHandler(MwMusicTimerHandler handler);
uint32_t MwSdlBiosTicksValue(void);
void MwSdlInitializeCombatHardware(void);
void MwSdlResetCombatHardware(void);
bool MwSdlInitializeCombatProcess(const char *directory);
bool MwSdlExecuteCombatChild(const MwCampaignLaunch *launch,uint8_t audioCode,uint8_t *exitCode);
bool MwSdlCombatKeyboardPending(void);
uint16_t MwSdlCombatDosAllocationCount(void);
void MwSdlDeliverCombatKeyboardInterrupt(void);
uint8_t MwSdlReadKeyboardScanByte(void);
#endif
