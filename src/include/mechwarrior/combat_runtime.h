#ifndef MECHWARRIOR_COMBAT_RUNTIME_H
#define MECHWARRIOR_COMBAT_RUNTIME_H
#include <stdio.h>
#include "mechwarrior/combat_sound.h"

enum { COMBAT_RUNTIME_MESSAGES=8,
    COMBAT_RUNTIME_CHECKSUM_BYTES=0x42,COMBAT_RUNTIME_CHECKSUM_XOR=0x55,
    COMBAT_RUNTIME_ERROR_EXIT=0xFF,COMBAT_RUNTIME_DEVICE_EXIT=2,
    COMBAT_RUNTIME_MESSAGE_NEWLINE=0xFC,COMBAT_RUNTIME_MESSAGE_PREFIX=0xFF,
    COMBAT_RUNTIME_MESSAGE_NULL_ASSIGNMENT=1,COMBAT_EXIT_DELAY_UNITS=0x3C,COMBAT_SHUTDOWN_SOUND_EVENTS=0x0F };
typedef uint16_t (*BtechRuntimeCallback)(void);
typedef struct BtechRuntimeMessage {uint16_t code;char text[0x40];} BtechRuntimeMessage;
/* The native checksum observes these exact66 bytes. The initial four WORDs
 * are also ambient audio fallback data, not a second zero/null owner. */
typedef struct BtechRuntimePrefix {
    uint16_t ambientFallbackWords[4];
    uint8_t copyrightBytes[COMBAT_RUNTIME_CHECKSUM_BYTES-8];
} BtechRuntimePrefix;
/* Missing ITM node0 dereferences these SAME authored prefix WORDs. */
#define missingSoundFallbackOffsetAlias (combatRuntime.runtimePrefix.ambientFallbackWords[0])
#define missingSoundFallbackSelectorAlias (combatRuntime.runtimePrefix.ambientFallbackWords[1])
/* Combat state owned by combat_runtime.h (plan, slice B3a). */
struct CombatRuntimeState
{
    BtechRuntimePrefix runtimePrefix;
    BtechRuntimeCallback runtimeExitCallbacks[1], runtimeDiagnosticCallback;
    void (*runtimeDeviceCleanup)(uint16_t operation);
    MwDivideOwner runtimeSavedDivideOwner;
    BtechRuntimeMessage runtimeMessages[COMBAT_RUNTIME_MESSAGES];
};
extern CombatRuntimeState combatRuntime;
/* Actual DOS IO/process boundaries. Termination must leave the child (the
 * launcher may implement that by a nonlocal return); it must not return here. */
extern void (*btechHostWriteStderr)(const char *text,uint16_t bytes);
extern void (*btechHostTerminateChild)(uint8_t exitCode);
bool BtechImportRuntimeInitialData(const uint8_t *data,size_t bytes);
int16_t BtechRuntimeStreamClose(BtechRuntimeStream *stream);
uint16_t BtechStreamIdentity(const BtechRuntimeStream *stream);
void BtechCloseRuntimeStreamFiles(void);
size_t BtechReadStreamElements(void *destination,size_t size,size_t count,FILE *file);
int BtechReadStreamByte(FILE *file);
int BtechCloseStreamFile(FILE *file);
void BtechRuntimeStreamAllocateBuffer(BtechRuntimeStream *stream);
void BtechRuntimeStreamReleaseBuffer(BtechRuntimeStream *stream);
int16_t BtechRuntimeStreamFlush(BtechRuntimeStream *stream);
uint16_t BtechFlushAllRuntimeStreams(void);
void BtechRunNearCallbacksReverse(BtechRuntimeCallback *callbacks,uint16_t count);
void BtechRunFarCallbacksReverse(BtechRuntimeCallback *callbacks,uint16_t count);
const char *BtechFindRuntimeMessageByCode(uint16_t messageCode);
void BtechWriteRuntimeMessageToStderr(uint16_t messageCode);
void BtechReportRuntimeFatalState(void);
uint16_t BtechCheckRuntimeDataChecksum(void);
void BtechRestoreRuntimeVectors(void);
void BtechRuntimeExitAndCloseHandles(uint16_t exitCode);
void BtechTerminateToLauncher(uint16_t quitWholeGame);
#endif
