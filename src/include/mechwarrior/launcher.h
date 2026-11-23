#ifndef MECHWARRIOR_LAUNCHER_H
#define MECHWARRIOR_LAUNCHER_H
#include "mechwarrior/campaign_startup.h"

/* The launcher keeps only the campaign/combat loop. The DOS path (memory
 * probes, the graphics prompt, the disk prompts) is dropped: the host always
 * runs EGA with the 512K memory tier the probes chose on a full machine. */
enum { LAUNCHER_MEMORY_512K=3,LAUNCHER_GRAPHICS_EGA='E' };
/* Child execution transport. Child lifetime recreation belongs inside
 * executeChild, not in campaign mechanics. true means the child could not be
 * started (native Carry), never success. */
typedef struct MwLauncherSystemCalls
{
    bool (*executeChild)(const char *filename,const MwCampaignLaunch *launch,uint8_t audioCode,uint8_t *exitCode);
} MwLauncherSystemCalls;
extern MwLauncherSystemCalls mwLauncherSystem;
extern uint8_t launcherIdentity[11];
extern char launcherCampaignFilename[12],launcherCombatFilename[10];
void MwImportLauncherInitialData(void);
/* audioCode is MW.EXE's S/T/A/R; the result is the native terminate code. */
uint8_t MwLauncherMain(uint8_t audioCode);
#endif
