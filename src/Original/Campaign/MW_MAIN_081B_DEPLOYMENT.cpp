#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/campaign_continuations.h"
#include "mechwarrior/campaign_handoff.h"
#include "mechwarrior/campaign_startup.h"
#include "mechwarrior/campaign_travel.h"
#include "mechwarrior/campaign_ui.h"
#include <stdlib.h>
#include <string.h>

/* MW_MAIN081B:1423..144D. The original asked for the combat disk until the
 * combat program (BTECH.EXE) opened, then closed it unread. The combat
 * program is ours (slice 6d) and play opens no exe (slice 6f, the guard in
 * find_game_file), so there is no disk to wait for: the open and close
 * are dropped, an intended difference (docs/phase2/PLAN.md 8.5, 6f). The
 * handle was closed at once, so nothing after it sees the difference. */
void MwWaitForRequiredResourceFile(void)
{
}

/* MW_MAIN081B:1C3E..1CCB. Readiness falls here; POP AX discards the callback
 * return before JMP1CD4/015F. Return codes below transport those JMPs only. */
uint16_t MwResolveDeploymentOrExportCombatExchangeContinuation(void)
{
    MwPlayHiredTransportAnimation();
    /* Native REP STOSB clears0451..0467, NOT the accepted flag0468. */
    memset(campaignMechanics.rosterStatus,0,sizeof(campaignMechanics.rosterStatus));
    memset(campaignMission.salvageEntries,0,sizeof(campaignMission.salvageEntries));
    campaignMission.returnedDisabledEnemyCount=0;
    campaignMission.combatTransitionResult=0;
    if ((int8_t)campaignMission.combatScenarioCode<=DEPLOYMENT_BYPASS_LAST_SCENARIO
        && campaignMission.combatScenarioCode!=DEPLOYMENT_NO_BYPASS_SCENARIO)
    {
        if (MwRandomUpTo(PERCENT_DIVISOR)<=DEPLOYMENT_BYPASS_ROLL_LIMIT)
        {
            for (uint16_t slot=0;slot<CAMPAIGN_SALVAGE_SLOTS;++slot)
            {
                campaignMission.salvageEntries[slot].unitKind=0xFF;
                campaignMission.salvageEntries[slot].leadingZeroByte=0xFF; /* Native WORD FFFF. */
            }
            campaignMission.acceptedContractActive=0;
            campaignHandoff.deploymentResolvedWithoutCombat=1;
            return CAMPAIGN_CONTINUE_AFTER_ACTION;
        }
    }
    MwWaitForRequiredResourceFile();
    campaignMission.missionCategoryOrMemoryTier=(uint8_t)campaignStartup.launcherMemoryTier;
    campaignMission.deploymentTerrainDetailLevel=campaignMission.campaignTerrainDetailLevel;
    /* Real launcher and child allocations are disjoint. Encode at their
     * transfer boundary; do not fabricate a live1090 image for this copy.
     * Native copies forward0718 bytes to resident0806:0004; no cursor wraps
     * for that actual allocation. Arbitrary aliased foreign PSPs are not a
     * supported launcher configuration, nor silently treated as memcpy. */
    MwEncodeCampaignExchange(campaignCombatExchange);
    campaignStartup.launcherExitCode=CAMPAIGN_EXIT_COMBAT;
    return CAMPAIGN_CONTINUE_CLEANUP;
}
