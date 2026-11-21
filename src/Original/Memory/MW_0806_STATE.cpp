#include "mechwarrior/launcher.h"
#include <string.h>

/* Native resident exchange outlives both child executables. Only transport
 * owns these encoded bytes; campaign and combat use their own typed objects. */
MwCampaignTransferStorage campaignTransfer;
uint8_t launcherIdentity[11];
char launcherCampaignFilename[12],launcherCombatFilename[10];

/* Installation boundary only. No retained launcher code/segment image. The
 * launcher's DOS messages (0x085A..0x09E9) are no longer imported. */
bool MwImportLauncherInitialData(const uint8_t *data,size_t byteCount)
{
    if (!data || byteCount<0x09EE) return false;
    memcpy(campaignCombatExchange,data+0x0004,sizeof(campaignCombatExchange));
    memcpy(launcherIdentity,data+0x07D4,sizeof(launcherIdentity));
    memcpy(launcherCampaignFilename,data+0x07E3,sizeof(launcherCampaignFilename));
    memcpy(launcherCombatFilename,data+0x07EF,sizeof(launcherCombatFilename));
    return launcherCampaignFilename[11]==0 && launcherCombatFilename[9]==0;
}
