#ifndef MECHWARRIOR_CAMPAIGN_STATE_H
#define MECHWARRIOR_CAMPAIGN_STATE_H
#include "mechwarrior/campaign.h"
#include "mechwarrior/campaign_exchange.h"

/* Named owners from the existing campaign/combat exchange schema. Unknown
 * bytes stay neutral; this header does not introduce a packed memory image. */
enum { COMMANDER_INITIAL_AGE_YEARS=0x12 };
/* Campaign state owned by campaign_state.h (plan, slice B2). */
struct CampaignMissionState
{
    uint8_t unidentifiedField18, missionCategoryOrMemoryTier, deploymentLandscapeIndex;
    uint8_t combatEnemyHeavyCount, combatEnemyMediumCount, combatEnemyLightCount;
    uint8_t combatPlanetSelector, combatScenarioCode, deploymentTerrainDetailLevel;
    CampaignSalvageEntry salvageEntries[4];
    uint16_t returnedDisabledEnemyCount;
    uint8_t combatTransitionResult, acceptedContractActive;
    uint8_t unidentifiedField19[2];
    uint8_t commanderNameBytes[0x000A];
    uint16_t commanderAgeYears, houseNetStandings[CAMPAIGN_HOUSE_COUNT];
    uint16_t requestedContractPaymentThousands, requestedContractSalvagePercent;
    uint16_t requestedContractAdvancePercent, requestContractPaymentEnabled;
    uint16_t requestContractSalvageEnabled, requestContractAdvanceEnabled;
    uint8_t unidentifiedField20[6];
    uint16_t acceptedContractPaymentThousands, acceptedContractSalvagePercent;
    uint16_t acceptedContractAdvancePercent, remainingContractPaymentThousands;
    uint8_t audioOptionLabelIndex, campaignTerrainDetailLevel;
    uint16_t acceptedContractTargetHouseIndex, playerSkillProgress;
    uint8_t defaultHouseMissionTypeLimits[0x28], campaignDefaultCommanderNameBytes[0x0A];
};
extern CampaignMissionState campaignMission;
#endif
