#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_platform.h"
#include "mechwarrior/combat_sound.h"
#include <stdlib.h>

/*0810:CC6F successful-open branch.2777's missing-disk screen/retry is game
 * code, not a host disk-prompt callback. Fail explicitly until it is linked. */
FILE *BtechOpenStreamWithRetry(const char *filename,const char *mode)
{
    BtechRuntimeStream *stream=BtechRuntimeStreamOpen(filename,mode);
    if (!stream) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !stream");
    return stream->file;
}

/*0810:293C/2953. IRQ delivery is the host boundary, not a replacement delay. */
void BtechWaitScaledRuntimeDelay(uint16_t delayUnits)
{
    combatPlatform.runtimeDelayRemaining=(uint16_t)(delayUnits<<2);
    while (BtechGetActiveRuntimeDelayFar())
    {
        if (!btechHostPollInterrupts) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostPollInterrupts");
        btechHostPollInterrupts();
    }
}
uint16_t BtechGetActiveRuntimeDelayFar(void)
{
    return combatPlatform.runtimeDelayActive?combatPlatform.runtimeDelayRemaining:0;
}

/*0810:29CD. Filename changes even when requested00 leaves the old mode. */
void BtechConfigureCombatInputReplay(uint16_t requestedMode,const char *filename)
{
    combatPlatform.combatInputReplayFilename=filename;
    if (!requestedMode) return;
    combatPlatform.combatInputReplayMode=requestedMode;
    if (requestedMode==COMBAT_INPUT_RECORD) BtechCloseStreamFile(BtechOpenStreamWithRetry(filename,"wb"));
}

/*0810:EAA0.2CBC sign-extends the command BYTE. Negative characters address
 * the preceding authored bytes, not an unsigned-ASCII classification entry. */
uint16_t BtechLowercaseCharacterByClassTable(uint16_t character)
{
    uint16_t index=character+0x80;
    if (index>=sizeof(combatPlatform.characterClassStorage.bytes)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: index>=sizeof(characterClassStorage.bytes)");
    return combatPlatform.characterClassStorage.bytes[index]&1?character+0x20:character;
}
