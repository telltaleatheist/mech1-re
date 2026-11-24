#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_heap.h"
#include "mechwarrior/btech_start_image.h"
#include "mechwarrior/port_diagnostics.h"
#include <stdio.h>
#include <string.h>
CombatRuntimeState combatRuntime;
CombatNearHeapState combatNearHeap={
    .nearHeapProgramBreak=BTECH_NEAR_INITIAL_BREAK,
    .nearHeapGrowthAlignment=BTECH_NEAR_GROWTH_ALIGNMENT
};

bool BtechImportRuntimeInitialData(void)
{
    const BtechStartRowData runtimeMessagesRow("runtime.messages");
    const BtechStartRowData runtimePrefixRow("runtime.prefix");
    const BtechStartRowData runtimeExtraVectorRow("runtime.extra_vector");
    const BtechStartRowData runtimeSignedDivisionRow("runtime.signed_division");
    const BtechStartRowData runtimeExitCallbacksRow("runtime.exit_callbacks");
    const BtechStartRowData runtimeExitFlushRow("runtime.exit_flush");
    BtechHostDiscardNearHeap();
    uint16_t callback=runtimeExitFlushRow.word();
    if (callback!=0 && callback!=0xFE80) return false;
    if (runtimeSignedDivisionRow.word() || runtimeExitCallbacksRow.word()) return false;
    combatRuntime.runtimeExitCallbacks[0]=callback?BtechFlushAllRuntimeStreams:NULL;
    /* FE80's returned flush count is unobserved by E1CD. */
    combatRuntime.runtimeDiagnosticCallback=NULL;combatRuntime.runtimeDeviceCleanup=NULL;
    for (uint16_t word=0;word<4;++word) combatRuntime.runtimePrefix.ambientFallbackWords[word]=runtimePrefixRow.word(word*2);
    runtimePrefixRow.copy(combatRuntime.runtimePrefix.copyrightBytes,sizeof(combatRuntime.runtimePrefix.copyrightBytes),8);
    combatRuntime.runtimeSavedDivideOwner=MwDivideOwner::System; /*4B4D: 0000:0000*/
    /* 4B8A/4B8B: the CRT's extra hooked vector, inactive (0) in both
     * revisions, so its restore at exit is gone (H4c). An image with it
     * active names a vector this port has no owner for. */
    if (runtimeExtraVectorRow.byte())
    {
        char reason[96];snprintf(reason,sizeof(reason),"The image's extra CRT vector at 304E:4B8A is active: bytes %02X %02X",runtimeExtraVectorRow.byte(),runtimeExtraVectorRow.byte(1));
        MW_PORT_FAILSAFE(reason);
    }
    size_t cursor=0;
    for (uint16_t message=0;message<COMBAT_RUNTIME_MESSAGES;++message)
    {
        BtechRuntimeMessage *entry=&combatRuntime.runtimeMessages[message];
        entry->code=runtimeMessagesRow.word(cursor);cursor+=2;
        const char *text=runtimeMessagesRow.text(cursor);
        size_t textBytes=strlen(text)+1;
        if (textBytes>sizeof(entry->text)) return false;
        memcpy(entry->text,text,textBytes);cursor+=textBytes;
        if (entry->code==UINT16_MAX) return message==COMBAT_RUNTIME_MESSAGES-1;
    }
    return false;
}
