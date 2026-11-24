#include "SDLBackend.h"
#include "mechwarrior/launcher.h"
#include "mechwarrior/game_files.h"
#include "mechwarrior/combat_installation.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_platform.h"
#include "mechwarrior/combat_scene.h"
#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_sound.h"
#include "mechwarrior/combat_input.h"
#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_orders.h"
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

/* Host DOS process ownership only. Game objects and allocation decisions stay
 * in Original. Captured640KiB DOSBox-X machine profile: resident program ends
 * at4194's MCB, code starts0956. AH48 uses DOS first fit, split MCB paragraphs
 * and adjacent-free coalescing, not a host-malloc total-byte approximation. */
enum { COMBAT_DOS_FIRST_MCB=0x4194,COMBAT_DOS_CONVENTIONAL_END=0xA000,
    DOS_MCB_PARAGRAPHS=1,DOS_PARAGRAPH_BYTES=16 };
typedef struct DosAllocation {
    struct DosAllocation *next;
    uint8_t *bytes;
    uint16_t paragraphs;
} DosAllocation;
static DosAllocation *allocations;
static char gameDirectory[4096];
static jmp_buf childReturn;
static bool childActive;
static uint8_t childExitCode;
static uint8_t *AllocateParagraphs(uint16_t paragraphs,uint16_t *largestFreeParagraphs)
{
    if (!allocations)
    {
        allocations=static_cast<DosAllocation *>(malloc(sizeof(*allocations)));
        if (!allocations) {*largestFreeParagraphs=0;return NULL;}
        *allocations=DosAllocation{NULL,NULL,COMBAT_DOS_CONVENTIONAL_END-COMBAT_DOS_FIRST_MCB-DOS_MCB_PARAGRAPHS};
    }
    *largestFreeParagraphs=0;
    DosAllocation *allocation=NULL;
    for (DosAllocation *block=allocations;block;block=block->next)
        if (!block->bytes)
        {
            if (block->paragraphs>*largestFreeParagraphs) *largestFreeParagraphs=block->paragraphs;
            if (!allocation && block->paragraphs>=paragraphs) allocation=block;
        }
    if (!allocation) return NULL;
    uint8_t *bytes=static_cast<uint8_t *>(malloc(paragraphs?(size_t)paragraphs*DOS_PARAGRAPH_BYTES:1));
    if (!bytes) {*largestFreeParagraphs=0;return NULL;}
    if (allocation->paragraphs>paragraphs)
    {
        DosAllocation *remaining=static_cast<DosAllocation *>(malloc(sizeof(*remaining)));
        if (!remaining) {free(bytes);*largestFreeParagraphs=0;return NULL;}
        *remaining=DosAllocation{allocation->next,NULL,(uint16_t)(allocation->paragraphs-paragraphs-DOS_MCB_PARAGRAPHS)};
        allocation->next=remaining;
    }
    allocation->paragraphs=paragraphs;allocation->bytes=bytes;
    *largestFreeParagraphs=paragraphs; /* Successful DOS AH48 leaves caller BX. */
    return allocation->bytes;
}
static void FreeDosAllocation(uint8_t *bytes)
{
    /* Native AH49 selector0 fails without freeing anything.1F10 discards CF;
     *228B releases empty bank slots too. Do not turn that DOS error into a
     * host failure or free a fabricated allocation. */
    if (!bytes) return;
    DosAllocation *allocation=allocations;
    while (allocation && allocation->bytes!=bytes) allocation=allocation->next;
    if (!allocation) MwSdlFail("Free of unknown BTECH DOS allocation");
    free(allocation->bytes);allocation->bytes=NULL;
    for (DosAllocation *block=allocations;block;)
    {
        if (!block->bytes && block->next && !block->next->bytes)
        {
            DosAllocation *next=block->next;
            block->paragraphs+=(uint16_t)(DOS_MCB_PARAGRAPHS+next->paragraphs);
            block->next=next->next;free(next);
        }
        else block=block->next;
    }
}
static void ReleaseChildDosAllocations(void)
{
    while (allocations)
    {DosAllocation *block=allocations;allocations=block->next;free(block->bytes);free(block);}
}
uint16_t MwSdlCombatDosAllocationCount(void)
{
    uint16_t count=0;
    for (const DosAllocation *block=allocations;block;block=block->next) if (block->bytes) ++count;
    return count;
}
static FILE *OpenStream(const char *filename,const char *mode)
{
    /* Original resources are flat installation files. Never change the shared
     * working directory or let a native filename escape the installation. */
    if (!filename[0] || strpbrk(filename,"/\\:")) return NULL;
    return fopen(mw1o::files::find_game_file(gameDirectory,filename,__func__).string().c_str(),mode);
}
static void WriteStderr(const char *text,uint16_t bytes)
{
    if (fwrite(text,1,bytes,stderr)!=bytes) MwSdlFail("BTECH stderr write failed");
    fflush(stderr);
}
static void TerminateChild(uint8_t exitCode)
{
    if (!childActive) MwSdlFail("BTECH termination outside EXEC");
    printf("Combat return: DOS exit=%u result=%u tick=%u objective=%u completion=%u/%u protectedTargetRemaining=%u playerValue=%d movementMode=%u lastScan=%02X\n",
        exitCode,campaignResultExchange.result,combatStartup.combatTickCounter,combatImport.sideObjectiveRules[0],
        combatStartup.sideCompletionFlags[0],combatStartup.sideCompletionFlags[1],combatWorld.specialTargetDamageRemaining,
        combatMechs.unitControls[0].conditionScoreOrExitCode,combatMechs.unitControls[0].movementMode,combatInput.combatInputScanCode);
    fflush(stdout);
    childExitCode=exitCode;
    longjmp(childReturn,1); /* Actual DOS termination never returns to E144. */
}
bool MwSdlInitializeCombatProcess(const char *directory)
{
    if (strlen(directory)>=sizeof(gameDirectory)) return false;
    strcpy(gameDirectory,directory);
    btechHostOpenStream=OpenStream;btechHostAllocateParagraphs=AllocateParagraphs;
    btechHostFreeDosAllocation=FreeDosAllocation;btechHostReleaseChildDosAllocations=ReleaseChildDosAllocations;
    btechHostWriteStderr=WriteStderr;btechHostTerminateChild=TerminateChild;
    return true;
}
bool MwSdlExecuteCombatChild(const MwCampaignLaunch *launch,uint8_t audioCode,uint8_t *exitCode)
{
    if (childActive) MwSdlFail("Nested BTECH EXEC");
    if (!BtechHostLoadInitialData()) return true;
    MwSdlResetCombatHardware();mwSdlCombatActive=true;
    MwSdlUpdateWindowTitle();
    /* MW.EXE's DOS tail is03,<graphics>,20,<audio>,0D. Phase/transfer live in
     * its private FCB; they are NOT an extra C command-line argument. */
    char graphics[2]={(char)launch->graphicsCode,0},audio[2]={(char)audioCode,0};
    const char *arguments[]={"BTECH.EXE",graphics,audio};
    childActive=true;
    if (setjmp(childReturn)==0)
    {
        BtechProgramMain(3,arguments);
        MwSdlFail("Native BTECH main unexpectedly returned");
    }
    childActive=false;mwSdlCombatActive=false;
    MwSdlUpdateWindowTitle();
    *exitCode=childExitCode;
    BtechHostDiscardChildData();
    return false;
}
