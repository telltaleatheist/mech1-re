#ifndef MECHWARRIOR_HARDWARE_PORTS_H
#define MECHWARRIOR_HARDWARE_PORTS_H
#include <stdint.h>
/* One physical MPU and timer shared by campaign and combat, not game-specific IDs. */
enum { MPU_DATA_PORT=0x330,MPU_STATUS_COMMAND_PORT=0x331 };
enum { ADLIB_INDEX_PORT=0x388,ADLIB_DATA_PORT=0x389 };

enum { PIT_COUNTER_MODULUS=0x10000,BIOS_TICKS_PER_DAY=0x1800B0 };

/* The device IRQs passed to their owners (H4c), where the games wrote the
 * PIC's mask port: a set of the host's two devices (the MPU's left with S8), all on at start as the
 * BIOS left them. No end-of-interrupt is sent: the host has no PIC. */
enum class MwDeviceIrq : uint8_t { Timer=0x01,Keyboard=0x02,All=0x03 };

/* Who owns each interrupt the games hook (H4c), where the original got and
 * set FAR vectors. Previous/System is whoever owned it before the game hooked
 * it: for IRQ0 the handler that ran before combat installed its own, for the
 * others the host's own handling. IRQ2's (the MPU's) left with S8. */
enum class MwTimerOwner : uint8_t { Previous,CombatScheduler,CombatMusic };
enum class MwKeyboardOwner : uint8_t { System,Combat };
enum class MwDivideOwner : uint8_t { System,Combat };
#endif
