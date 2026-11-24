#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_startup.h"
#include <stdlib.h>

/*1BF5:1F4A: the horizon's and the clippers' divide that overflows a word. The original installed this as the divide
 * fault's handler (its owner, 1BF5's install and cleanup); its result is the dividend's SAR DX/RCR AX, which the callers
 * divide again. B5: a plain call where the divide would overflow, as step 0 measured it in play (the combat traces'
 * horizon: 14 and 36 each). Called with the handler not installed, the original would have crashed: a failsafe. The handler's
 * owner itself stays (1BF5:1F20, 1F84, 0810:E1A0): the tone fallback reads INT00's vector, so it is sound until S8. */
uint32_t runtime_1989::horizon_divide_fallback(uint32_t dividend)
{
    if (!combatStartup.divideErrorHookMarkedActive) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !divideErrorHookMarkedActive");
    return (dividend>>1)|(dividend&UINT32_C(0x80000000));
}
