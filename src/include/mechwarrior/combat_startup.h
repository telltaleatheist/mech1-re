#ifndef MECHWARRIOR_COMBAT_STARTUP_H
#define MECHWARRIOR_COMBAT_STARTUP_H
#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_world.h"
#include <stdbool.h>
#include "mechwarrior/hardware_ports.h"

enum { COMBAT_SCREEN_COCKPIT=0,BIOS_VIDEO_EQUIPMENT=0x10,PIC_MASK_PORT=0x21 };

enum { COMBAT_SCHEDULER_INVALID_SLOT_BITS=0xF0,COMBAT_SCHEDULER_MAX_POSITIVE_PERIOD=0xFF,
    BIOS_ROM_SELECTOR=0xF000,BIOS_MODEL_OFFSET=0xFFFE,BIOS_PCJR_MODEL_BYTE=0xFF,
    TANDY_ROM_SIGNATURE_OFFSET=0xC000,TANDY_ROM_SIGNATURE_BYTE=0x21,
    BIOS_EQUIPMENT_CLEAR_DISPLAY_BITS=0xCF,BIOS_EQUIPMENT_DISPLAY_SHIFT=4,
    BIOS_TEXT_MODE_80_COLUMN=3,VIDEO_ADAPTER_UNSAVED=0xFF };
enum { PLAYER_SCREEN_INSPECTION=1,PLAYER_SCREEN_ORDERS=2,PLAYER_SCREEN_TARGET_INSPECTION=3 };
enum { COMBAT_FRAME_DELAY_TICKS=18,COMBAT_OUTCOME_DELAY_TICKS=20,
    COMBAT_LOSS_FLASHES=20,COMBAT_LOSS_FLASH_DELAY=6,COMBAT_LOSS_SOUND=5,COMBAT_LOSS_SOUND_PRIORITY=100,
    COMBAT_LOSS_COLOUR_A=Combat_Pal_EGA_LightRed,COMBAT_LOSS_COLOUR_B=Combat_Pal_EGA_Yellow };
extern uint8_t aiUpdatesSuppressed,unidentifiedField22;
void BtechProgramMain(uint16_t argumentCount,const char *const *arguments);
void BtechFlashPlayerMechLoss(void);
extern uint16_t combatTickCounter,playerTargetSlot,selectedWeaponMount,aimReticleScreenY;
extern int16_t heatAimJitterTiers[COMBAT_MECH_SLOTS];
extern uint16_t radarRangeShift,viewScaleShift,requestedPlayerForwardStepScale,unidentifiedField21;
extern uint16_t playerSpecialTargetLandingLatch;
extern uint8_t mountGroupMode,headingRadarSelected,playerImpactThisTick;
extern uint16_t combatDispatchState;
extern uint8_t firingIndicatorBank0Pending,firingIndicatorBank1Pending,sideCompletionFlags[2];
extern BtechPooledWorldObject *unitLastDrawParts[COMBAT_MECH_SLOTS];
extern BtechWorldObjectPrefix *selectedHitObject;
extern uint8_t criticalErrorHandlerInstalled,keyboardHandlersInstalled,mouseDriverInstalled;
extern uint8_t divideErrorHookMarkedActive,videoAdapter;
extern MwDivideOwner savedDivideErrorOwner;
/* Writable native CS storage belongs to1BF5, NOT to304E. */
extern MwKeyboardOwner savedKeyboardOwner;
extern MwTimerOwner savedTimerOwner;
extern uint16_t pitChannel0DivisorShadow;
extern uint16_t schedulerPeriod,schedulerCountdown,schedulerActiveSlots;
enum { COMBAT_SCHEDULER_SLOTS=0x10 };
typedef void (*BtechTimerCallback)(void);
typedef struct BtechSchedulerTiming {uint16_t ticksRemaining,period;} BtechSchedulerTiming;
extern BtechTimerCallback schedulerCallbacks[COMBAT_SCHEDULER_SLOTS];
extern BtechSchedulerTiming schedulerTimings[COMBAT_SCHEDULER_SLOTS];
/* The tail to IRQ0's saved owner, not an audio/game callback seam. */
extern void (*btechHostChainTimerInterrupt)(MwTimerOwner owner);
/* The timer's period (H4b), where PIT channel 0 was programmed: in the PIT's
 * clock units; PIT_COUNTER_MODULUS is the BIOS's 18.2 Hz. */
extern void (*btechHostSetTimerPeriod)(uint32_t pitCycles);
uint16_t BtechRegisterTimerCallback(BtechTimerCallback callback,uint16_t period);
uint16_t BtechUnregisterTimerCallback(uint16_t callbackHandle);
void BtechTimerSchedulerInterrupt(void);
enum { COMBAT_KEYBOARD_SCAN_CODES=0x59,COMBAT_KEYBOARD_COMPATIBILITY_CODES=0x0B };
extern uint8_t keyboardCapsLockRequested,keyboardBiosIntercept,keyboardScanTranslations[COMBAT_KEYBOARD_SCAN_CODES];
extern uint8_t keyboardScanMask,keyboardExtendedScanMask;
extern bool (*btechHostQueryKeyboardIntercept)(uint16_t ax); /* INT15 -> carry. */
/* Interrupt ownership (H4c), where the original got and set vectors. */
extern MwTimerOwner (*btechHostGetTimerOwner)(void);
extern void (*btechHostSetTimerOwner)(MwTimerOwner owner);
extern MwKeyboardOwner (*btechHostGetKeyboardOwner)(void);
extern void (*btechHostSetKeyboardOwner)(MwKeyboardOwner owner);
extern MwDivideOwner (*btechHostGetDivideOwner)(void);
extern void (*btechHostSetDivideOwner)(MwDivideOwner owner);
extern MwMpuIrqOwner (*btechHostGetMpuIrqOwner)(void);
extern void (*btechHostSetMpuIrqOwner)(MwMpuIrqOwner owner);
extern uint16_t (*btechHostReadBiosWord)(uint16_t offset);
extern void (*btechHostWriteBiosWord)(uint16_t offset,uint16_t value);
extern uint8_t (*btechHostReadBiosByte)(uint16_t offset);
extern void (*btechHostWriteBiosByte)(uint16_t offset,uint8_t value);
extern void (*btechHostResetMouse)(void);
bool BtechImportStartupInitialData(const uint8_t *data,size_t bytes);
void BtechResetCombatState(void);
/* Native EGA1E84 returns after actual4176 briefing/input and26C5 cockpit
 * setup. Main0019 mission loop and live SDL child entry remain separate. */
void BtechInitializeCombatRuntime(void);
extern uint16_t playerMountFireFlags[COMBAT_WEAPON_MOUNTS];
void BtechInstallDosCriticalErrorHandler(void);
void BtechRestoreDosCriticalErrorHandler(void);
void BtechShutdownPlatformHandlers(void);
void BtechRestoreTextVideoMode(void);
void BtechSetBiosTextModeAndAdapterBits(uint16_t displayBits);
void BtechRestoreKeyboardAndTickVectors(void);
void BtechShutdownMouseDriver(void);
void BtechRestoreTimerScheduler(void);
void BtechDeactivateDivideErrorHook(void);
void BtechInstallKeyboardAndTickHandlers(void);
void BtechInstallDivideErrorHandler(void);
void BtechInstallTimerScheduler(uint16_t period);
int16_t BtechProbePcjrTandyRomSignatures(void);
#endif
