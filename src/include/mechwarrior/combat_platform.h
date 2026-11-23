#ifndef MECHWARRIOR_COMBAT_PLATFORM_H
#define MECHWARRIOR_COMBAT_PLATFORM_H
#include <stdint.h>
#include <stdio.h>
#include "mechwarrior/joystick.h"
enum { DOS_MAX_PARAGRAPH_ALLOCATION_BYTES=0xFFFF0 };
/* Required low-level host boundaries. No combat/game decision callbacks. A
 * missing binding fails explicitly, never simulates successful calibration. */
extern FILE *(*btechHostOpenStream)(const char *filename,const char *mode);
extern uint8_t (*btechHostReadPort)(uint16_t port);
extern void (*btechHostWritePort)(uint16_t port,uint8_t value);
extern uint8_t (*btechHostSetInterrupts)(uint8_t enabled);
extern uint8_t (*btechHostReadInterrupts)(void);
/* Joystick A (0) or B (1) as it is now (H3). */
extern MwJoystickState (*btechHostReadJoystick)(uint16_t joystick);
/* The test clock's charge for the time a game-port wait took before H3, made
 * where the original waited (H4 removes it); nothing in real time. */
enum { BTECH_GAME_PORT_WAIT_CALIBRATION,BTECH_GAME_PORT_WAIT_AXIS,BTECH_GAME_PORT_WAIT_PRESENCE };
extern void (*btechHostChargeGamePortWait)(uint8_t wait);
extern void (*btechHostPollInterrupts)(void);
/* Host-only pacing at the native 0019 loop boundary: the fixed tick length.
 * Does not add a native argument, game tick, decision or source procedure. */
extern void (*btechHostPaceCombatTick)(void);
/* Host-only test instrumentation at each 0810:2A17 combat input poll, before
 * the BIOS buffer is read (a scripted feed queues its scans here). Optional;
 * no native argument, decision or source procedure. */
extern void (*btechHostNoteCombatInputPoll)(void);
extern uint8_t *(*btechHostAllocateParagraphs)(uint16_t paragraphs,uint16_t *largestFreeParagraphs);
extern void (*btechHostFreeDosAllocation)(uint8_t *allocation);
extern void (*btechHostFreeDosSelector)(uint16_t selector);
/* Query the existing DOS MCB owner; this is not a host-pointer cache key. */
extern uint16_t (*btechHostDosAllocationSelector)(const uint8_t *allocation);
/* Physical foreign-memory edge of the signed, wrapping BA56 directory read.
 * Returns a real recovered byte owner, never a synthetic surface/priority. */
struct BtechPosition3D;
/* Initial visibility-query cache aliases the preceding signed divider's saved
 * BX/SI. A nonzero physical priority proves that D5CE discards that cache.
 * Missing/zero bytes are unresolved, not permission to invent an empty cache. */
typedef struct BtechDosAllocation {uint8_t *bytes;uint32_t availableBytes;} BtechDosAllocation;
extern uint8_t runtimeDelayActive;
extern uint16_t runtimeDelayRemaining;
extern uint8_t joystickPairInstalled[2],joystickAxisCentres[4];
extern uint16_t joystickAxisReciprocals[4];
/* 304E:41D9, 41DD, 41DF: the first axis's value while 1BF5:22F9 reads the
 * second, and the X and Y values 1BF5:2376 tests. */
extern int16_t joystickFirstAxisValue,joystickDirectionX,joystickDirectionY;
extern const char *combatInputReplayFilename;
extern uint16_t combatInputReplayMode;
typedef union BtechCharacterClassStorage {
    struct {uint8_t precedingBytes[0x80],classes[0x100];} fields;
    uint8_t bytes[0x180];
} BtechCharacterClassStorage;
extern BtechCharacterClassStorage characterClassStorage;
#define characterClasses characterClassStorage.fields.classes
FILE *BtechOpenStreamWithRetry(const char *filename,const char *mode);
uint16_t BtechGetActiveRuntimeDelayFar(void);
uint16_t BtechReadJoystickAxisTiming(uint8_t axisMask);
BtechDosAllocation BtechAllocateDosMemoryBlock(uint32_t bytes);
void BtechFreeDosMemorySelector(uint8_t *allocation);
#endif
