#ifndef MECHWARRIOR_HARDWARE_PORTS_H
#define MECHWARRIOR_HARDWARE_PORTS_H
#include <stdint.h>
/* One physical PIT/PIC shared by campaign and combat, not game-specific IDs. */
enum { PIC_COMMAND_PORT=0x20,PIC_END_OF_INTERRUPT=0x20,
    MPU_DATA_PORT=0x330,MPU_STATUS_COMMAND_PORT=0x331 };

enum { PIC_TIMER_IRQ_MASK=0x01,PIC_KEYBOARD_IRQ_MASK=0x02,
    COMBAT_PIC_DEVICE_MASK=0x28, /* Original install AND restore mask, not a saved mask. */
    PIT_COUNTER_MODULUS=0x10000,
    BIOS_TICKS_PER_DAY=0x1800B0,BIOS_TIMER_TICKS_LOW=0x6C,BIOS_TIMER_TICKS_HIGH=0x6E };

/* Who owns each interrupt the games hook (H4c), where the original got and
 * set FAR vectors. Previous/System is whoever owned it before the game hooked
 * it: for IRQ0 the handler that ran before combat installed its own, for the
 * others the host's own handling. IRQ2's owner is shared by campaign and
 * combat, and outlives a combat that doesn't restore it. */
enum class MwTimerOwner : uint8_t { Previous,CombatScheduler,CombatMusic };
enum class MwKeyboardOwner : uint8_t { System,Combat };
enum class MwDivideOwner : uint8_t { System,Combat };
enum class MwMpuIrqOwner : uint8_t { System,CombatProbe,CombatReceive,CampaignReceive };
#endif
