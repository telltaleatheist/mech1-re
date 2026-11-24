#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_heap.h"
#include "mechwarrior/native_address.h"
#include "mechwarrior/port_diagnostics.h"
#include <stdio.h>
#include <string.h>
CombatRuntimeState combatRuntime;
CombatNearHeapState combatNearHeap={
    .nearHeapProgramBreak=BTECH_NEAR_INITIAL_BREAK,
    .nearHeapGrowthAlignment=BTECH_NEAR_GROWTH_ALIGNMENT
};

bool BtechImportRuntimeInitialData(const uint8_t *data,size_t bytes)
{
    if (!data || bytes<0x4EDC) return false;
    BtechHostDiscardNearHeap();
    uint16_t callback=MwReadNativeWord(data,0x4DFA);
    if (callback!=0 && callback!=0xFE80) return false;
    if (MwReadNativeWord(data,0x4BA8) || MwReadNativeWord(data,0x4DF0)) return false;
    combatRuntime.runtimeExitCallbacks[0]=callback?BtechFlushAllRuntimeStreams:NULL;
    /* FE80's returned flush count is unobserved by E1CD. */
    combatRuntime.runtimeDiagnosticCallback=NULL;combatRuntime.runtimeDeviceCleanup=NULL;
    for (uint16_t word=0;word<4;++word) combatRuntime.runtimePrefix.ambientFallbackWords[word]=MwReadNativeWord(data,word*2);
    memcpy(combatRuntime.runtimePrefix.copyrightBytes,data+8,sizeof(combatRuntime.runtimePrefix.copyrightBytes));
    combatRuntime.runtimeSavedDivideOwner=MwDivideOwner::System; /*4B4D: 0000:0000*/
    /* 4B8A/4B8B: the CRT's extra hooked vector, inactive (0) in both
     * revisions, so its restore at exit is gone (H4c). An image with it
     * active names a vector this port has no owner for. */
    if (data[0x4B8A])
    {
        char reason[96];snprintf(reason,sizeof(reason),"The image's extra CRT vector at 304E:4B8A is active: bytes %02X %02X",data[0x4B8A],data[0x4B8B]);
        MW_PORT_FAILSAFE(reason);
    }
    combatRuntime.runtimeHandleLimit=MwReadNativeWord(data,0x4B66);
    if (combatRuntime.runtimeHandleLimit!=COMBAT_DOS_HANDLES) return false;
    memcpy(combatRuntime.runtimeHandleFlags,data+0x4B68,sizeof(combatRuntime.runtimeHandleFlags));
    for (uint16_t slot=0;slot<COMBAT_STREAM_SLOTS;++slot)
    {
        combatRuntime.runtimeTemporaryFileIds[slot]=0; /*4C64*/
        combatRuntime.runtimeStreamAuxiliaryFlags[slot]=data[0x4C60+slot*6];
    }
    uint16_t cursor=0x4E0A;
    for (uint16_t message=0;message<COMBAT_RUNTIME_MESSAGES;++message)
    {
        BtechRuntimeMessage *entry=&combatRuntime.runtimeMessages[message];
        entry->code=MwReadNativeWord(data,cursor);cursor+=2;
        const uint8_t *end=static_cast<const uint8_t *>(memchr(data+cursor,0,bytes-cursor));
        if (!end || (size_t)(end-data-cursor)>=sizeof(entry->text)) return false;
        memcpy(entry->text,data+cursor,(size_t)(end-data-cursor)+1);cursor=(uint16_t)(end-data+1);
        if (entry->code==UINT16_MAX) return message==COMBAT_RUNTIME_MESSAGES-1;
    }
    return false;
}
