#include "mechwarrior/combat_platform.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_sound.h"
#include "mechwarrior/combat_navigation.h"
#include "mechwarrior/combat_effects.h"
#include "mechwarrior/combat_scene.h"
#include "mechwarrior/combat_heap.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_render.h"
#include <stdlib.h>
MwNativeByteView (*btechHostBorrowDosMemory)(uint16_t);
FILE *(*btechHostOpenStream)(const char *,const char *);
void (*btechHostRunDue)(void);
void (*btechHostWaitIdle)(void);
MwJoystickState (*btechHostReadJoystick)(uint16_t);
void (*btechHostPollInterrupts)(void);
void (*btechHostPlayTone)(double);
void (*btechHostPaceCombatTick)(void);
void (*btechHostNoteCombatInputPoll)(void);
void (*btechHostRunPreviousTimerOwner)(void);
void (*btechHostEnableDeviceIrqs)(MwDeviceIrq);
void (*btechHostSetTimerPeriod)(uint32_t);
uint8_t *(*btechHostAllocateParagraphs)(uint16_t,uint16_t *);
void (*btechHostFreeDosAllocation)(uint8_t *);
void (*btechHostFreeDosSelector)(uint16_t);
uint16_t (*btechHostDosAllocationSelector)(const uint8_t *);

MwTimerOwner (*btechHostGetTimerOwner)(void);
void (*btechHostSetTimerOwner)(MwTimerOwner);
MwKeyboardOwner (*btechHostGetKeyboardOwner)(void);
void (*btechHostSetKeyboardOwner)(MwKeyboardOwner);
MwDivideOwner (*btechHostGetDivideOwner)(void);
void (*btechHostSetDivideOwner)(MwDivideOwner);
void (*btechHostWriteBiosWord)(uint16_t,uint16_t);
uint8_t (*btechHostReadBiosByte)(uint16_t);
void (*btechHostWriteBiosByte)(uint16_t,uint8_t);
void (*btechHostResetMouse)(void);
void (*btechHostWriteStderr)(const char *,uint16_t);
void (*btechHostTerminateChild)(uint8_t);
uint16_t (*btechHostQueryEgaInfo)(void);
uint8_t (*btechHostReadRomByte)(CombatRomByte);
bool (*btechHostQueryKeyboardIntercept)(uint16_t);
bool (*btechHostQuerySystemConfiguration)(uint16_t);
