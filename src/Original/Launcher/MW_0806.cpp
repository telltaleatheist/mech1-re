#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/launcher.h"
#include "mechwarrior/campaign.h"
#include "mechwarrior/video_mode.h"

MwLauncherSystemCalls mwLauncherSystem;

/* MW0806:09EE..0C25, the loop from 0B9A on. The DOS path before it (resident
 * resize, memory probes, adapter detection and the graphics prompt) and the
 * disk prompts after a failed EXEC are dropped. A child that cannot be started
 * fails loudly with its file name. Native termination is always code00. */
uint8_t MwLauncherMain(uint8_t audioCode)
{
    if (!mwLauncherSystem.executeChild) /* Port failsafe; not an original DOS exit. */
        MW_PORT_FAILSAFE("Launcher system calls are not bound");
    uint8_t childExitCode=0;
    uint8_t phase=CAMPAIGN_PHASE_NEW;
    MwSetVideoMode(MW_VIDEO_MODE_TEXT); /* The text screen cleared by a mode 3 set. */
    for (;;)
    {
        MwCampaignLaunch launch=MwCampaignLaunch{{launcherResident.launcherIdentity,0x07D4,sizeof(launcherResident.launcherIdentity)},
            0x0004,phase,(uint8_t)LAUNCHER_GRAPHICS_EGA,(uint8_t)LAUNCHER_MEMORY_512K};
        if (mwLauncherSystem.executeChild(launcherResident.launcherCampaignFilename,&launch,audioCode,&childExitCode))
            MwFailGameFileRead(launcherResident.launcherCampaignFilename);
        if (childExitCode==0) return 0; /* Any NONZERO AL enters combat; AH ignored. */
        if (mwLauncherSystem.executeChild(launcherResident.launcherCombatFilename,&launch,audioCode,&childExitCode))
            MwFailGameFileRead(launcherResident.launcherCombatFilename);
        if (childExitCode==0) return 0;
        phase=CAMPAIGN_PHASE_COMBAT_RETURN;
    }
}
