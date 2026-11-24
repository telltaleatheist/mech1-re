#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/campaign_continuations.h"
#include "mechwarrior/campaign_events.h"
#include "mechwarrior/campaign_mechanics.h"
#include "mechwarrior/campaign_state.h"
#include "mechwarrior/campaign_economy.h"
#include "mechwarrior/ega_campaign.h"
#include <stdlib.h>

enum { EVENT_PAGE_PANEL_X=0x001D, EVENT_PAGE_PANEL_Y=0x0017,
    EVENT_PAGE_PANEL_WIDTH=0x0106, EVENT_PAGE_PANEL_HEIGHT=0x009A,
    EVENT_PAGE_TEXT_X=0x001F, EVENT_PAGE_TEXT_Y=0x0019 };

/* MW_MAIN081B:61CC..621E. FF asks0EF1 to skip THIS caller's return word:
 * the page remains visible and the actual text origin reaches the choice code.
 * Without FF,6210 overwrites AX with1AA5 before the next-page test. */
MwTextPanelReturn MwShowCampaignEventText(uint16_t textId)
{
    if (textId==8) campaignUi.textPanelYOverride=0x0082;
    if (textId>=EVENT_TEXT_DIRECTORY_COUNT || !campaignEventText.eventTextRecords[textId]) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: textId>=EVENT_TEXT_DIRECTORY_COUNT || !eventTextRecords[textId]"); /* Unrepresented zero-directory alias, not a native empty-text test. */
    const uint8_t *page=campaignEventText.eventTextRecords[textId];
    MwText text=MwArrayText(campaignEventText.eventTextStorage);
    for (;;)
    {
        if (*page==EVENT_TEXT_PLACEHOLDER)
        {
            ++page;
            campaignUi.textPanelContinuationMode=TEXT_PANEL_MODE_DISCARD_RETURN;
        }
        const MwEventTextPageHeader *header=(const MwEventTextPageHeader *)page;
        campaignEvents.eventTextRowCount=header->rowCount;
        page+=sizeof(*header);
        text.cursor=page;
        MwTextPanelReturn result=MwShowCompactTextPanel(0x000C,0x000A,0x002F,
            campaignEvents.eventTextRowCount+3,text);
        if (result.discardReturnWord) return result; /*100C RET bypasses6210..621E. */
        page=result.textEnd+1;
        if (page>=text.end) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: page>=eventTextStorage+sizeof(eventTextStorage)");
        if (!*page)
        {
            result.textEnd=page;
            return result;
        }
    }
}

/* MW_MAIN081B:6855..6912. Native byte clear [0448,0B5D) expressed as
 * assignments to its nonoverlapping typed owners. IRQ/timer state is outside
 * this range and there are no intervening calls. Memory tier0B5D is retained. */
uint16_t MwRestartCampaignContinuation(void)
{
    if (!campaignStopMusic) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !campaignStopMusic");
    campaignStopMusic();
    campaignEventReturnContinuation=campaignEventDispatchReturnContinuation=0; /*6865/66 discard suspended native frames. */
    campaignEvents.campaignRestartPending=0;
    for (uint16_t i=0;i<CAMPAIGN_NOTIFICATION_COUNT;++i) campaignEconomy.notificationPending[i]=0;
    campaignMission.unidentifiedField18=0;
    campaignMission.missionCategoryOrMemoryTier=0;
    campaignMission.deploymentLandscapeIndex=0;
    campaignMission.combatEnemyHeavyCount=0;
    campaignMission.combatEnemyMediumCount=0;
    campaignMission.combatEnemyLightCount=0;
    campaignMission.combatPlanetSelector=0;
    campaignMission.combatScenarioCode=0;
    campaignMission.deploymentTerrainDetailLevel=0;
    for (size_t i=0;i<sizeof(campaignMechanics.rosterStatus)/sizeof(campaignMechanics.rosterStatus[0]);++i) campaignMechanics.rosterStatus[i]=0;
    for (size_t i=0;i<sizeof(campaignMission.salvageEntries)/sizeof(campaignMission.salvageEntries[0]);++i) campaignMission.salvageEntries[i]=CampaignSalvageEntry{};
    campaignMission.returnedDisabledEnemyCount=0;
    campaignMission.combatTransitionResult=0;
    campaignMission.acceptedContractActive=0;
    campaignCore.currentPlanetIndex=0;
    campaignCore.currentPlanetLandscapeIndex=0;
    campaignCore.currentPlanetHouseIndex=0;
    campaignCore.currentPlanetMarketTier=0;
    campaignCore.currentPlanetFacilityDisplayFlag=0;
    campaignCore.currentPlanetMapPositionX=0;
    campaignCore.currentPlanetMapPositionY=0;
    campaignCore.planetMarketPremiumPercent=0;
    campaignEconomy.calendarSubmonthUnits=0;
    campaignCore.calendarMonthIndex=0;
    campaignCore.calendarYear=0;
    campaignEconomy.marketRefreshUnits=0;
    campaignEconomy.elapsedBillingMonths=0;
    for (size_t i=0;i<sizeof(campaignMission.commanderNameBytes)/sizeof(campaignMission.commanderNameBytes[0]);++i) campaignMission.commanderNameBytes[i]=0;
    for (size_t i=0;i<sizeof(campaignMission.unidentifiedField19)/sizeof(campaignMission.unidentifiedField19[0]);++i) campaignMission.unidentifiedField19[i]=0;
    campaignMission.commanderAgeYears=0;
    campaignEconomy.campaignFundsLow=0;
    campaignEconomy.campaignFundsHigh=0;
    for (size_t i=0;i<sizeof(campaignMission.houseNetStandings)/sizeof(campaignMission.houseNetStandings[0]);++i) campaignMission.houseNetStandings[i]=0;
    for (size_t i=0;i<sizeof(campaignEconomy.houseFavorPoints)/sizeof(campaignEconomy.houseFavorPoints[0]);++i) campaignEconomy.houseFavorPoints[i]=0;
    for (size_t i=0;i<sizeof(campaignEconomy.houseGrievancePoints)/sizeof(campaignEconomy.houseGrievancePoints[0]);++i) campaignEconomy.houseGrievancePoints[i]=0;
    campaignEconomy.playerCombatCredit=0;
    campaignMechanics.pilotIdentityStorage=CampaignPilotIdentityStorage{};
    campaignEconomy.pilotDepartureFlagTail=0;
    for (size_t i=0;i<sizeof(campaignMechanics.rosterPayTiers)/sizeof(campaignMechanics.rosterPayTiers[0]);++i) campaignMechanics.rosterPayTiers[i]=0;
    for (size_t i=0;i<sizeof(campaignMechanics.rosterGunneryTiers)/sizeof(campaignMechanics.rosterGunneryTiers[0]);++i) campaignMechanics.rosterGunneryTiers[i]=0;
    for (size_t i=0;i<sizeof(campaignMechanics.rosterPilotingTiers)/sizeof(campaignMechanics.rosterPilotingTiers[0]);++i) campaignMechanics.rosterPilotingTiers[i]=0;
    for (size_t i=0;i<sizeof(campaignMechanics.rosterUnknownWords)/sizeof(campaignMechanics.rosterUnknownWords[0]);++i) campaignMechanics.rosterUnknownWords[i]=0;
    for (size_t i=0;i<sizeof(campaignMechanics.rosterMechAssignments)/sizeof(campaignMechanics.rosterMechAssignments[0]);++i) campaignMechanics.rosterMechAssignments[i]=0;
    for (size_t i=0;i<sizeof(campaignMechanics.rosterPortraitIds)/sizeof(campaignMechanics.rosterPortraitIds[0]);++i) campaignMechanics.rosterPortraitIds[i]=0;
    campaignEconomy.dropShipMissedUpkeepPayments=0;
    campaignEconomy.unpaidPilotWageMarker=0;
    campaignEconomy.availableRecruitCount=0;
    for (size_t i=0;i<sizeof(campaignEconomy.availableRecruitIds)/sizeof(campaignEconomy.availableRecruitIds[0]);++i) campaignEconomy.availableRecruitIds[i]=0;
    campaignEconomy.ownedUnitCount=0;
    for (size_t i=0;i<sizeof(campaignEconomy.ownedUnitKinds)/sizeof(campaignEconomy.ownedUnitKinds[0]);++i) campaignEconomy.ownedUnitKinds[i]=0;
    for (size_t i=0;i<sizeof(campaignMechanics.ownedUnitDamage)/sizeof(campaignMechanics.ownedUnitDamage[0]);++i) campaignMechanics.ownedUnitDamage[i]=CampaignUnitDamage{};
    for (size_t i=0;i<sizeof(campaignEconomy.ownedAmmoQuantitiesByGroup)/sizeof(campaignEconomy.ownedAmmoQuantitiesByGroup[0]);++i) campaignEconomy.ownedAmmoQuantitiesByGroup[i]=0;
    for (size_t i=0;i<sizeof(campaignEconomy.marketAmmoQuantitiesByGroup)/sizeof(campaignEconomy.marketAmmoQuantitiesByGroup[0]);++i) campaignEconomy.marketAmmoQuantitiesByGroup[i]=0;
    for (size_t i=0;i<sizeof(campaignMechanics.reserveAmmoQuantitiesByFamily)/sizeof(campaignMechanics.reserveAmmoQuantitiesByFamily[0]);++i) campaignMechanics.reserveAmmoQuantitiesByFamily[i]=0;
    for (size_t i=0;i<sizeof(campaignEconomy.ownedAmmoKindsByGroup)/sizeof(campaignEconomy.ownedAmmoKindsByGroup[0]);++i) campaignEconomy.ownedAmmoKindsByGroup[i]=0;
    for (size_t i=0;i<sizeof(campaignEconomy.marketAmmoKindsByGroup)/sizeof(campaignEconomy.marketAmmoKindsByGroup[0]);++i) campaignEconomy.marketAmmoKindsByGroup[i]=0;
    campaignEconomy.marketUnitCount=0;
    for (size_t i=0;i<sizeof(campaignEconomy.marketUnitKinds)/sizeof(campaignEconomy.marketUnitKinds[0]);++i) campaignEconomy.marketUnitKinds[i]=0;
    for (size_t i=0;i<sizeof(campaignMechanics.marketUnitDamage)/sizeof(campaignMechanics.marketUnitDamage[0]);++i) campaignMechanics.marketUnitDamage[i]=CampaignUnitDamage{};
    campaignEconomy.missionOfferCount=0;
    for (size_t i=0;i<sizeof(campaignEconomy.missionDescriptionOffsets)/sizeof(campaignEconomy.missionDescriptionOffsets[0]);++i) campaignEconomy.missionDescriptionOffsets[i]=0;
    for (size_t i=0;i<sizeof(campaignEconomy.missionPlanetRecordIndices)/sizeof(campaignEconomy.missionPlanetRecordIndices[0]);++i) campaignEconomy.missionPlanetRecordIndices[i]=0;
    for (size_t i=0;i<sizeof(campaignEconomy.missionTargetHouseIndices)/sizeof(campaignEconomy.missionTargetHouseIndices[0]);++i) campaignEconomy.missionTargetHouseIndices[i]=0;
    for (size_t i=0;i<sizeof(campaignEconomy.missionTypeCodes)/sizeof(campaignEconomy.missionTypeCodes[0]);++i) campaignEconomy.missionTypeCodes[i]=0;
    for (size_t i=0;i<sizeof(campaignEconomy.missionEnemyHeavyCounts)/sizeof(campaignEconomy.missionEnemyHeavyCounts[0]);++i) campaignEconomy.missionEnemyHeavyCounts[i]=0;
    for (size_t i=0;i<sizeof(campaignEconomy.missionEnemyMediumCounts)/sizeof(campaignEconomy.missionEnemyMediumCounts[0]);++i) campaignEconomy.missionEnemyMediumCounts[i]=0;
    for (size_t i=0;i<sizeof(campaignEconomy.missionEnemyLightCounts)/sizeof(campaignEconomy.missionEnemyLightCounts[0]);++i) campaignEconomy.missionEnemyLightCounts[i]=0;
    for (size_t i=0;i<sizeof(campaignEconomy.missionPaymentThousands)/sizeof(campaignEconomy.missionPaymentThousands[0]);++i) campaignEconomy.missionPaymentThousands[i]=0;
    for (size_t i=0;i<sizeof(campaignEconomy.missionSalvagePercentages)/sizeof(campaignEconomy.missionSalvagePercentages[0]);++i) campaignEconomy.missionSalvagePercentages[i]=0;
    for (size_t i=0;i<sizeof(campaignEconomy.missionAdvancePercentages)/sizeof(campaignEconomy.missionAdvancePercentages[0]);++i) campaignEconomy.missionAdvancePercentages[i]=0;
    for (size_t i=0;i<sizeof(campaignEconomy.missionPaymentCeilingsThousands)/sizeof(campaignEconomy.missionPaymentCeilingsThousands[0]);++i) campaignEconomy.missionPaymentCeilingsThousands[i]=0;
    for (size_t i=0;i<sizeof(campaignEconomy.missionSalvagePercentageCeilings)/sizeof(campaignEconomy.missionSalvagePercentageCeilings[0]);++i) campaignEconomy.missionSalvagePercentageCeilings[i]=0;
    for (size_t i=0;i<sizeof(campaignEconomy.missionAdvancePercentageCeilings)/sizeof(campaignEconomy.missionAdvancePercentageCeilings[0]);++i) campaignEconomy.missionAdvancePercentageCeilings[i]=0;
    campaignCore.contractNegotiationAttemptLimit=0;
    campaignMission.requestedContractPaymentThousands=0;
    campaignMission.requestedContractSalvagePercent=0;
    campaignMission.requestedContractAdvancePercent=0;
    campaignMission.requestContractPaymentEnabled=0;
    campaignMission.requestContractSalvageEnabled=0;
    campaignMission.requestContractAdvanceEnabled=0;
    for (size_t i=0;i<sizeof(campaignMission.unidentifiedField20)/sizeof(campaignMission.unidentifiedField20[0]);++i) campaignMission.unidentifiedField20[i]=0;
    campaignMission.acceptedContractPaymentThousands=0;
    campaignMission.acceptedContractSalvagePercent=0;
    campaignMission.acceptedContractAdvancePercent=0;
    campaignMission.remainingContractPaymentThousands=0;
    for (size_t i=0;i<sizeof(campaignEconomy.houseMissionTypeLimits)/sizeof(campaignEconomy.houseMissionTypeLimits[0]);++i) campaignEconomy.houseMissionTypeLimits[i]=0;
    campaignMission.audioOptionLabelIndex=0;
    for (size_t i=0;i<sizeof(campaignEconomy.missionScenarioCodes)/sizeof(campaignEconomy.missionScenarioCodes[0]);++i) campaignEconomy.missionScenarioCodes[i]=0;
    for (size_t i=0;i<sizeof(campaignMechanics.pilotGunneryTiers)/sizeof(campaignMechanics.pilotGunneryTiers[0]);++i) campaignMechanics.pilotGunneryTiers[i]=0;
    for (size_t i=0;i<sizeof(campaignMechanics.pilotPilotingTiers)/sizeof(campaignMechanics.pilotPilotingTiers[0]);++i) campaignMechanics.pilotPilotingTiers[i]=0;
    for (size_t i=0;i<sizeof(campaignMechanics.pilotExperienceCounts)/sizeof(campaignMechanics.pilotExperienceCounts[0]);++i) campaignMechanics.pilotExperienceCounts[i]=0;
    campaignEvents.eventDirectoryIndex=0;
    for (size_t i=0;i<sizeof(campaignEvents.eventSeenFlags)/sizeof(campaignEvents.eventSeenFlags[0]);++i) campaignEvents.eventSeenFlags[i]=0;
    for (size_t i=0;i<sizeof(campaignStartup.campaignEventPlanetSelectors)/sizeof(campaignStartup.campaignEventPlanetSelectors[0]);++i) campaignStartup.campaignEventPlanetSelectors[i]=0;
    campaignEconomy.majorCampaignMissionRollThreshold=0;
    campaignMission.acceptedContractTargetHouseIndex=0;
    campaignMission.campaignTerrainDetailLevel=0;
    campaignEconomy.dealerUnitPurchaseLimit=0;
    campaignEconomy.unitsSoldSinceMarketRefresh=0;
    campaignMission.playerSkillProgress=0;
    campaignEvents.darkWingBaseVictoryRecorded=0;
    campaignEconomy.calendarSubmonthUnits=1;campaignCore.calendarMonthIndex=3;campaignCore.calendarYear=0x0BD0;
    campaignMission.commanderAgeYears=COMMANDER_INITIAL_AGE_YEARS;
    campaignEconomy.campaignFundsLow=(uint16_t)DECIMAL_MILLION;campaignEconomy.campaignFundsHigh=DECIMAL_MILLION>>NATIVE_WORD_BITS;
    pilotRosterCount=1;
    for (uint16_t i=0;i<CAMPAIGN_ROSTER_SLOTS;++i) campaignMechanics.rosterMechAssignments[i]=UINT16_MAX;
    for (uint16_t i=0;i<CAMPAIGN_OWNED_UNIT_SLOTS;++i)
    {campaignEconomy.ownedUnitKinds[i]=UINT16_MAX;campaignEconomy.marketUnitKinds[i]=UINT16_MAX;}
    for (size_t i=0;i<sizeof(campaignEconomy.houseMissionTypeLimits);++i)
        campaignEconomy.houseMissionTypeLimits[i]=campaignMission.defaultHouseMissionTypeLimits[i];
    for (size_t i=0;i<sizeof(campaignMission.commanderNameBytes);++i)
        campaignMission.commanderNameBytes[i]=campaignMission.campaignDefaultCommanderNameBytes[i];
    campaignEvents.eventDirectoryIndex=1;campaignMission.campaignTerrainDetailLevel=2;
    return CAMPAIGN_CONTINUE_INITIALIZE_COMPANY; /* Native JMP into the same166D body, not a new procedure. */
}

/* MW_MAIN081B:6153..61BE. This style deliberately does not parse FF. Only
 * intermediate pages acknowledge input; the final page remains displayed. */
MwTextPanelReturn MwShowClassThreeEventPages(uint16_t textId)
{
    EgaSelectFont(0);
    EgaFillDisplayPageRectangle(EVENT_PAGE_PANEL_X,EVENT_PAGE_PANEL_Y,EVENT_PAGE_PANEL_WIDTH,EVENT_PAGE_PANEL_HEIGHT,Campaign_Pal_EGA_Black);
    if (textId>=EVENT_TEXT_DIRECTORY_COUNT || !campaignEventText.eventTextRecords[textId]) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: textId>=EVENT_TEXT_DIRECTORY_COUNT || !eventTextRecords[textId]");
    const uint8_t *page=campaignEventText.eventTextRecords[textId];
    MwText text=MwArrayText(campaignEventText.eventTextStorage);
    for (;;)
    {
        page+=sizeof(MwEventTextPageHeader);
        text.cursor=page;
        page=EgaDrawTextWithLineSpacing(EVENT_PAGE_TEXT_X,EVENT_PAGE_TEXT_Y,Campaign_Pal_EGA_Green,text)+1;
        if (page>=text.end) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: page>=eventTextStorage+sizeof(eventTextStorage)");
        if (!*page)
        {
            MwTextPanelReturn result={page,0x1AA5,0,false};
            return result; /* Register projection; no class03 choice consumes BX. */
        }
        MwWaitForInputAcknowledgment();
        EgaFillDisplayPageRectangle(EVENT_PAGE_PANEL_X,EVENT_PAGE_PANEL_Y,EVENT_PAGE_PANEL_WIDTH,EVENT_PAGE_PANEL_HEIGHT,Campaign_Pal_EGA_Black);
    }
}

/* MW_MAIN081B:6668..66CB. No positive-selector alternative; the script BYTE
 * is complemented unconditionally. Text writes persist in the resource owner. */
void MwEventSubstitutePlanetName(const MwEventCommand *command)
{
    if (campaignEvents.eventConditionResult==1)
    {
        uint16_t reference=(uint16_t)command->operand.planetSelector^UINT16_MAX;
        if (reference>=6 || campaignEvents.currentEventId>=EVENT_TEXT_DIRECTORY_COUNT
            || !campaignEventText.eventTextRecords[campaignEvents.currentEventId]) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: reference>=6 || currentEventId>=EVENT_TEXT_DIRECTORY_COUNT || !eventTextRecords[currentEventId]");
        uint16_t planetIndex=campaignStartup.campaignEventPlanetSelectors[reference]-1;
        if (planetIndex>=CAMPAIGN_PLANET_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: planetIndex>=CAMPAIGN_PLANET_COUNT");
        uint16_t planetOffset=campaignCore.planetRecordOffsets[planetIndex];
        if (planetOffset<PLANET_RECORDS_START || planetOffset+PLANET_NAME_HEADER_BYTES>=PLANET_RECORDS_END) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: planetOffset<PLANET_RECORDS_START || planetOffset+PLANET_NAME_HEADER_BYTES>=PLANET_RECORDS_END");
        const uint8_t *planetName=&campaignCore.planetRecordStorage[planetOffset-PLANET_RECORDS_START+PLANET_NAME_HEADER_BYTES];
        uint8_t *write=campaignEventText.eventTextRecords[campaignEvents.currentEventId]+2;
        while (*write!=EVENT_TEXT_PLACEHOLDER)
        {
            if (++write==campaignEventText.eventTextStorage+sizeof(campaignEventText.eventTextStorage)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: ++write==eventTextStorage+sizeof(eventTextStorage)");
        }
        ++write;
        uint16_t padding=EVENT_PLANET_NAME_FIELD_BYTES;
        while (*planetName)
        {
            if (write==campaignEventText.eventTextStorage+sizeof(campaignEventText.eventTextStorage)
                || planetName==campaignCore.planetRecordStorage+sizeof(campaignCore.planetRecordStorage)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: write==eventTextStorage+sizeof(eventTextStorage) || planetName==planetRecordStorage+sizeof(planetRecordStorage)");
            *write++=*planetName++;
            --padding;
        }
        do /* Native LOOP executes even with CX0. No bounded-copy rewrite. */
        {
            if (write==campaignEventText.eventTextStorage+sizeof(campaignEventText.eventTextStorage)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: write==eventTextStorage+sizeof(eventTextStorage)"); /* Unrepresented wrap into the encoded directory, not a native length clamp. */
            *write++=EVENT_TEXT_PLACEHOLDER;
        } while (--padding!=0);
    }
    campaignEvents.eventOperandCursor=command->next;
}

/* MW_MAIN081B:66CC..67C4. Returned code offsets are nonlocal JMP transport,
 * not native AX results. The skipped interpreter call remains suspended. */
uint16_t MwEventOpenChoiceContinuation(const MwEventCommand *command)
{
    if (campaignEvents.eventConditionResult!=1)
    {campaignEvents.eventOperandCursor=command->next;return 0;}
    campaignEvents.eventChoiceCount=command->operand.choices.count;
    MwUiRecord *records=campaignEvents.eventChoiceCount==2?campaignEvents.eventTwoChoiceRecords:campaignEvents.eventThreeChoiceRecords;
    uint16_t choices=campaignEvents.eventChoiceCount==2?2:3;
    for (uint16_t i=0;i<choices;++i)
    {
        MwEventCaption *caption=command->operand.choices.captions[i];
        if (!caption) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !caption");
        records[i].text=caption->text;
        records[i].textColour=&caption->colour;
    }
    for (uint16_t i=0;i<choices;++i) campaignEvents.eventChoiceBranches[i]=command->operand.choices.branches[i];
    if (campaignEvents.currentEventId!=EVENT_DARK_WING_BASE_ATTACK)
    {
        if (campaignEvents.currentEventId>=CAMPAIGN_EVENT_SEEN_FLAGS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: currentEventId>=CAMPAIGN_EVENT_SEEN_FLAGS");
        campaignEvents.eventSeenFlags[campaignEvents.currentEventId]=1;
    }
    if (campaignEvents.campaignEventClass>=7 || !campaignEvents.eventClassHandlers[campaignEvents.campaignEventClass]) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: campaignEventClass>=7 || !eventClassHandlers[campaignEventClass]");
    MwTextPanelReturn display=campaignEvents.eventClassHandlers[campaignEvents.campaignEventClass](campaignEvents.currentEventId);
    /* Place choices relative to the text panel on either return path. */
    uint16_t x=display.textOriginX+0x000C;
    for (uint16_t i=0;i<2;++i) campaignEvents.eventTwoChoiceRecords[i].x=x;
    for (uint16_t i=0;i<3;++i) campaignEvents.eventThreeChoiceRecords[i].x=x;
    uint16_t y=(uint8_t)(campaignEvents.eventTextRowCount-campaignEvents.eventChoiceCount)*EVENT_CHOICE_ROW_PIXELS+display.textOriginY;
    campaignEvents.eventTwoChoiceRecords[0].y=campaignEvents.eventThreeChoiceRecords[0].y=y;
    y+=EVENT_CHOICE_ROW_PIXELS;
    campaignEvents.eventTwoChoiceRecords[1].y=campaignEvents.eventThreeChoiceRecords[1].y=y;
    y+=EVENT_CHOICE_ROW_PIXELS;campaignEvents.eventThreeChoiceRecords[2].y=y;
    campaignEvents.eventChoiceSavedFontIndex=EgaGetFontIndex();
    EgaSelectFont(0);
    MwSelectUiRecordTable(0,campaignEvents.eventChoiceCount==2?&campaignEvents.eventTwoChoiceMenu:&campaignEvents.eventThreeChoiceMenu);
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN081B:67C5..67F7.67D8 remains a shared label, not a Support method. */
uint16_t MwEventFirstChoiceCallback(void)
{
    EgaSelectFont(campaignEvents.eventChoiceSavedFontIndex);
    MwRestoreSavedTextPanelRegion();
    const MwCampaignEvent *branch=campaignEvents.eventChoiceBranches[0];
    if (campaignEvents.campaignRestartPending==1) return CAMPAIGN_CONTINUE_RESTART;
    if (campaignEvents.currentEventId==EVENT_DARK_WING_BASE_ATTACK) return CAMPAIGN_CONTINUE_EVENT_COMBAT;
    if (!campaignEventReturnContinuation) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !campaignEventReturnContinuation"); /* Unrepresented caller frame, not an ordinary UI RET. */
    if (campaignEvents.currentEventId==EVENT_CONTINUE_OR_QUIT) return campaignEventReturnContinuation;
    if (!branch) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !branch"); /* Encoded DS0000 branch is unrepresented, not success. */
    campaignEvents.currentEventId=branch->eventId;
    bool carry;
    uint16_t transfer=MwInterpretCampaignEvent(branch,&carry);
    return transfer?transfer:campaignEventReturnContinuation;
}

/* MW_MAIN081B:67F8..6827. Restart quit is tested before panel close; the
 * shared67D8 comparisons are reread AFTER close for the ordinary branch. */
uint16_t MwEventSecondChoiceCallback(void)
{
    EgaSelectFont(campaignEvents.eventChoiceSavedFontIndex);
    if (campaignEvents.campaignRestartPending==1) return CAMPAIGN_CONTINUE_EVENT_QUIT;
    MwRestoreSavedTextPanelRegion();
    if (campaignEvents.currentEventId==EVENT_DARK_WING_BASE_ATTACK) return CAMPAIGN_CONTINUE_EVENT_COMBAT_RETURN;
    if (campaignEvents.currentEventId==EVENT_CONTINUE_OR_QUIT) return CAMPAIGN_CONTINUE_EVENT_QUIT;
    const MwCampaignEvent *branch=campaignEvents.eventChoiceBranches[1];
    if (campaignEvents.campaignRestartPending==1) return CAMPAIGN_CONTINUE_RESTART;
    if (campaignEvents.currentEventId==EVENT_DARK_WING_BASE_ATTACK) return CAMPAIGN_CONTINUE_EVENT_COMBAT;
    if (!campaignEventReturnContinuation) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !campaignEventReturnContinuation");
    if (campaignEvents.currentEventId==EVENT_CONTINUE_OR_QUIT) return campaignEventReturnContinuation;
    if (!branch) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !branch");
    campaignEvents.currentEventId=branch->eventId;
    bool carry;
    uint16_t transfer=MwInterpretCampaignEvent(branch,&carry);
    return transfer?transfer:campaignEventReturnContinuation;
}

/* MW_MAIN081B:6828..683C, JMP67D8. No invented count guard: branch2 can be
 * stale after a two-choice dialog, just as the original stored WORD can. */
uint16_t MwEventThirdChoiceCallback(void)
{
    EgaSelectFont(campaignEvents.eventChoiceSavedFontIndex);
    MwRestoreSavedTextPanelRegion();
    const MwCampaignEvent *branch=campaignEvents.eventChoiceBranches[2];
    if (campaignEvents.campaignRestartPending==1) return CAMPAIGN_CONTINUE_RESTART;
    if (campaignEvents.currentEventId==EVENT_DARK_WING_BASE_ATTACK) return CAMPAIGN_CONTINUE_EVENT_COMBAT;
    if (!campaignEventReturnContinuation) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !campaignEventReturnContinuation");
    if (campaignEvents.currentEventId==EVENT_CONTINUE_OR_QUIT) return campaignEventReturnContinuation;
    if (!branch) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !branch");
    campaignEvents.currentEventId=branch->eventId;
    bool carry;
    uint16_t transfer=MwInterpretCampaignEvent(branch,&carry);
    return transfer?transfer:campaignEventReturnContinuation;
}

/* MW_MAIN081B:683D..6854; shared015F cleanup follows the explicit music stop. */
uint16_t MwQuitFromEventContinuation(void)
{
    if (!campaignStopMusic) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !campaignStopMusic");
    campaignStopMusic();
    campaignStartup.launcherExitCode=CAMPAIGN_EXIT_QUIT;
    return CAMPAIGN_CONTINUE_CLEANUP;
}

/* MW_MAIN081B:6913..692A. INC BL wraps independently of the planet WORD. */
uint16_t MwPrepareEventSpecialCombatContinuation(void)
{
    campaignMission.combatPlanetSelector=(uint8_t)(campaignCore.currentPlanetIndex+1);
    campaignMission.deploymentLandscapeIndex=(uint8_t)campaignCore.currentPlanetLandscapeIndex;
    campaignMission.combatScenarioCode=COMBAT_SCENARIO_DARK_WING_BASE;
    return CAMPAIGN_CONTINUE_EVENT_DEPLOYMENT;
}
