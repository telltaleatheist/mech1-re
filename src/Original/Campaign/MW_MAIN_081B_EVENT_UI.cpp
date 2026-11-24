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
    if (textId>=EVENT_TEXT_DIRECTORY_COUNT || !eventTextRecords[textId]) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: textId>=EVENT_TEXT_DIRECTORY_COUNT || !eventTextRecords[textId]"); /* Unrepresented zero-directory alias, not a native empty-text test. */
    const uint8_t *page=eventTextRecords[textId];
    MwNativeByteView text={eventTextStorage,EVENT_TEXT_STORAGE_START,sizeof(eventTextStorage)};
    for (;;)
    {
        if (*page==EVENT_TEXT_PLACEHOLDER)
        {
            ++page;
            campaignUi.textPanelContinuationMode=TEXT_PANEL_MODE_DISCARD_RETURN;
        }
        const MwEventTextPageHeader *header=(const MwEventTextPageHeader *)page;
        eventTextRowCount=header->rowCount;
        page+=sizeof(*header);
        MwTextPanelReturn result=MwShowCompactTextPanel(0x000C,0x000A,0x002F,
            eventTextRowCount+3,&text,(uint16_t)(page-eventTextStorage+EVENT_TEXT_STORAGE_START));
        if (result.discardReturnWord) return result; /*100C RET bypasses6210..621E. */
        page=&eventTextStorage[result.textOffset-EVENT_TEXT_STORAGE_START+1];
        if (page>=eventTextStorage+sizeof(eventTextStorage)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: page>=eventTextStorage+sizeof(eventTextStorage)");
        if (!*page)
        {
            result.textOffset=(uint16_t)(page-eventTextStorage+EVENT_TEXT_STORAGE_START);
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
    campaignRestartPending=0;
    for (uint16_t i=0;i<CAMPAIGN_NOTIFICATION_COUNT;++i) campaignEconomy.notificationPending[i]=0;
    unidentifiedField18=0;
    missionCategoryOrMemoryTier=0;
    deploymentLandscapeIndex=0;
    combatEnemyHeavyCount=0;
    combatEnemyMediumCount=0;
    combatEnemyLightCount=0;
    combatPlanetSelector=0;
    combatScenarioCode=0;
    deploymentTerrainDetailLevel=0;
    for (size_t i=0;i<sizeof(campaignMechanics.rosterStatus)/sizeof(campaignMechanics.rosterStatus[0]);++i) campaignMechanics.rosterStatus[i]=0;
    for (size_t i=0;i<sizeof(salvageEntries)/sizeof(salvageEntries[0]);++i) salvageEntries[i]=CampaignSalvageEntry{};
    returnedDisabledEnemyCount=0;
    combatTransitionResult=0;
    acceptedContractActive=0;
    currentPlanetIndex=0;
    currentPlanetLandscapeIndex=0;
    currentPlanetHouseIndex=0;
    currentPlanetMarketTier=0;
    currentPlanetFacilityDisplayFlag=0;
    currentPlanetMapPositionX=0;
    currentPlanetMapPositionY=0;
    planetMarketPremiumPercent=0;
    campaignEconomy.calendarSubmonthUnits=0;
    calendarMonthIndex=0;
    calendarYear=0;
    campaignEconomy.marketRefreshUnits=0;
    campaignEconomy.elapsedBillingMonths=0;
    for (size_t i=0;i<sizeof(commanderNameBytes)/sizeof(commanderNameBytes[0]);++i) commanderNameBytes[i]=0;
    for (size_t i=0;i<sizeof(unidentifiedField19)/sizeof(unidentifiedField19[0]);++i) unidentifiedField19[i]=0;
    commanderAgeYears=0;
    campaignEconomy.campaignFundsLow=0;
    campaignEconomy.campaignFundsHigh=0;
    for (size_t i=0;i<sizeof(houseNetStandings)/sizeof(houseNetStandings[0]);++i) houseNetStandings[i]=0;
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
    contractNegotiationAttemptLimit=0;
    requestedContractPaymentThousands=0;
    requestedContractSalvagePercent=0;
    requestedContractAdvancePercent=0;
    requestContractPaymentEnabled=0;
    requestContractSalvageEnabled=0;
    requestContractAdvanceEnabled=0;
    for (size_t i=0;i<sizeof(unidentifiedField20)/sizeof(unidentifiedField20[0]);++i) unidentifiedField20[i]=0;
    acceptedContractPaymentThousands=0;
    acceptedContractSalvagePercent=0;
    acceptedContractAdvancePercent=0;
    remainingContractPaymentThousands=0;
    for (size_t i=0;i<sizeof(campaignEconomy.houseMissionTypeLimits)/sizeof(campaignEconomy.houseMissionTypeLimits[0]);++i) campaignEconomy.houseMissionTypeLimits[i]=0;
    audioOptionLabelIndex=0;
    for (size_t i=0;i<sizeof(campaignEconomy.missionScenarioCodes)/sizeof(campaignEconomy.missionScenarioCodes[0]);++i) campaignEconomy.missionScenarioCodes[i]=0;
    for (size_t i=0;i<sizeof(campaignMechanics.pilotGunneryTiers)/sizeof(campaignMechanics.pilotGunneryTiers[0]);++i) campaignMechanics.pilotGunneryTiers[i]=0;
    for (size_t i=0;i<sizeof(campaignMechanics.pilotPilotingTiers)/sizeof(campaignMechanics.pilotPilotingTiers[0]);++i) campaignMechanics.pilotPilotingTiers[i]=0;
    for (size_t i=0;i<sizeof(campaignMechanics.pilotExperienceCounts)/sizeof(campaignMechanics.pilotExperienceCounts[0]);++i) campaignMechanics.pilotExperienceCounts[i]=0;
    eventDirectoryIndex=0;
    for (size_t i=0;i<sizeof(eventSeenFlags)/sizeof(eventSeenFlags[0]);++i) eventSeenFlags[i]=0;
    for (size_t i=0;i<sizeof(campaignEventPlanetSelectors)/sizeof(campaignEventPlanetSelectors[0]);++i) campaignEventPlanetSelectors[i]=0;
    campaignEconomy.majorCampaignMissionRollThreshold=0;
    acceptedContractTargetHouseIndex=0;
    campaignTerrainDetailLevel=0;
    campaignEconomy.dealerUnitPurchaseLimit=0;
    campaignEconomy.unitsSoldSinceMarketRefresh=0;
    playerSkillProgress=0;
    darkWingBaseVictoryRecorded=0;
    campaignEconomy.calendarSubmonthUnits=1;calendarMonthIndex=3;calendarYear=0x0BD0;
    commanderAgeYears=COMMANDER_INITIAL_AGE_YEARS;
    campaignEconomy.campaignFundsLow=(uint16_t)DECIMAL_MILLION;campaignEconomy.campaignFundsHigh=DECIMAL_MILLION>>NATIVE_WORD_BITS;
    pilotRosterCount=1;
    for (uint16_t i=0;i<CAMPAIGN_ROSTER_SLOTS;++i) campaignMechanics.rosterMechAssignments[i]=UINT16_MAX;
    for (uint16_t i=0;i<CAMPAIGN_OWNED_UNIT_SLOTS;++i)
    {campaignEconomy.ownedUnitKinds[i]=UINT16_MAX;campaignEconomy.marketUnitKinds[i]=UINT16_MAX;}
    for (size_t i=0;i<sizeof(campaignEconomy.houseMissionTypeLimits);++i)
        campaignEconomy.houseMissionTypeLimits[i]=defaultHouseMissionTypeLimits[i];
    for (size_t i=0;i<sizeof(commanderNameBytes);++i)
        commanderNameBytes[i]=campaignDefaultCommanderNameBytes[i];
    eventDirectoryIndex=1;campaignTerrainDetailLevel=2;
    return CAMPAIGN_CONTINUE_INITIALIZE_COMPANY; /* Native JMP into the same166D body, not a new procedure. */
}

/* MW_MAIN081B:6153..61BE. This style deliberately does not parse FF. Only
 * intermediate pages acknowledge input; the final page remains displayed. */
MwTextPanelReturn MwShowClassThreeEventPages(uint16_t textId)
{
    EgaSelectFont(0);
    EgaFillDisplayPageRectangle(EVENT_PAGE_PANEL_X,EVENT_PAGE_PANEL_Y,EVENT_PAGE_PANEL_WIDTH,EVENT_PAGE_PANEL_HEIGHT,Campaign_Pal_EGA_Black);
    if (textId>=EVENT_TEXT_DIRECTORY_COUNT || !eventTextRecords[textId]) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: textId>=EVENT_TEXT_DIRECTORY_COUNT || !eventTextRecords[textId]");
    const uint8_t *page=eventTextRecords[textId];
    MwNativeByteView text={eventTextStorage,EVENT_TEXT_STORAGE_START,sizeof(eventTextStorage)};
    for (;;)
    {
        page+=sizeof(MwEventTextPageHeader);
        uint16_t cursor=EgaDrawTextWithLineSpacing(EVENT_PAGE_TEXT_X,EVENT_PAGE_TEXT_Y,Campaign_Pal_EGA_Green,&text,(uint16_t)(page-eventTextStorage+EVENT_TEXT_STORAGE_START));
        page=&eventTextStorage[cursor-EVENT_TEXT_STORAGE_START+1];
        if (page>=eventTextStorage+sizeof(eventTextStorage)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: page>=eventTextStorage+sizeof(eventTextStorage)");
        if (!*page)
        {
            MwTextPanelReturn result={(uint16_t)(page-eventTextStorage+EVENT_TEXT_STORAGE_START),0x1AA5,0,false};
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
    if (eventConditionResult==1)
    {
        uint16_t reference=(uint16_t)command->operand.planetSelector^UINT16_MAX;
        if (reference>=6 || currentEventId>=EVENT_TEXT_DIRECTORY_COUNT
            || !eventTextRecords[currentEventId]) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: reference>=6 || currentEventId>=EVENT_TEXT_DIRECTORY_COUNT || !eventTextRecords[currentEventId]");
        uint16_t planetIndex=campaignEventPlanetSelectors[reference]-1;
        if (planetIndex>=CAMPAIGN_PLANET_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: planetIndex>=CAMPAIGN_PLANET_COUNT");
        uint16_t planetOffset=planetRecordOffsets[planetIndex];
        if (planetOffset<PLANET_RECORDS_START || planetOffset+PLANET_NAME_HEADER_BYTES>=PLANET_RECORDS_END) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: planetOffset<PLANET_RECORDS_START || planetOffset+PLANET_NAME_HEADER_BYTES>=PLANET_RECORDS_END");
        const uint8_t *planetName=&planetRecordStorage[planetOffset-PLANET_RECORDS_START+PLANET_NAME_HEADER_BYTES];
        uint8_t *write=eventTextRecords[currentEventId]+2;
        while (*write!=EVENT_TEXT_PLACEHOLDER)
        {
            if (++write==eventTextStorage+sizeof(eventTextStorage)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: ++write==eventTextStorage+sizeof(eventTextStorage)");
        }
        ++write;
        uint16_t padding=EVENT_PLANET_NAME_FIELD_BYTES;
        while (*planetName)
        {
            if (write==eventTextStorage+sizeof(eventTextStorage)
                || planetName==planetRecordStorage+sizeof(planetRecordStorage)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: write==eventTextStorage+sizeof(eventTextStorage) || planetName==planetRecordStorage+sizeof(planetRecordStorage)");
            *write++=*planetName++;
            --padding;
        }
        do /* Native LOOP executes even with CX0. No bounded-copy rewrite. */
        {
            if (write==eventTextStorage+sizeof(eventTextStorage)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: write==eventTextStorage+sizeof(eventTextStorage)"); /* Unrepresented wrap into the encoded directory, not a native length clamp. */
            *write++=EVENT_TEXT_PLACEHOLDER;
        } while (--padding!=0);
    }
    eventOperandCursor=command->next;
}

/* MW_MAIN081B:66CC..67C4. Returned code offsets are nonlocal JMP transport,
 * not native AX results. The skipped interpreter call remains suspended. */
uint16_t MwEventOpenChoiceContinuation(const MwEventCommand *command)
{
    if (eventConditionResult!=1)
    {eventOperandCursor=command->next;return 0;}
    eventChoiceCount=command->operand.choices.count;
    MwUiRecord *records=eventChoiceCount==2?eventTwoChoiceRecords:eventThreeChoiceRecords;
    uint16_t choices=eventChoiceCount==2?2:3;
    for (uint16_t i=0;i<choices;++i)
    {
        MwEventCaption *caption=command->operand.choices.captions[i];
        if (!caption) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !caption");
        records[i].text=caption->text;
        records[i].textColour=&caption->colour;
    }
    for (uint16_t i=0;i<choices;++i) eventChoiceBranches[i]=command->operand.choices.branches[i];
    if (currentEventId!=EVENT_DARK_WING_BASE_ATTACK)
    {
        if (currentEventId>=CAMPAIGN_EVENT_SEEN_FLAGS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: currentEventId>=CAMPAIGN_EVENT_SEEN_FLAGS");
        eventSeenFlags[currentEventId]=1;
    }
    if (campaignEventClass>=7 || !eventClassHandlers[campaignEventClass]) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: campaignEventClass>=7 || !eventClassHandlers[campaignEventClass]");
    MwTextPanelReturn display=eventClassHandlers[campaignEventClass](currentEventId);
    /* Place choices relative to the text panel on either return path. */
    uint16_t x=display.textOriginX+0x000C;
    for (uint16_t i=0;i<2;++i) eventTwoChoiceRecords[i].x=x;
    for (uint16_t i=0;i<3;++i) eventThreeChoiceRecords[i].x=x;
    uint16_t y=(uint8_t)(eventTextRowCount-eventChoiceCount)*EVENT_CHOICE_ROW_PIXELS+display.textOriginY;
    eventTwoChoiceRecords[0].y=eventThreeChoiceRecords[0].y=y;
    y+=EVENT_CHOICE_ROW_PIXELS;
    eventTwoChoiceRecords[1].y=eventThreeChoiceRecords[1].y=y;
    y+=EVENT_CHOICE_ROW_PIXELS;eventThreeChoiceRecords[2].y=y;
    eventChoiceSavedFontIndex=EgaGetFontIndex();
    EgaSelectFont(0);
    MwSelectUiRecordTable(0,eventChoiceCount==2?&eventTwoChoiceMenu:&eventThreeChoiceMenu);
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN081B:67C5..67F7.67D8 remains a shared label, not a Support method. */
uint16_t MwEventFirstChoiceCallback(void)
{
    EgaSelectFont(eventChoiceSavedFontIndex);
    MwRestoreSavedTextPanelRegion();
    const MwCampaignEvent *branch=eventChoiceBranches[0];
    if (campaignRestartPending==1) return CAMPAIGN_CONTINUE_RESTART;
    if (currentEventId==EVENT_DARK_WING_BASE_ATTACK) return CAMPAIGN_CONTINUE_EVENT_COMBAT;
    if (!campaignEventReturnContinuation) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !campaignEventReturnContinuation"); /* Unrepresented caller frame, not an ordinary UI RET. */
    if (currentEventId==EVENT_CONTINUE_OR_QUIT) return campaignEventReturnContinuation;
    if (!branch) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !branch"); /* Encoded DS0000 branch is unrepresented, not success. */
    currentEventId=branch->eventId;
    bool carry;
    uint16_t transfer=MwInterpretCampaignEvent(branch,&carry);
    return transfer?transfer:campaignEventReturnContinuation;
}

/* MW_MAIN081B:67F8..6827. Restart quit is tested before panel close; the
 * shared67D8 comparisons are reread AFTER close for the ordinary branch. */
uint16_t MwEventSecondChoiceCallback(void)
{
    EgaSelectFont(eventChoiceSavedFontIndex);
    if (campaignRestartPending==1) return CAMPAIGN_CONTINUE_EVENT_QUIT;
    MwRestoreSavedTextPanelRegion();
    if (currentEventId==EVENT_DARK_WING_BASE_ATTACK) return CAMPAIGN_CONTINUE_EVENT_COMBAT_RETURN;
    if (currentEventId==EVENT_CONTINUE_OR_QUIT) return CAMPAIGN_CONTINUE_EVENT_QUIT;
    const MwCampaignEvent *branch=eventChoiceBranches[1];
    if (campaignRestartPending==1) return CAMPAIGN_CONTINUE_RESTART;
    if (currentEventId==EVENT_DARK_WING_BASE_ATTACK) return CAMPAIGN_CONTINUE_EVENT_COMBAT;
    if (!campaignEventReturnContinuation) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !campaignEventReturnContinuation");
    if (currentEventId==EVENT_CONTINUE_OR_QUIT) return campaignEventReturnContinuation;
    if (!branch) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !branch");
    currentEventId=branch->eventId;
    bool carry;
    uint16_t transfer=MwInterpretCampaignEvent(branch,&carry);
    return transfer?transfer:campaignEventReturnContinuation;
}

/* MW_MAIN081B:6828..683C, JMP67D8. No invented count guard: branch2 can be
 * stale after a two-choice dialog, just as the original stored WORD can. */
uint16_t MwEventThirdChoiceCallback(void)
{
    EgaSelectFont(eventChoiceSavedFontIndex);
    MwRestoreSavedTextPanelRegion();
    const MwCampaignEvent *branch=eventChoiceBranches[2];
    if (campaignRestartPending==1) return CAMPAIGN_CONTINUE_RESTART;
    if (currentEventId==EVENT_DARK_WING_BASE_ATTACK) return CAMPAIGN_CONTINUE_EVENT_COMBAT;
    if (!campaignEventReturnContinuation) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !campaignEventReturnContinuation");
    if (currentEventId==EVENT_CONTINUE_OR_QUIT) return campaignEventReturnContinuation;
    if (!branch) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !branch");
    currentEventId=branch->eventId;
    bool carry;
    uint16_t transfer=MwInterpretCampaignEvent(branch,&carry);
    return transfer?transfer:campaignEventReturnContinuation;
}

/* MW_MAIN081B:683D..6854; shared015F cleanup follows the explicit music stop. */
uint16_t MwQuitFromEventContinuation(void)
{
    if (!campaignStopMusic) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !campaignStopMusic");
    campaignStopMusic();
    launcherExitCode=CAMPAIGN_EXIT_QUIT;
    return CAMPAIGN_CONTINUE_CLEANUP;
}

/* MW_MAIN081B:6913..692A. INC BL wraps independently of the planet WORD. */
uint16_t MwPrepareEventSpecialCombatContinuation(void)
{
    combatPlanetSelector=(uint8_t)(currentPlanetIndex+1);
    deploymentLandscapeIndex=(uint8_t)currentPlanetLandscapeIndex;
    combatScenarioCode=COMBAT_SCENARIO_DARK_WING_BASE;
    return CAMPAIGN_CONTINUE_EVENT_DEPLOYMENT;
}
