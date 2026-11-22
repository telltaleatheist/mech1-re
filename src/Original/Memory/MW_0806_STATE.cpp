#include "mechwarrior/launcher.h"
#include "mechwarrior/native_start.h"
#include <string.h>

/* Native resident exchange outlives both child executables. Only transport
 * owns these encoded bytes; campaign and combat use their own typed objects. */
MwCampaignTransferStorage campaignTransfer;
uint8_t launcherIdentity[11];
char launcherCampaignFilename[12],launcherCombatFilename[10];

/* The resident data's start (MW.EXE 0806), set in code: MW.EXE is not read.
 * The exchange (0004..07D3) is zero in the exe. Of the identity (07D4, 11
 * bytes) MW_MAIN tests three, at 07D4, 07D8 and 07DE (MwRunCampaign, 081B),
 * so those are set to what it tests and the rest, never read, are zero. The
 * children's names are code (native_start.h, checked by start_exe_test). */
void MwImportLauncherInitialData(void)
{
    memset(campaignCombatExchange,0,sizeof(campaignCombatExchange)); /*0004*/
    memset(launcherIdentity,0,sizeof(launcherIdentity));
    launcherIdentity[0]='M';launcherIdentity[4]='W';launcherIdentity[10]='R';
    memcpy(launcherCampaignFilename,mwLauncherCampaignFilename,sizeof(launcherCampaignFilename)); /*07E3*/
    memcpy(launcherCombatFilename,mwLauncherCombatFilename,sizeof(launcherCombatFilename)); /*07EF*/
}
