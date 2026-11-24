/* GAM/resident transfer boundary only. Offsets are native0448-relative; never
 * use this encoded buffer as live game state. Generated from canonical
 * owners by Generate-CampaignSaveCodec.py, checked for exact0718 coverage. */
#include "mechwarrior/campaign_options.h"
#include "mechwarrior/campaign_economy.h"
#include <string.h>

void MwEncodeCampaignExchange(uint8_t bytes[CAMPAIGN_SAVE_BYTES])
{
    bytes[0x0000]=unidentifiedField18;
    bytes[0x0001]=missionCategoryOrMemoryTier;
    bytes[0x0002]=deploymentLandscapeIndex;
    bytes[0x0003]=combatEnemyHeavyCount;
    bytes[0x0004]=combatEnemyMediumCount;
    bytes[0x0005]=combatEnemyLightCount;
    bytes[0x0006]=combatPlanetSelector;
    bytes[0x0007]=combatScenarioCode;
    bytes[0x0008]=deploymentTerrainDetailLevel;
    memcpy(bytes+0x0009,campaignMechanics.rosterStatus,sizeof(campaignMechanics.rosterStatus));
    memcpy(bytes+0x000D,salvageEntries,sizeof(salvageEntries));
    bytes[0x001D]=(uint8_t)returnedDisabledEnemyCount;
    bytes[0x001D+1]=(uint8_t)(returnedDisabledEnemyCount>>8);
    bytes[0x001F]=combatTransitionResult;
    bytes[0x0020]=acceptedContractActive;
    bytes[0x0021]=(uint8_t)currentPlanetIndex;
    bytes[0x0021+1]=(uint8_t)(currentPlanetIndex>>8);
    bytes[0x0023]=(uint8_t)currentPlanetLandscapeIndex;
    bytes[0x0023+1]=(uint8_t)(currentPlanetLandscapeIndex>>8);
    bytes[0x0025]=(uint8_t)currentPlanetHouseIndex;
    bytes[0x0025+1]=(uint8_t)(currentPlanetHouseIndex>>8);
    bytes[0x0027]=(uint8_t)currentPlanetMarketTier;
    bytes[0x0027+1]=(uint8_t)(currentPlanetMarketTier>>8);
    bytes[0x0029]=(uint8_t)currentPlanetFacilityDisplayFlag;
    bytes[0x0029+1]=(uint8_t)(currentPlanetFacilityDisplayFlag>>8);
    bytes[0x002B]=(uint8_t)currentPlanetMapPositionX;
    bytes[0x002B+1]=(uint8_t)(currentPlanetMapPositionX>>8);
    bytes[0x002D]=(uint8_t)currentPlanetMapPositionY;
    bytes[0x002D+1]=(uint8_t)(currentPlanetMapPositionY>>8);
    bytes[0x002F]=(uint8_t)planetMarketPremiumPercent;
    bytes[0x002F+1]=(uint8_t)(planetMarketPremiumPercent>>8);
    bytes[0x0031]=(uint8_t)campaignEconomy.calendarSubmonthUnits;
    bytes[0x0031+1]=(uint8_t)(campaignEconomy.calendarSubmonthUnits>>8);
    bytes[0x0033]=(uint8_t)calendarMonthIndex;
    bytes[0x0033+1]=(uint8_t)(calendarMonthIndex>>8);
    bytes[0x0035]=(uint8_t)calendarYear;
    bytes[0x0035+1]=(uint8_t)(calendarYear>>8);
    bytes[0x0037]=(uint8_t)campaignEconomy.marketRefreshUnits;
    bytes[0x0037+1]=(uint8_t)(campaignEconomy.marketRefreshUnits>>8);
    bytes[0x0039]=(uint8_t)campaignEconomy.elapsedBillingMonths;
    bytes[0x0039+1]=(uint8_t)(campaignEconomy.elapsedBillingMonths>>8);
    memcpy(bytes+0x003B,commanderNameBytes,sizeof(commanderNameBytes));
    memcpy(bytes+0x0045,unidentifiedField19,sizeof(unidentifiedField19));
    bytes[0x0047]=(uint8_t)commanderAgeYears;
    bytes[0x0047+1]=(uint8_t)(commanderAgeYears>>8);
    bytes[0x0049]=(uint8_t)campaignEconomy.campaignFundsLow;
    bytes[0x0049+1]=(uint8_t)(campaignEconomy.campaignFundsLow>>8);
    bytes[0x004B]=(uint8_t)campaignEconomy.campaignFundsHigh;
    bytes[0x004B+1]=(uint8_t)(campaignEconomy.campaignFundsHigh>>8);
    for (size_t i=0;i<sizeof(houseNetStandings)/sizeof(houseNetStandings[0]);++i)
    {
        bytes[0x004D+i*2]=(uint8_t)houseNetStandings[i];
        bytes[0x004D+i*2+1]=(uint8_t)(houseNetStandings[i]>>8);
    }
    for (size_t i=0;i<sizeof(campaignEconomy.houseFavorPoints)/sizeof(campaignEconomy.houseFavorPoints[0]);++i)
    {
        bytes[0x0057+i*2]=(uint8_t)campaignEconomy.houseFavorPoints[i];
        bytes[0x0057+i*2+1]=(uint8_t)(campaignEconomy.houseFavorPoints[i]>>8);
    }
    for (size_t i=0;i<sizeof(campaignEconomy.houseGrievancePoints)/sizeof(campaignEconomy.houseGrievancePoints[0]);++i)
    {
        bytes[0x0061+i*2]=(uint8_t)campaignEconomy.houseGrievancePoints[i];
        bytes[0x0061+i*2+1]=(uint8_t)(campaignEconomy.houseGrievancePoints[i]>>8);
    }
    bytes[0x006B]=(uint8_t)campaignEconomy.playerCombatCredit;
    bytes[0x006B+1]=(uint8_t)(campaignEconomy.playerCombatCredit>>8);
    bytes[0x006D]=(uint8_t)pilotRosterCount;
    bytes[0x006D+1]=(uint8_t)(pilotRosterCount>>8);
    for (size_t i=0;i<sizeof(rosterPilotIds)/sizeof(rosterPilotIds[0]);++i)
    {
        bytes[0x006F+i*2]=(uint8_t)rosterPilotIds[i];
        bytes[0x006F+i*2+1]=(uint8_t)(rosterPilotIds[i]>>8);
    }
    memcpy(bytes+0x0077,campaignMechanics.pilotIdentityStorage.roster.unavailablePilots1To42,sizeof(campaignMechanics.pilotIdentityStorage.roster.unavailablePilots1To42));
    bytes[0x00A1]=campaignEconomy.pilotDepartureFlagTail;
    for (size_t i=0;i<sizeof(campaignMechanics.rosterPayTiers)/sizeof(campaignMechanics.rosterPayTiers[0]);++i)
    {
        bytes[0x00A2+i*2]=(uint8_t)campaignMechanics.rosterPayTiers[i];
        bytes[0x00A2+i*2+1]=(uint8_t)(campaignMechanics.rosterPayTiers[i]>>8);
    }
    for (size_t i=0;i<sizeof(campaignMechanics.rosterGunneryTiers)/sizeof(campaignMechanics.rosterGunneryTiers[0]);++i)
    {
        bytes[0x00AA+i*2]=(uint8_t)campaignMechanics.rosterGunneryTiers[i];
        bytes[0x00AA+i*2+1]=(uint8_t)(campaignMechanics.rosterGunneryTiers[i]>>8);
    }
    for (size_t i=0;i<sizeof(campaignMechanics.rosterPilotingTiers)/sizeof(campaignMechanics.rosterPilotingTiers[0]);++i)
    {
        bytes[0x00B2+i*2]=(uint8_t)campaignMechanics.rosterPilotingTiers[i];
        bytes[0x00B2+i*2+1]=(uint8_t)(campaignMechanics.rosterPilotingTiers[i]>>8);
    }
    for (size_t i=0;i<sizeof(campaignMechanics.rosterUnknownWords)/sizeof(campaignMechanics.rosterUnknownWords[0]);++i)
    {
        bytes[0x00BA+i*2]=(uint8_t)campaignMechanics.rosterUnknownWords[i];
        bytes[0x00BA+i*2+1]=(uint8_t)(campaignMechanics.rosterUnknownWords[i]>>8);
    }
    for (size_t i=0;i<sizeof(campaignMechanics.rosterMechAssignments)/sizeof(campaignMechanics.rosterMechAssignments[0]);++i)
    {
        bytes[0x00C2+i*2]=(uint8_t)campaignMechanics.rosterMechAssignments[i];
        bytes[0x00C2+i*2+1]=(uint8_t)(campaignMechanics.rosterMechAssignments[i]>>8);
    }
    for (size_t i=0;i<sizeof(campaignMechanics.rosterPortraitIds)/sizeof(campaignMechanics.rosterPortraitIds[0]);++i)
    {
        bytes[0x00CA+i*2]=(uint8_t)campaignMechanics.rosterPortraitIds[i];
        bytes[0x00CA+i*2+1]=(uint8_t)(campaignMechanics.rosterPortraitIds[i]>>8);
    }
    bytes[0x00D2]=(uint8_t)campaignEconomy.dropShipMissedUpkeepPayments;
    bytes[0x00D2+1]=(uint8_t)(campaignEconomy.dropShipMissedUpkeepPayments>>8);
    bytes[0x00D4]=(uint8_t)campaignEconomy.unpaidPilotWageMarker;
    bytes[0x00D4+1]=(uint8_t)(campaignEconomy.unpaidPilotWageMarker>>8);
    bytes[0x00D6]=(uint8_t)campaignEconomy.availableRecruitCount;
    bytes[0x00D6+1]=(uint8_t)(campaignEconomy.availableRecruitCount>>8);
    for (size_t i=0;i<sizeof(campaignEconomy.availableRecruitIds)/sizeof(campaignEconomy.availableRecruitIds[0]);++i)
    {
        bytes[0x00D8+i*2]=(uint8_t)campaignEconomy.availableRecruitIds[i];
        bytes[0x00D8+i*2+1]=(uint8_t)(campaignEconomy.availableRecruitIds[i]>>8);
    }
    bytes[0x00E8]=(uint8_t)campaignEconomy.ownedUnitCount;
    bytes[0x00E8+1]=(uint8_t)(campaignEconomy.ownedUnitCount>>8);
    for (size_t i=0;i<sizeof(campaignEconomy.ownedUnitKinds)/sizeof(campaignEconomy.ownedUnitKinds[0]);++i)
    {
        bytes[0x00EA+i*2]=(uint8_t)campaignEconomy.ownedUnitKinds[i];
        bytes[0x00EA+i*2+1]=(uint8_t)(campaignEconomy.ownedUnitKinds[i]>>8);
    }
    memcpy(bytes+0x0102,campaignMechanics.ownedUnitDamage,sizeof(campaignMechanics.ownedUnitDamage));
    for (size_t i=0;i<sizeof(campaignEconomy.ownedAmmoQuantitiesByGroup)/sizeof(campaignEconomy.ownedAmmoQuantitiesByGroup[0]);++i)
    {
        bytes[0x025E +i*2]=(uint8_t)campaignEconomy.ownedAmmoQuantitiesByGroup[i];
        bytes[0x025E +i*2+1]=(uint8_t)(campaignEconomy.ownedAmmoQuantitiesByGroup[i]>>8);
    }
    for (size_t i=0;i<sizeof(campaignEconomy.marketAmmoQuantitiesByGroup)/sizeof(campaignEconomy.marketAmmoQuantitiesByGroup[0]);++i)
    {
        bytes[0x02A6+i*2]=(uint8_t)campaignEconomy.marketAmmoQuantitiesByGroup[i];
        bytes[0x02A6+i*2+1]=(uint8_t)(campaignEconomy.marketAmmoQuantitiesByGroup[i]>>8);
    }
    for (size_t i=0;i<sizeof(campaignMechanics.reserveAmmoQuantitiesByFamily)/sizeof(campaignMechanics.reserveAmmoQuantitiesByFamily[0]);++i)
    {
        bytes[0x02EE +i*2]=(uint8_t)campaignMechanics.reserveAmmoQuantitiesByFamily[i];
        bytes[0x02EE +i*2+1]=(uint8_t)(campaignMechanics.reserveAmmoQuantitiesByFamily[i]>>8);
    }
    for (size_t i=0;i<sizeof(campaignEconomy.ownedAmmoKindsByGroup)/sizeof(campaignEconomy.ownedAmmoKindsByGroup[0]);++i)
    {
        bytes[0x02FA+i*2]=(uint8_t)campaignEconomy.ownedAmmoKindsByGroup[i];
        bytes[0x02FA+i*2+1]=(uint8_t)(campaignEconomy.ownedAmmoKindsByGroup[i]>>8);
    }
    for (size_t i=0;i<sizeof(campaignEconomy.marketAmmoKindsByGroup)/sizeof(campaignEconomy.marketAmmoKindsByGroup[0]);++i)
    {
        bytes[0x0342+i*2]=(uint8_t)campaignEconomy.marketAmmoKindsByGroup[i];
        bytes[0x0342+i*2+1]=(uint8_t)(campaignEconomy.marketAmmoKindsByGroup[i]>>8);
    }
    bytes[0x038A]=(uint8_t)campaignEconomy.marketUnitCount;
    bytes[0x038A+1]=(uint8_t)(campaignEconomy.marketUnitCount>>8);
    for (size_t i=0;i<sizeof(campaignEconomy.marketUnitKinds)/sizeof(campaignEconomy.marketUnitKinds[0]);++i)
    {
        bytes[0x038C+i*2]=(uint8_t)campaignEconomy.marketUnitKinds[i];
        bytes[0x038C+i*2+1]=(uint8_t)(campaignEconomy.marketUnitKinds[i]>>8);
    }
    memcpy(bytes+0x03A4,campaignMechanics.marketUnitDamage,sizeof(campaignMechanics.marketUnitDamage));
    bytes[0x0500]=(uint8_t)campaignEconomy.missionOfferCount;
    bytes[0x0500+1]=(uint8_t)(campaignEconomy.missionOfferCount>>8);
    for (size_t i=0;i<sizeof(campaignEconomy.missionDescriptionOffsets)/sizeof(campaignEconomy.missionDescriptionOffsets[0]);++i)
    {
        bytes[0x0502+i*2]=(uint8_t)campaignEconomy.missionDescriptionOffsets[i];
        bytes[0x0502+i*2+1]=(uint8_t)(campaignEconomy.missionDescriptionOffsets[i]>>8);
    }
    for (size_t i=0;i<sizeof(campaignEconomy.missionPlanetRecordIndices)/sizeof(campaignEconomy.missionPlanetRecordIndices[0]);++i)
    {
        bytes[0x050E +i*2]=(uint8_t)campaignEconomy.missionPlanetRecordIndices[i];
        bytes[0x050E +i*2+1]=(uint8_t)(campaignEconomy.missionPlanetRecordIndices[i]>>8);
    }
    for (size_t i=0;i<sizeof(campaignEconomy.missionTargetHouseIndices)/sizeof(campaignEconomy.missionTargetHouseIndices[0]);++i)
    {
        bytes[0x051A+i*2]=(uint8_t)campaignEconomy.missionTargetHouseIndices[i];
        bytes[0x051A+i*2+1]=(uint8_t)(campaignEconomy.missionTargetHouseIndices[i]>>8);
    }
    for (size_t i=0;i<sizeof(campaignEconomy.missionTypeCodes)/sizeof(campaignEconomy.missionTypeCodes[0]);++i)
    {
        bytes[0x0526+i*2]=(uint8_t)campaignEconomy.missionTypeCodes[i];
        bytes[0x0526+i*2+1]=(uint8_t)(campaignEconomy.missionTypeCodes[i]>>8);
    }
    for (size_t i=0;i<sizeof(campaignEconomy.missionEnemyHeavyCounts)/sizeof(campaignEconomy.missionEnemyHeavyCounts[0]);++i)
    {
        bytes[0x0532+i*2]=(uint8_t)campaignEconomy.missionEnemyHeavyCounts[i];
        bytes[0x0532+i*2+1]=(uint8_t)(campaignEconomy.missionEnemyHeavyCounts[i]>>8);
    }
    for (size_t i=0;i<sizeof(campaignEconomy.missionEnemyMediumCounts)/sizeof(campaignEconomy.missionEnemyMediumCounts[0]);++i)
    {
        bytes[0x053E +i*2]=(uint8_t)campaignEconomy.missionEnemyMediumCounts[i];
        bytes[0x053E +i*2+1]=(uint8_t)(campaignEconomy.missionEnemyMediumCounts[i]>>8);
    }
    for (size_t i=0;i<sizeof(campaignEconomy.missionEnemyLightCounts)/sizeof(campaignEconomy.missionEnemyLightCounts[0]);++i)
    {
        bytes[0x054A+i*2]=(uint8_t)campaignEconomy.missionEnemyLightCounts[i];
        bytes[0x054A+i*2+1]=(uint8_t)(campaignEconomy.missionEnemyLightCounts[i]>>8);
    }
    for (size_t i=0;i<sizeof(campaignEconomy.missionPaymentThousands)/sizeof(campaignEconomy.missionPaymentThousands[0]);++i)
    {
        bytes[0x0556+i*2]=(uint8_t)campaignEconomy.missionPaymentThousands[i];
        bytes[0x0556+i*2+1]=(uint8_t)(campaignEconomy.missionPaymentThousands[i]>>8);
    }
    for (size_t i=0;i<sizeof(campaignEconomy.missionSalvagePercentages)/sizeof(campaignEconomy.missionSalvagePercentages[0]);++i)
    {
        bytes[0x0562+i*2]=(uint8_t)campaignEconomy.missionSalvagePercentages[i];
        bytes[0x0562+i*2+1]=(uint8_t)(campaignEconomy.missionSalvagePercentages[i]>>8);
    }
    for (size_t i=0;i<sizeof(campaignEconomy.missionAdvancePercentages)/sizeof(campaignEconomy.missionAdvancePercentages[0]);++i)
    {
        bytes[0x056E +i*2]=(uint8_t)campaignEconomy.missionAdvancePercentages[i];
        bytes[0x056E +i*2+1]=(uint8_t)(campaignEconomy.missionAdvancePercentages[i]>>8);
    }
    for (size_t i=0;i<sizeof(campaignEconomy.missionPaymentCeilingsThousands)/sizeof(campaignEconomy.missionPaymentCeilingsThousands[0]);++i)
    {
        bytes[0x057A+i*2]=(uint8_t)campaignEconomy.missionPaymentCeilingsThousands[i];
        bytes[0x057A+i*2+1]=(uint8_t)(campaignEconomy.missionPaymentCeilingsThousands[i]>>8);
    }
    for (size_t i=0;i<sizeof(campaignEconomy.missionSalvagePercentageCeilings)/sizeof(campaignEconomy.missionSalvagePercentageCeilings[0]);++i)
    {
        bytes[0x0586+i*2]=(uint8_t)campaignEconomy.missionSalvagePercentageCeilings[i];
        bytes[0x0586+i*2+1]=(uint8_t)(campaignEconomy.missionSalvagePercentageCeilings[i]>>8);
    }
    for (size_t i=0;i<sizeof(campaignEconomy.missionAdvancePercentageCeilings)/sizeof(campaignEconomy.missionAdvancePercentageCeilings[0]);++i)
    {
        bytes[0x0592+i*2]=(uint8_t)campaignEconomy.missionAdvancePercentageCeilings[i];
        bytes[0x0592+i*2+1]=(uint8_t)(campaignEconomy.missionAdvancePercentageCeilings[i]>>8);
    }
    bytes[0x059E]=(uint8_t)contractNegotiationAttemptLimit;
    bytes[0x059E +1]=(uint8_t)(contractNegotiationAttemptLimit>>8);
    bytes[0x05A0]=(uint8_t)requestedContractPaymentThousands;
    bytes[0x05A0+1]=(uint8_t)(requestedContractPaymentThousands>>8);
    bytes[0x05A2]=(uint8_t)requestedContractSalvagePercent;
    bytes[0x05A2+1]=(uint8_t)(requestedContractSalvagePercent>>8);
    bytes[0x05A4]=(uint8_t)requestedContractAdvancePercent;
    bytes[0x05A4+1]=(uint8_t)(requestedContractAdvancePercent>>8);
    bytes[0x05A6]=(uint8_t)requestContractPaymentEnabled;
    bytes[0x05A6+1]=(uint8_t)(requestContractPaymentEnabled>>8);
    bytes[0x05A8]=(uint8_t)requestContractSalvageEnabled;
    bytes[0x05A8+1]=(uint8_t)(requestContractSalvageEnabled>>8);
    bytes[0x05AA]=(uint8_t)requestContractAdvanceEnabled;
    bytes[0x05AA+1]=(uint8_t)(requestContractAdvanceEnabled>>8);
    memcpy(bytes+0x05AC,unidentifiedField20,sizeof(unidentifiedField20));
    bytes[0x05B2]=(uint8_t)acceptedContractPaymentThousands;
    bytes[0x05B2+1]=(uint8_t)(acceptedContractPaymentThousands>>8);
    bytes[0x05B4]=(uint8_t)acceptedContractSalvagePercent;
    bytes[0x05B4+1]=(uint8_t)(acceptedContractSalvagePercent>>8);
    bytes[0x05B6]=(uint8_t)acceptedContractAdvancePercent;
    bytes[0x05B6+1]=(uint8_t)(acceptedContractAdvancePercent>>8);
    bytes[0x05B8]=(uint8_t)remainingContractPaymentThousands;
    bytes[0x05B8+1]=(uint8_t)(remainingContractPaymentThousands>>8);
    memcpy(bytes+0x05BA,campaignEconomy.houseMissionTypeLimits,sizeof(campaignEconomy.houseMissionTypeLimits));
    bytes[0x05E2]=audioOptionLabelIndex;
    for (size_t i=0;i<sizeof(campaignEconomy.missionScenarioCodes)/sizeof(campaignEconomy.missionScenarioCodes[0]);++i)
    {
        bytes[0x05E3+i*2]=(uint8_t)campaignEconomy.missionScenarioCodes[i];
        bytes[0x05E3+i*2+1]=(uint8_t)(campaignEconomy.missionScenarioCodes[i]>>8);
    }
    memcpy(bytes+0x05EF,campaignMechanics.pilotGunneryTiers,sizeof(campaignMechanics.pilotGunneryTiers));
    memcpy(bytes+0x061A,campaignMechanics.pilotPilotingTiers,sizeof(campaignMechanics.pilotPilotingTiers));
    memcpy(bytes+0x0645,campaignMechanics.pilotExperienceCounts,sizeof(campaignMechanics.pilotExperienceCounts));
    bytes[0x0670]=(uint8_t)eventDirectoryIndex;
    bytes[0x0670+1]=(uint8_t)(eventDirectoryIndex>>8);
    memcpy(bytes+0x0672,eventSeenFlags,sizeof(eventSeenFlags));
    for (size_t i=0;i<sizeof(campaignEventPlanetSelectors)/sizeof(campaignEventPlanetSelectors[0]);++i)
    {
        bytes[0x06FE +i*2]=(uint8_t)campaignEventPlanetSelectors[i];
        bytes[0x06FE +i*2+1]=(uint8_t)(campaignEventPlanetSelectors[i]>>8);
    }
    bytes[0x070A]=campaignEconomy.majorCampaignMissionRollThreshold;
    bytes[0x070B]=(uint8_t)acceptedContractTargetHouseIndex;
    bytes[0x070B+1]=(uint8_t)(acceptedContractTargetHouseIndex>>8);
    bytes[0x070D]=campaignTerrainDetailLevel;
    bytes[0x070E]=(uint8_t)campaignEconomy.dealerUnitPurchaseLimit;
    bytes[0x070E +1]=(uint8_t)(campaignEconomy.dealerUnitPurchaseLimit>>8);
    bytes[0x0710]=(uint8_t)campaignEconomy.unitsSoldSinceMarketRefresh;
    bytes[0x0710+1]=(uint8_t)(campaignEconomy.unitsSoldSinceMarketRefresh>>8);
    bytes[0x0712]=(uint8_t)playerSkillProgress;
    bytes[0x0712+1]=(uint8_t)(playerSkillProgress>>8);
    bytes[0x0714]=darkWingBaseVictoryRecorded;
    bytes[0x0715]=(uint8_t)launcherMemoryTier;
    bytes[0x0715+1]=(uint8_t)(launcherMemoryTier>>8);
    bytes[0x0717]=savedVideoMode;
}

void MwDecodeCampaignExchange(const uint8_t bytes[CAMPAIGN_SAVE_BYTES])
{
    unidentifiedField18=bytes[0x0000];
    missionCategoryOrMemoryTier=bytes[0x0001];
    deploymentLandscapeIndex=bytes[0x0002];
    combatEnemyHeavyCount=bytes[0x0003];
    combatEnemyMediumCount=bytes[0x0004];
    combatEnemyLightCount=bytes[0x0005];
    combatPlanetSelector=bytes[0x0006];
    combatScenarioCode=bytes[0x0007];
    deploymentTerrainDetailLevel=bytes[0x0008];
    memcpy(campaignMechanics.rosterStatus,bytes+0x0009,sizeof(campaignMechanics.rosterStatus));
    memcpy(salvageEntries,bytes+0x000D,sizeof(salvageEntries));
    returnedDisabledEnemyCount=(uint16_t)(bytes[0x001D]|((uint16_t)bytes[0x001D+1]<<8));
    combatTransitionResult=bytes[0x001F];
    acceptedContractActive=bytes[0x0020];
    currentPlanetIndex=(uint16_t)(bytes[0x0021]|((uint16_t)bytes[0x0021+1]<<8));
    currentPlanetLandscapeIndex=(uint16_t)(bytes[0x0023]|((uint16_t)bytes[0x0023+1]<<8));
    currentPlanetHouseIndex=(uint16_t)(bytes[0x0025]|((uint16_t)bytes[0x0025+1]<<8));
    currentPlanetMarketTier=(uint16_t)(bytes[0x0027]|((uint16_t)bytes[0x0027+1]<<8));
    currentPlanetFacilityDisplayFlag=(uint16_t)(bytes[0x0029]|((uint16_t)bytes[0x0029+1]<<8));
    currentPlanetMapPositionX=(uint16_t)(bytes[0x002B]|((uint16_t)bytes[0x002B+1]<<8));
    currentPlanetMapPositionY=(uint16_t)(bytes[0x002D]|((uint16_t)bytes[0x002D+1]<<8));
    planetMarketPremiumPercent=(uint16_t)(bytes[0x002F]|((uint16_t)bytes[0x002F+1]<<8));
    campaignEconomy.calendarSubmonthUnits=(uint16_t)(bytes[0x0031]|((uint16_t)bytes[0x0031+1]<<8));
    calendarMonthIndex=(uint16_t)(bytes[0x0033]|((uint16_t)bytes[0x0033+1]<<8));
    calendarYear=(uint16_t)(bytes[0x0035]|((uint16_t)bytes[0x0035+1]<<8));
    campaignEconomy.marketRefreshUnits=(uint16_t)(bytes[0x0037]|((uint16_t)bytes[0x0037+1]<<8));
    campaignEconomy.elapsedBillingMonths=(uint16_t)(bytes[0x0039]|((uint16_t)bytes[0x0039+1]<<8));
    memcpy(commanderNameBytes,bytes+0x003B,sizeof(commanderNameBytes));
    memcpy(unidentifiedField19,bytes+0x0045,sizeof(unidentifiedField19));
    commanderAgeYears=(uint16_t)(bytes[0x0047]|((uint16_t)bytes[0x0047+1]<<8));
    campaignEconomy.campaignFundsLow=(uint16_t)(bytes[0x0049]|((uint16_t)bytes[0x0049+1]<<8));
    campaignEconomy.campaignFundsHigh=(uint16_t)(bytes[0x004B]|((uint16_t)bytes[0x004B+1]<<8));
    for (size_t i=0;i<sizeof(houseNetStandings)/sizeof(houseNetStandings[0]);++i)
    {
        houseNetStandings[i]=(uint16_t)(bytes[0x004D+i*2]|((uint16_t)bytes[0x004D+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(campaignEconomy.houseFavorPoints)/sizeof(campaignEconomy.houseFavorPoints[0]);++i)
    {
        campaignEconomy.houseFavorPoints[i]=(uint16_t)(bytes[0x0057+i*2]|((uint16_t)bytes[0x0057+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(campaignEconomy.houseGrievancePoints)/sizeof(campaignEconomy.houseGrievancePoints[0]);++i)
    {
        campaignEconomy.houseGrievancePoints[i]=(uint16_t)(bytes[0x0061+i*2]|((uint16_t)bytes[0x0061+i*2+1]<<8));
    }
    campaignEconomy.playerCombatCredit=(uint16_t)(bytes[0x006B]|((uint16_t)bytes[0x006B+1]<<8));
    pilotRosterCount=(uint16_t)(bytes[0x006D]|((uint16_t)bytes[0x006D+1]<<8));
    for (size_t i=0;i<sizeof(rosterPilotIds)/sizeof(rosterPilotIds[0]);++i)
    {
        rosterPilotIds[i]=(uint16_t)(bytes[0x006F+i*2]|((uint16_t)bytes[0x006F+i*2+1]<<8));
    }
    memcpy(campaignMechanics.pilotIdentityStorage.roster.unavailablePilots1To42,bytes+0x0077,sizeof(campaignMechanics.pilotIdentityStorage.roster.unavailablePilots1To42));
    campaignEconomy.pilotDepartureFlagTail=bytes[0x00A1];
    for (size_t i=0;i<sizeof(campaignMechanics.rosterPayTiers)/sizeof(campaignMechanics.rosterPayTiers[0]);++i)
    {
        campaignMechanics.rosterPayTiers[i]=(uint16_t)(bytes[0x00A2+i*2]|((uint16_t)bytes[0x00A2+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(campaignMechanics.rosterGunneryTiers)/sizeof(campaignMechanics.rosterGunneryTiers[0]);++i)
    {
        campaignMechanics.rosterGunneryTiers[i]=(uint16_t)(bytes[0x00AA+i*2]|((uint16_t)bytes[0x00AA+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(campaignMechanics.rosterPilotingTiers)/sizeof(campaignMechanics.rosterPilotingTiers[0]);++i)
    {
        campaignMechanics.rosterPilotingTiers[i]=(uint16_t)(bytes[0x00B2+i*2]|((uint16_t)bytes[0x00B2+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(campaignMechanics.rosterUnknownWords)/sizeof(campaignMechanics.rosterUnknownWords[0]);++i)
    {
        campaignMechanics.rosterUnknownWords[i]=(uint16_t)(bytes[0x00BA+i*2]|((uint16_t)bytes[0x00BA+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(campaignMechanics.rosterMechAssignments)/sizeof(campaignMechanics.rosterMechAssignments[0]);++i)
    {
        campaignMechanics.rosterMechAssignments[i]=(uint16_t)(bytes[0x00C2+i*2]|((uint16_t)bytes[0x00C2+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(campaignMechanics.rosterPortraitIds)/sizeof(campaignMechanics.rosterPortraitIds[0]);++i)
    {
        campaignMechanics.rosterPortraitIds[i]=(uint16_t)(bytes[0x00CA+i*2]|((uint16_t)bytes[0x00CA+i*2+1]<<8));
    }
    campaignEconomy.dropShipMissedUpkeepPayments=(uint16_t)(bytes[0x00D2]|((uint16_t)bytes[0x00D2+1]<<8));
    campaignEconomy.unpaidPilotWageMarker=(uint16_t)(bytes[0x00D4]|((uint16_t)bytes[0x00D4+1]<<8));
    campaignEconomy.availableRecruitCount=(uint16_t)(bytes[0x00D6]|((uint16_t)bytes[0x00D6+1]<<8));
    for (size_t i=0;i<sizeof(campaignEconomy.availableRecruitIds)/sizeof(campaignEconomy.availableRecruitIds[0]);++i)
    {
        campaignEconomy.availableRecruitIds[i]=(uint16_t)(bytes[0x00D8+i*2]|((uint16_t)bytes[0x00D8+i*2+1]<<8));
    }
    campaignEconomy.ownedUnitCount=(uint16_t)(bytes[0x00E8]|((uint16_t)bytes[0x00E8+1]<<8));
    for (size_t i=0;i<sizeof(campaignEconomy.ownedUnitKinds)/sizeof(campaignEconomy.ownedUnitKinds[0]);++i)
    {
        campaignEconomy.ownedUnitKinds[i]=(uint16_t)(bytes[0x00EA+i*2]|((uint16_t)bytes[0x00EA+i*2+1]<<8));
    }
    memcpy(campaignMechanics.ownedUnitDamage,bytes+0x0102,sizeof(campaignMechanics.ownedUnitDamage));
    for (size_t i=0;i<sizeof(campaignEconomy.ownedAmmoQuantitiesByGroup)/sizeof(campaignEconomy.ownedAmmoQuantitiesByGroup[0]);++i)
    {
        campaignEconomy.ownedAmmoQuantitiesByGroup[i]=(uint16_t)(bytes[0x025E +i*2]|((uint16_t)bytes[0x025E +i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(campaignEconomy.marketAmmoQuantitiesByGroup)/sizeof(campaignEconomy.marketAmmoQuantitiesByGroup[0]);++i)
    {
        campaignEconomy.marketAmmoQuantitiesByGroup[i]=(uint16_t)(bytes[0x02A6+i*2]|((uint16_t)bytes[0x02A6+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(campaignMechanics.reserveAmmoQuantitiesByFamily)/sizeof(campaignMechanics.reserveAmmoQuantitiesByFamily[0]);++i)
    {
        campaignMechanics.reserveAmmoQuantitiesByFamily[i]=(uint16_t)(bytes[0x02EE +i*2]|((uint16_t)bytes[0x02EE +i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(campaignEconomy.ownedAmmoKindsByGroup)/sizeof(campaignEconomy.ownedAmmoKindsByGroup[0]);++i)
    {
        campaignEconomy.ownedAmmoKindsByGroup[i]=(uint16_t)(bytes[0x02FA+i*2]|((uint16_t)bytes[0x02FA+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(campaignEconomy.marketAmmoKindsByGroup)/sizeof(campaignEconomy.marketAmmoKindsByGroup[0]);++i)
    {
        campaignEconomy.marketAmmoKindsByGroup[i]=(uint16_t)(bytes[0x0342+i*2]|((uint16_t)bytes[0x0342+i*2+1]<<8));
    }
    campaignEconomy.marketUnitCount=(uint16_t)(bytes[0x038A]|((uint16_t)bytes[0x038A+1]<<8));
    for (size_t i=0;i<sizeof(campaignEconomy.marketUnitKinds)/sizeof(campaignEconomy.marketUnitKinds[0]);++i)
    {
        campaignEconomy.marketUnitKinds[i]=(uint16_t)(bytes[0x038C+i*2]|((uint16_t)bytes[0x038C+i*2+1]<<8));
    }
    memcpy(campaignMechanics.marketUnitDamage,bytes+0x03A4,sizeof(campaignMechanics.marketUnitDamage));
    campaignEconomy.missionOfferCount=(uint16_t)(bytes[0x0500]|((uint16_t)bytes[0x0500+1]<<8));
    for (size_t i=0;i<sizeof(campaignEconomy.missionDescriptionOffsets)/sizeof(campaignEconomy.missionDescriptionOffsets[0]);++i)
    {
        campaignEconomy.missionDescriptionOffsets[i]=(uint16_t)(bytes[0x0502+i*2]|((uint16_t)bytes[0x0502+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(campaignEconomy.missionPlanetRecordIndices)/sizeof(campaignEconomy.missionPlanetRecordIndices[0]);++i)
    {
        campaignEconomy.missionPlanetRecordIndices[i]=(uint16_t)(bytes[0x050E +i*2]|((uint16_t)bytes[0x050E +i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(campaignEconomy.missionTargetHouseIndices)/sizeof(campaignEconomy.missionTargetHouseIndices[0]);++i)
    {
        campaignEconomy.missionTargetHouseIndices[i]=(uint16_t)(bytes[0x051A+i*2]|((uint16_t)bytes[0x051A+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(campaignEconomy.missionTypeCodes)/sizeof(campaignEconomy.missionTypeCodes[0]);++i)
    {
        campaignEconomy.missionTypeCodes[i]=(uint16_t)(bytes[0x0526+i*2]|((uint16_t)bytes[0x0526+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(campaignEconomy.missionEnemyHeavyCounts)/sizeof(campaignEconomy.missionEnemyHeavyCounts[0]);++i)
    {
        campaignEconomy.missionEnemyHeavyCounts[i]=(uint16_t)(bytes[0x0532+i*2]|((uint16_t)bytes[0x0532+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(campaignEconomy.missionEnemyMediumCounts)/sizeof(campaignEconomy.missionEnemyMediumCounts[0]);++i)
    {
        campaignEconomy.missionEnemyMediumCounts[i]=(uint16_t)(bytes[0x053E +i*2]|((uint16_t)bytes[0x053E +i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(campaignEconomy.missionEnemyLightCounts)/sizeof(campaignEconomy.missionEnemyLightCounts[0]);++i)
    {
        campaignEconomy.missionEnemyLightCounts[i]=(uint16_t)(bytes[0x054A+i*2]|((uint16_t)bytes[0x054A+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(campaignEconomy.missionPaymentThousands)/sizeof(campaignEconomy.missionPaymentThousands[0]);++i)
    {
        campaignEconomy.missionPaymentThousands[i]=(uint16_t)(bytes[0x0556+i*2]|((uint16_t)bytes[0x0556+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(campaignEconomy.missionSalvagePercentages)/sizeof(campaignEconomy.missionSalvagePercentages[0]);++i)
    {
        campaignEconomy.missionSalvagePercentages[i]=(uint16_t)(bytes[0x0562+i*2]|((uint16_t)bytes[0x0562+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(campaignEconomy.missionAdvancePercentages)/sizeof(campaignEconomy.missionAdvancePercentages[0]);++i)
    {
        campaignEconomy.missionAdvancePercentages[i]=(uint16_t)(bytes[0x056E +i*2]|((uint16_t)bytes[0x056E +i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(campaignEconomy.missionPaymentCeilingsThousands)/sizeof(campaignEconomy.missionPaymentCeilingsThousands[0]);++i)
    {
        campaignEconomy.missionPaymentCeilingsThousands[i]=(uint16_t)(bytes[0x057A+i*2]|((uint16_t)bytes[0x057A+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(campaignEconomy.missionSalvagePercentageCeilings)/sizeof(campaignEconomy.missionSalvagePercentageCeilings[0]);++i)
    {
        campaignEconomy.missionSalvagePercentageCeilings[i]=(uint16_t)(bytes[0x0586+i*2]|((uint16_t)bytes[0x0586+i*2+1]<<8));
    }
    for (size_t i=0;i<sizeof(campaignEconomy.missionAdvancePercentageCeilings)/sizeof(campaignEconomy.missionAdvancePercentageCeilings[0]);++i)
    {
        campaignEconomy.missionAdvancePercentageCeilings[i]=(uint16_t)(bytes[0x0592+i*2]|((uint16_t)bytes[0x0592+i*2+1]<<8));
    }
    contractNegotiationAttemptLimit=(uint16_t)(bytes[0x059E]|((uint16_t)bytes[0x059E +1]<<8));
    requestedContractPaymentThousands=(uint16_t)(bytes[0x05A0]|((uint16_t)bytes[0x05A0+1]<<8));
    requestedContractSalvagePercent=(uint16_t)(bytes[0x05A2]|((uint16_t)bytes[0x05A2+1]<<8));
    requestedContractAdvancePercent=(uint16_t)(bytes[0x05A4]|((uint16_t)bytes[0x05A4+1]<<8));
    requestContractPaymentEnabled=(uint16_t)(bytes[0x05A6]|((uint16_t)bytes[0x05A6+1]<<8));
    requestContractSalvageEnabled=(uint16_t)(bytes[0x05A8]|((uint16_t)bytes[0x05A8+1]<<8));
    requestContractAdvanceEnabled=(uint16_t)(bytes[0x05AA]|((uint16_t)bytes[0x05AA+1]<<8));
    memcpy(unidentifiedField20,bytes+0x05AC,sizeof(unidentifiedField20));
    acceptedContractPaymentThousands=(uint16_t)(bytes[0x05B2]|((uint16_t)bytes[0x05B2+1]<<8));
    acceptedContractSalvagePercent=(uint16_t)(bytes[0x05B4]|((uint16_t)bytes[0x05B4+1]<<8));
    acceptedContractAdvancePercent=(uint16_t)(bytes[0x05B6]|((uint16_t)bytes[0x05B6+1]<<8));
    remainingContractPaymentThousands=(uint16_t)(bytes[0x05B8]|((uint16_t)bytes[0x05B8+1]<<8));
    memcpy(campaignEconomy.houseMissionTypeLimits,bytes+0x05BA,sizeof(campaignEconomy.houseMissionTypeLimits));
    audioOptionLabelIndex=bytes[0x05E2];
    for (size_t i=0;i<sizeof(campaignEconomy.missionScenarioCodes)/sizeof(campaignEconomy.missionScenarioCodes[0]);++i)
    {
        campaignEconomy.missionScenarioCodes[i]=(uint16_t)(bytes[0x05E3+i*2]|((uint16_t)bytes[0x05E3+i*2+1]<<8));
    }
    memcpy(campaignMechanics.pilotGunneryTiers,bytes+0x05EF,sizeof(campaignMechanics.pilotGunneryTiers));
    memcpy(campaignMechanics.pilotPilotingTiers,bytes+0x061A,sizeof(campaignMechanics.pilotPilotingTiers));
    memcpy(campaignMechanics.pilotExperienceCounts,bytes+0x0645,sizeof(campaignMechanics.pilotExperienceCounts));
    eventDirectoryIndex=(uint16_t)(bytes[0x0670]|((uint16_t)bytes[0x0670+1]<<8));
    memcpy(eventSeenFlags,bytes+0x0672,sizeof(eventSeenFlags));
    for (size_t i=0;i<sizeof(campaignEventPlanetSelectors)/sizeof(campaignEventPlanetSelectors[0]);++i)
    {
        campaignEventPlanetSelectors[i]=(uint16_t)(bytes[0x06FE +i*2]|((uint16_t)bytes[0x06FE +i*2+1]<<8));
    }
    campaignEconomy.majorCampaignMissionRollThreshold=bytes[0x070A];
    acceptedContractTargetHouseIndex=(uint16_t)(bytes[0x070B]|((uint16_t)bytes[0x070B+1]<<8));
    campaignTerrainDetailLevel=bytes[0x070D];
    campaignEconomy.dealerUnitPurchaseLimit=(uint16_t)(bytes[0x070E]|((uint16_t)bytes[0x070E +1]<<8));
    campaignEconomy.unitsSoldSinceMarketRefresh=(uint16_t)(bytes[0x0710]|((uint16_t)bytes[0x0710+1]<<8));
    playerSkillProgress=(uint16_t)(bytes[0x0712]|((uint16_t)bytes[0x0712+1]<<8));
    darkWingBaseVictoryRecorded=bytes[0x0714];
    launcherMemoryTier=(uint16_t)(bytes[0x0715]|((uint16_t)bytes[0x0715+1]<<8));
    savedVideoMode=bytes[0x0717];
}
