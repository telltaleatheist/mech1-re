#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/campaign_continuations.h"
#include "mechwarrior/campaign_mechanics.h"
#include "mechwarrior/campaign_pilots.h"
#include "mechwarrior/campaign_startup.h"
#include "mechwarrior/campaign_events.h"
#include "mechwarrior/ega_campaign.h"
#include <stdlib.h>
#include <string.h>

/* MW_MAIN081B:692E..69C9. Shared6950 skips music,698F skips background,
 *69B8 resumes AFTER class01 event dispatch.6950 POPs the dispatcher word. */
uint16_t MwEnterBarContinuation(uint16_t entryOffset)
{
    if (entryOffset==CAMPAIGN_CONTINUE_BAR_EVENT_RETURN) goto drawMenu;
    if (entryOffset==CAMPAIGN_CONTINUE_BAR_REFRESH) goto refreshMenu;
    if (entryOffset==CAMPAIGN_CONTINUE_BAR_BACKGROUND) goto drawBackground;
    if (entryOffset!=CAMPAIGN_CONTINUE_BAR) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: entryOffset!=CAMPAIGN_CONTINUE_BAR");
    if (campaignMission.audioOptionLabelIndex!=AUDIO_LABEL_OFF)
    {
        if (!campaignStartMusic) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !campaignStartMusic");
        campaignStartMusic(campaignStartup.campaignMusicBuffer,0);
    }
drawBackground:
    campaignUi.hubSelectedRecordIndex=BAR_HUB_ROW;
    campaignPilots.recruitOfferSlot=0;
    EgaClearDrawPage(BAR_BACKGROUND_COLOUR);
    MwDrawIndexedPicture(0,0,BAR_BACKGROUND_PICTURE);
    EgaSelectFont(BAR_FONT);
    if (campaignCore.currentPlanetLandscapeIndex!=0)
        MwDrawIndexedPicture(0,0,(uint16_t)(campaignCore.currentPlanetLandscapeIndex+BAR_LANDSCAPE_PICTURE_BASE));
refreshMenu:
    EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
    MwShowPendingNotifications();
    if (campaignUi.barMenuSelectedRow==BAR_FIRST_VISIT)
    {
        campaignUi.barMenuSelectedRow=0;
        campaignEventDispatchReturnContinuation=CAMPAIGN_CONTINUE_BAR_EVENT_RETURN;
        uint16_t transfer=MwDispatchCampaignEvents(BAR_INITIAL_EVENT_CLASS,CAMPAIGN_CALL_DISPATCH_EVENTS);
        if (transfer) return transfer;
        campaignEventDispatchReturnContinuation=0;
    }
drawMenu:
    MwDrawDescriptorTextPanel(&campaignPilots.barMenuPanel);
    MwSelectUiRecordTable(campaignUi.barMenuSelectedRow,&campaignPilots.barMenu);
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN081B:69CA..6A05. Native callback keeps its return until6950 POP.
 *6A03 is the post6236 continuation; there is no direct drink-price debit. */
uint16_t MwOrderDrinkContinuation(uint16_t entryOffset)
{
    if (entryOffset==CAMPAIGN_CONTINUE_DRINK_EVENT_RETURN) return CAMPAIGN_CONTINUE_BAR_BACKGROUND;
    if (entryOffset!=CAMPAIGN_CONTINUE_ORDER_DRINK) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: entryOffset!=CAMPAIGN_CONTINUE_ORDER_DRINK");
    campaignUi.barMenuSelectedRow=BAR_ROW_DRINK;
    EgaClearDrawPage(BAR_BACKGROUND_COLOUR);
    MwDrawIndexedPicture(0,0,BAR_DRINK_PICTURE);
    EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
    MwAdvanceCampaignTimeAndChargeWages(1);
    campaignEventDispatchReturnContinuation=CAMPAIGN_CONTINUE_DRINK_EVENT_RETURN;
    uint16_t transfer=MwDispatchCampaignEvents(BAR_DRINK_EVENT_CLASS,CAMPAIGN_CALL_DISPATCH_EVENTS);
    if (transfer) return transfer;
    campaignEventDispatchReturnContinuation=0;
    return CAMPAIGN_CONTINUE_BAR_BACKGROUND;
}

/* MW_MAIN081B:6B40..6B68. Search forward without wrapping to earlier offers.
 * The exhausted cursor is not an invitation to recruit from Mech inventory. */
bool MwFindNextRecruitCandidate(void)
{
    if (campaignEconomy.availableRecruitCount==0) return false;
    while (campaignPilots.recruitOfferSlot<RECRUIT_OFFER_SLOTS)
    {
        uint16_t pilotId=campaignEconomy.availableRecruitIds[campaignPilots.recruitOfferSlot];
        if (pilotId!=0) {campaignPilots.selectedRecruitPilotId=pilotId;return true;}
        ++campaignPilots.recruitOfferSlot;
    }
    return false;
}

/* MW_MAIN081B:6A06..6B30. Discards the dispatch word then tails089B/698F.
 * No candidate: no time charge. Successful search is followed by time BEFORE
 * display, so refreshes can alter live ratings/offer state in between. */
uint16_t MwOpenRecruitInterviewContinuation(void)
{
    campaignUi.barMenuSelectedRow=BAR_ROW_RECRUIT;
    if (!MwFindNextRecruitCandidate())
    {
        EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
        MwText text=MwArrayText(campaignPilots.noAvailableRecruitsText);
        MwShowMonospaceTextPanel(NO_RECRUITS_PANEL_X,NO_RECRUITS_PANEL_Y,NO_RECRUITS_PANEL_WIDTH,NO_RECRUITS_PANEL_HEIGHT,text);
        return CAMPAIGN_CONTINUE_BAR_REFRESH;
    }
    MwAdvanceCampaignTimeAndChargeWages(1);
    EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
    MwDrawDescriptorTextPanel(&campaignPilots.recruitInterviewPanel);
    EgaFillDisplayPageRectangle(RECRUIT_STAT_PANEL_X,RECRUIT_STAT_PANEL_Y,RECRUIT_STAT_PANEL_WIDTH,RECRUIT_STAT_PANEL_HEIGHT,PILOT_CARD_BACKGROUND);
    MwDrawPicturePanelBorder(RECRUIT_STAT_PANEL_X,RECRUIT_STAT_PANEL_Y,RECRUIT_STAT_PANEL_WIDTH,RECRUIT_STAT_PANEL_HEIGHT);
    MwText labels=MwArrayText(campaignPilots.recruitStatLabels);
    EgaDrawTransparentText(RECRUIT_LABELS_X,RECRUIT_NAME_Y,PILOT_CARD_TEXT_COLOUR,labels);
    const MwPilotDefinitionPrefix *pilot=campaignPilots.pilotDefinitions[campaignPilots.selectedRecruitPilotId]; /* Saved6A9C across drawing. */
    MwDrawRecruitStatText((const char *)pilot->nameBytes,RECRUIT_NAME_Y);
    MwDrawRecruitStatText(campaignPilots.pilotSkillLabels[campaignMechanics.pilotGunneryTiers[campaignPilots.selectedRecruitPilotId]],RECRUIT_GUNNERY_Y);
    MwDrawRecruitStatText(campaignPilots.pilotSkillLabels[campaignMechanics.pilotPilotingTiers[campaignPilots.selectedRecruitPilotId]],RECRUIT_PILOTING_Y);
    uint16_t wage=campaignEconomy.pilotWageAmounts[campaignMechanics.pilotGunneryTiers[campaignPilots.selectedRecruitPilotId]];
    ++campaignCore.numericGroupingEnabled;
    MwDrawSignedDecimalField(wage,RECRUIT_STATS_X,1,0,PILOT_CARD_TEXT_COLOUR,PILOT_CARD_BACKGROUND,RECRUIT_WAGE_Y);
    /* Native POP/PUSH DI retains this definition, not a later selection. */
    MwText dialogue=MwTerminatedText((const char *)(pilot+1));
    EgaDrawTransparentText(RECRUIT_DIALOGUE_X,RECRUIT_DIALOGUE_Y,RECRUIT_DIALOGUE_COLOUR,dialogue);
    MwDrawAlternateBankPicture(RECRUIT_STATS_X,RECRUIT_PORTRAIT_Y,EGA_DISPLAY_PAGE,pilot->portraitId);
    MwSelectUiRecordTable(0,&campaignPilots.recruitInterviewMenu);
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN081B:6B31..6B3F, genuine DI text/BX Y register-input callee. */
void MwDrawRecruitStatText(const char *text,uint16_t y)
{
    MwText caption=MwTerminatedText(text);
    EgaDrawTransparentText(RECRUIT_STATS_X,y,PILOT_CARD_TEXT_COLOUR,caption);
}

/* MW_MAIN081B:6B69..6BF9. UI callback, POP dispatch IP/JMP698F. The capacity
 * test is equality only. Both full-roster and successful paths consume an offer. */
uint16_t MwHireSelectedRecruitContinuation(void)
{
    if (campaignPilots.recruitOfferSlot>=RECRUIT_OFFER_SLOTS) return CAMPAIGN_CONTINUE_BAR_REFRESH;
    uint16_t rosterSlot=pilotRosterCount;
    if (rosterSlot==(uint16_t)(campaignStartup.launcherMemoryTier+1))
    {
        EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
        const char *message=campaignStartup.launcherMemoryTier==FOUR_PILOT_MEMORY_TIER?campaignPilots.fullFourPilotRosterText:campaignPilots.fullThreePilotRosterText;
        MwText text=MwTerminatedText(message);
        MwShowMonospaceTextPanel(FULL_ROSTER_PANEL_X,FULL_ROSTER_PANEL_Y,FULL_ROSTER_PANEL_WIDTH,FULL_ROSTER_PANEL_HEIGHT,text);
    }
    else
    {
        if (rosterSlot>=CAMPAIGN_ROSTER_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: rosterSlot>=CAMPAIGN_ROSTER_SLOTS");
        uint16_t pilotId=campaignPilots.selectedRecruitPilotId;
        pilotUnavailableFlags[pilotId]=1;
        rosterPilotIds[rosterSlot]=pilotId;
        campaignMechanics.rosterPortraitIds[rosterSlot]=campaignPilots.pilotDefinitions[pilotId]->portraitId;
        /*6BC6 reloads the selection after portrait; preserve that live read. */
        pilotId=campaignPilots.selectedRecruitPilotId;
        campaignMechanics.rosterPayTiers[rosterSlot]=campaignMechanics.pilotGunneryTiers[pilotId];
        campaignMechanics.rosterGunneryTiers[rosterSlot]=campaignMechanics.pilotGunneryTiers[pilotId];
        campaignMechanics.rosterPilotingTiers[rosterSlot]=campaignMechanics.pilotPilotingTiers[pilotId];
        campaignMechanics.rosterMechAssignments[rosterSlot]=PILOT_ASSIGNMENT_NONE;
        ++pilotRosterCount; /* No0451/0502 clear, wage debit or time call. */
    }
    campaignEconomy.availableRecruitIds[campaignPilots.recruitOfferSlot]=0;
    --campaignEconomy.availableRecruitCount;
    return CAMPAIGN_CONTINUE_BAR_REFRESH;
}

/* MW_MAIN081B:6BFA..6C0B. Independent callback, same POP/JMP destination. */
uint16_t MwRejectSelectedRecruitContinuation(void)
{
    if (campaignPilots.recruitOfferSlot>=RECRUIT_OFFER_SLOTS) return CAMPAIGN_CONTINUE_BAR_REFRESH;
    campaignEconomy.availableRecruitIds[campaignPilots.recruitOfferSlot]=0;
    --campaignEconomy.availableRecruitCount; /* Native WORD wrap, no added zero-count guard. */
    return CAMPAIGN_CONTINUE_BAR_REFRESH;
}

/* MW_MAIN081B:247A..24F1, alternate24B6 omits the commander profile.
 * Both entries discard the UI callback word and tail089B. */
uint16_t MwEnterCompanyMenuContinuation(uint16_t entryOffset)
{
    if (entryOffset!=CAMPAIGN_CONTINUE_COMPANY_MENU && entryOffset!=CAMPAIGN_CONTINUE_COMPANY_MENU_WITHOUT_PROFILE) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: entryOffset!=CAMPAIGN_CONTINUE_COMPANY_MENU && entryOffset!=CAMPAIGN_CONTINUE_COMPANY_MENU_WITHOUT_PROFILE");
    if (entryOffset==CAMPAIGN_CONTINUE_COMPANY_MENU) campaignUi.hubSelectedRecordIndex=0;
    EgaSelectFont(COMPANY_MENU_FONT);
    if (entryOffset==CAMPAIGN_CONTINUE_COMPANY_MENU_WITHOUT_PROFILE) EgaClearDrawPage(BAR_BACKGROUND_COLOUR);
    MwDrawIndexedPicture(0,0,(uint16_t)(campaignCore.currentPlanetLandscapeIndex+COMPANY_LANDSCAPE_PICTURE_BASE));
    EgaFlipPages();
    if (entryOffset==CAMPAIGN_CONTINUE_COMPANY_MENU) MwDrawCommanderProfile();
    MwDrawDescriptorTextPanel(&campaignPilots.companyMenuPanel);
    MwSelectUiRecordTable(campaignUi.companyMenuSelectedRow,&campaignPilots.companyMenu);
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN081B:24F2..260B genuine CALL.24FC/2513/25BA are fall-throughs,
 * not additional source methods or CALL frames. */
void MwDrawCommanderProfile(void)
{
    EgaSelectFont(0);
    for (uint16_t house=0;house<CAMPAIGN_HOUSE_COUNT;++house)
        campaignMission.houseNetStandings[house]=(uint16_t)(campaignEconomy.houseFavorPoints[house]-campaignEconomy.houseGrievancePoints[house]);
    campaignPilots.commanderProfilePanel.width=campaignMechanics.rosterPayTiers[COMMANDER_ROSTER_SLOT]==COMMANDER_REPUTATION_WORTH_WATCHING?COMPANY_PROFILE_WATCHING_WIDTH:COMPANY_PROFILE_WIDTH;
    campaignPilots.commanderProfilePanel.x=COMPANY_PROFILE_X; /* Native252A discards the centering calculation. */
    MwDrawDescriptorTextPanel(&campaignPilots.commanderProfilePanel);
    MwText name=MwArrayText(campaignPilots.commanderProfileName);
    EgaDrawTransparentText((uint16_t)(campaignPilots.commanderProfilePanel.x+COMPANY_PROFILE_NAME_X),COMPANY_PROFILE_NAME_Y,RECRUIT_DIALOGUE_COLOUR,name);
    /* The commander's pay-table entry holds reputation, not a salary grade. */
    uint16_t commanderReputationTier=campaignMechanics.rosterPayTiers[COMMANDER_ROSTER_SLOT];
    if (commanderReputationTier>=COMMANDER_REPUTATION_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: commanderReputationTier>=COMMANDER_REPUTATION_COUNT");
    MwText reputationText=MwArrayText(campaignPilots.commanderReputationLabels[commanderReputationTier]);
    EgaDrawTransparentText((uint16_t)(campaignPilots.commanderProfilePanel.x+COMPANY_REPUTATION_X),COMPANY_REPUTATION_Y,PILOT_CARD_TEXT_COLOUR,reputationText);
    campaignMission.commanderAgeYears=campaignCore.calendarYear-COMMANDER_AGE_YEAR_BIAS;
    if (campaignCore.calendarMonthIndex>=COMMANDER_AGE_MONTH_THRESHOLD) ++campaignMission.commanderAgeYears;
    MwDrawSignedDecimalField(campaignMission.commanderAgeYears,(uint16_t)(campaignPilots.commanderProfilePanel.x+COMPANY_AGE_X),1,0,PILOT_CARD_TEXT_COLOUR,PILOT_CARD_BACKGROUND,COMPANY_AGE_Y);
    ++campaignCore.numericGroupingEnabled;
    MwDrawSignedDecimalField(((uint32_t)campaignEconomy.campaignFundsHigh<<NATIVE_WORD_BITS)|campaignEconomy.campaignFundsLow,
        (uint16_t)(campaignPilots.commanderProfilePanel.x+COMPANY_WEALTH_X),1,0,PILOT_CARD_TEXT_COLOUR,PILOT_CARD_BACKGROUND,COMPANY_WEALTH_Y);
    for (uint16_t house=0;house<CAMPAIGN_HOUSE_COUNT;++house)
    {
        int16_t standing=campaignMission.houseNetStandings[house];
        if (standing<-COMPANY_HOUSE_STANDING_LIMIT) standing=-COMPANY_HOUSE_STANDING_LIMIT;
        else if (standing>COMPANY_HOUSE_STANDING_LIMIT) standing=COMPANY_HOUSE_STANDING_LIMIT;
        uint16_t attitude=(uint16_t)(((uint16_t)standing&0xFFFE)+COMPANY_HOUSE_STANDING_LIMIT)>>1;
        MwText text=MwTerminatedText(campaignPilots.houseAttitudeText[attitude]);
        EgaDrawTransparentText((uint16_t)(campaignPilots.commanderProfilePanel.x+COMPANY_HOUSE_ATTITUDE_X),(uint16_t)(COMPANY_HOUSE_FIRST_Y+house*COMPANY_HOUSE_ROW_HEIGHT),Campaign_Pal_EGA_LightGrey,text);
    }
    EgaSelectFont(COMPANY_MENU_FONT);
}

/* MW_MAIN081B:260C..273D. POP/time/roster redraw is also the dismissal tail. */
uint16_t MwEnterCrewMenuContinuation(void)
{
    campaignUi.companyMenuSelectedRow=0;
    MwAdvanceCampaignTimeAndChargeWages(1);
    for (uint16_t rosterSlot=FIRST_HIRED_PILOT_SLOT;rosterSlot<CAMPAIGN_ROSTER_SLOTS;++rosterSlot)
        campaignPilots.crewMenu.records[rosterSlot+CREW_FIRST_PILOT_MENU_RECORD].enabled=0;
    for (uint16_t rosterSlot=FIRST_HIRED_PILOT_SLOT;rosterSlot<CAMPAIGN_ROSTER_SLOTS && rosterSlot!=pilotRosterCount;++rosterSlot)
        campaignPilots.crewMenu.records[rosterSlot+CREW_FIRST_PILOT_MENU_RECORD].enabled=1;
    EgaClearDrawPage(PILOT_CARD_BACKGROUND);
    EgaFillDrawPageRectangle(CREW_FULL_RIGHT_EDGE,0,CREW_RIGHT_MARGIN_WIDTH,EGA_ROWS,BAR_BACKGROUND_COLOUR);
    EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
    EgaSelectFont(CREW_FONT);
    campaignPilots.crewLabelsPanel.width=(uint8_t)pilotRosterCount*PILOT_CARD_COLUMN_WIDTH+PILOT_CARD_FIRST_X;
    MwDrawDescriptorTextPanel(&campaignPilots.crewLabelsPanel);
    MwDrawAlternateBankPicture(0,0,EGA_DISPLAY_PAGE,PILOT_CARD_LABEL_PICTURE);
    MwDrawAlternateBankPicture(0,PILOT_CARD_MECH_Y,EGA_DISPLAY_PAGE,PILOT_CARD_LABEL_PICTURE);
    campaignPilots.crewScreenRightEdge=CREW_FULL_RIGHT_EDGE;
    if (campaignStartup.launcherMemoryTier!=FOUR_PILOT_MEMORY_TIER)
    {
        campaignPilots.crewMenu.records[THREE_PILOT_ROSTER_CAPACITY].rightRecordIndex=CREW_NO_NEIGHBOUR;
        campaignPilots.crewScreenRightEdge=CREW_SMALL_RIGHT_EDGE;
    } /* Tier3 does NOT restore a previously changed23C1 byte. */
    EgaDrawLine(PILOT_CARD_TEXT_COLOUR,PILOT_CARD_NAME_Y-3,campaignPilots.crewScreenRightEdge,0,PILOT_CARD_NAME_Y-3);
    EgaDrawLine(PILOT_CARD_TEXT_COLOUR,PILOT_CARD_MECH_Y-1,campaignPilots.crewScreenRightEdge,0,PILOT_CARD_MECH_Y-1);
    EgaDrawLine(Campaign_Pal_EGA_LightGrey,0,campaignPilots.crewScreenRightEdge,0,0);
    EgaDrawLine(PILOT_CARD_TEXT_COLOUR,CREW_LAST_Y,campaignPilots.crewScreenRightEdge,0,CREW_LAST_Y);
    for (uint16_t rosterSlot=0;rosterSlot<PILOT_CARD_COUNT;++rosterSlot) MwDrawPilotRosterCard(rosterSlot);
    MwSelectUiRecordTable(0,&campaignPilots.crewMenu);
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN081B:273E..286E. The card snapshot is separate from persistent
 * roster fields. Empty cards retain it; commander duplicates Gunnery. */
void MwDrawPilotRosterCard(uint16_t pilotRosterSlot)
{
    campaignPilots.pilotCardRosterSlot=pilotRosterSlot;
    if (pilotRosterSlot==LAST_PILOT_CARD_SLOT && campaignStartup.launcherMemoryTier!=FOUR_PILOT_MEMORY_TIER) return;
    if (pilotRosterSlot>=PILOT_CARD_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: pilotRosterSlot>=PILOT_CARD_COUNT");
    campaignPilots.pilotCardTextX=campaignPilots.pilotCardColumnOrigins[pilotRosterSlot]+PILOT_CARD_TEXT_INSET;
    MwDrawPilotCardSeparators();
    if (pilotRosterSlot>=pilotRosterCount) return;
    uint16_t pilotId=rosterPilotIds[pilotRosterSlot];
    campaignPilots.displayedPilotId=pilotId;
    campaignPilots.pilotCardGunneryTier=campaignMechanics.pilotGunneryTiers[pilotId];
    campaignPilots.pilotCardPilotingTier=campaignMechanics.pilotPilotingTiers[pilotId];
    campaignPilots.pilotCardPortraitId=campaignMechanics.rosterPortraitIds[pilotRosterSlot];
    /* The card quotes current Gunnery rates; payroll uses the hiring-time tier. */
    campaignPilots.pilotCardQuotedWageTier=campaignMechanics.pilotGunneryTiers[pilotId];
    campaignPilots.pilotCardOwnedUnitSlot=campaignMechanics.rosterMechAssignments[pilotRosterSlot];
    if (pilotRosterSlot==0)
        campaignPilots.pilotCardGunneryTier=campaignPilots.pilotCardPilotingTier=campaignMechanics.rosterGunneryTiers[0];
    MwDrawAlternateBankPicture((uint16_t)(campaignPilots.pilotCardTextX-PILOT_CARD_TEXT_INSET),0,EGA_DISPLAY_PAGE,
        campaignPilots.pilotCardRosterSlot==0?PILOT_CARD_COMMANDER_PICTURE:campaignPilots.pilotCardPortraitId);
    pilotId=campaignPilots.displayedPilotId;
    MwDrawPilotCardText(pilotId?(const char *)campaignPilots.pilotDefinitions[pilotId]->nameBytes:campaignPilots.commanderProfileName,PILOT_CARD_NAME_Y);
    MwDrawPilotCardText(campaignPilots.pilotSkillLabels[campaignPilots.pilotCardGunneryTier],PILOT_CARD_GUNNERY_Y);
    MwDrawPilotCardText(campaignPilots.pilotSkillLabels[campaignPilots.pilotCardPilotingTier],PILOT_CARD_PILOTING_Y);
    if (campaignPilots.pilotCardRosterSlot!=0)
    {
        uint16_t wage=campaignEconomy.pilotWageAmounts[campaignPilots.pilotCardQuotedWageTier];
        ++campaignCore.numericGroupingEnabled;
        MwDrawSignedDecimalField(wage,campaignPilots.pilotCardTextX,1,0,Campaign_Pal_EGA_LightGrey,Campaign_Pal_EGA_DarkGrey,PILOT_CARD_WAGE_Y);
    }
    uint16_t assignedSlot=campaignPilots.pilotCardOwnedUnitSlot;
    uint16_t assignedMechKind=assignedSlot==PILOT_ASSIGNMENT_NONE?PILOT_ASSIGNMENT_NONE:campaignEconomy.ownedUnitKinds[assignedSlot];
    MwDrawPilotCardText(MwSetPilotAssignmentCaption(assignedMechKind),PILOT_CARD_ASSIGNMENT_Y);
    assignedSlot=campaignPilots.pilotCardOwnedUnitSlot;
    if ((int16_t)assignedSlot>=0)
    {
        MwCampaignBuffer *mechPicture=MwLoadIndexedPicture((uint16_t)(campaignEconomy.ownedUnitKinds[assignedSlot]+PILOT_MECH_PICTURE_BASE));
        EgaDrawPackedSprite((uint16_t)(campaignPilots.pilotCardTextX-PILOT_CARD_TEXT_INSET),PILOT_CARD_MECH_Y,EGA_DISPLAY_PAGE,0,mechPicture);
    }
}

/* MW_MAIN081B:286F..28A2, reload shared X between the two graphics calls. */
void MwDrawPilotCardSeparators(void)
{
    uint16_t x=campaignPilots.pilotCardTextX-PILOT_CARD_TEXT_INSET;
    EgaDrawLine(Campaign_Pal_EGA_LightGrey,0,x,x,EGA_ROWS);
    x=campaignPilots.pilotCardTextX-PILOT_CARD_TEXT_INSET+PILOT_CARD_COLUMN_WIDTH-1;
    EgaDrawLine(Campaign_Pal_EGA_LightGrey,0,x,x,EGA_ROWS);
}

/* MW_MAIN081B:28A3..28B1, genuine DI text/BX Y callee. */
void MwDrawPilotCardText(const char *text,uint16_t y)
{
    MwText caption=MwTerminatedText(text);
    EgaDrawTransparentText(campaignPilots.pilotCardTextX,y,PILOT_CARD_TEXT_COLOUR,caption);
}

/* MW_MAIN081B:28B2..28E6, native DI result becomes a typed caption reference.
 * Colour prefix and text have canonical owners; no negative C subscripts. */
const char *MwSetPilotAssignmentCaption(uint16_t assignedMechKind)
{
    uint16_t assignmentCaptionIndex;
    if (assignedMechKind==PILOT_ASSIGNMENT_DISMISS) assignmentCaptionIndex=ASSIGNMENT_DISMISS_CAPTION;
    else if (assignedMechKind==PILOT_ASSIGNMENT_NONE) assignmentCaptionIndex=ASSIGNMENT_NONE_CAPTION;
    else assignmentCaptionIndex=assignedMechKind+ASSIGNMENT_MECH_CAPTION_BASE;
    if (assignmentCaptionIndex>=PILOT_ASSIGNMENT_CAPTION_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: assignmentCaptionIndex>=PILOT_ASSIGNMENT_CAPTION_COUNT");
    uint16_t pilotMenuRecord=(campaignPilots.pilotCardRosterSlot<LAST_PILOT_CARD_SLOT?campaignPilots.pilotCardRosterSlot:LAST_PILOT_CARD_SLOT)+CREW_FIRST_PILOT_MENU_RECORD;
    campaignPilots.crewMenu.records[pilotMenuRecord].text=campaignPilots.pilotAssignmentCaptions[assignmentCaptionIndex];
    campaignPilots.crewMenu.records[pilotMenuRecord].textColour=&campaignPilots.pilotAssignmentCaptionColours[assignmentCaptionIndex];
    return campaignPilots.pilotAssignmentCaptions[assignmentCaptionIndex];
}

/* MW_MAIN081B:28E7/28EC/28F1/28F6 load slots0..3 then share28F9..2910.
 * Native AX is the slot, no added source frame at the shared continuation. */
uint16_t MwBeginPilotMechAssignmentContinuation(uint16_t pilotRosterSlot)
{
    campaignPilots.assignmentSelectedPilotSlot=pilotRosterSlot;
    campaignPilots.assignmentConflictHighlighted=0;
    if (campaignPilots.assignmentSelectedPilotSlot>=CAMPAIGN_ROSTER_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: assignmentSelectedPilotSlot>=CAMPAIGN_ROSTER_SLOTS");
    campaignPilots.assignmentCandidateOwnedSlot=campaignMechanics.rosterMechAssignments[campaignPilots.assignmentSelectedPilotSlot];
    MwHighlightSelectedPilotAssignment();
    return CAMPAIGN_CONTINUE_PILOT_ASSIGNMENT_INPUT;
}

/* MW_MAIN081B:2911..2947 and JMP297F..29E2. Enter returns the native2948
 * transfer; navigation labels share this loop, not extra callable methods. */
uint16_t MwPollPilotMechAssignmentContinuation(void)
{
    for (;;)
    {
        uint16_t key;
        if (!MwPollNavigationKeyFromMouseOrJoystick(&key))
        {
            if (!mwCampaignInput.keyboardAvailable || !mwCampaignInput.readKeyboard) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mwCampaignInput.keyboardAvailable || !mwCampaignInput.readKeyboard");
            if (!mwCampaignInput.keyboardAvailable()) continue;
            key=MwCampaignKeyWord(mwCampaignInput.readKeyboard());
        }
        if (key==CAMPAIGN_KEY_ENTER) return CAMPAIGN_CONTINUE_CONFIRM_PILOT_ASSIGNMENT;
        if (key==CAMPAIGN_KEY_ESCAPE)
        {
            if (!mwCampaignInput.controlHeld) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mwCampaignInput.controlHeld");
            if (mwCampaignInput.controlHeld()) return CAMPAIGN_CONTINUE_CLEANUP;
            continue; /* Plain Escape is NOT a cancel action here. */
        }
        uint16_t candidateOwnedUnitSlot=campaignPilots.assignmentCandidateOwnedSlot;
        if (key==CAMPAIGN_KEY_UP)
        {
            for (;;)
            {
                if (candidateOwnedUnitSlot==PILOT_ASSIGNMENT_DISMISS || (candidateOwnedUnitSlot==PILOT_ASSIGNMENT_NONE && campaignPilots.assignmentSelectedPilotSlot==COMMANDER_ROSTER_SLOT)) goto pollAgain;
                --candidateOwnedUnitSlot;
                if (candidateOwnedUnitSlot==PILOT_ASSIGNMENT_NONE || candidateOwnedUnitSlot==PILOT_ASSIGNMENT_DISMISS) break;
                if (candidateOwnedUnitSlot>=CAMPAIGN_OWNED_UNIT_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: candidateOwnedUnitSlot>=CAMPAIGN_OWNED_UNIT_SLOTS");
                if (campaignEconomy.ownedUnitKinds[candidateOwnedUnitSlot]!=CAMPAIGN_VACANT_UNIT_KIND && campaignEconomy.ownedUnitKinds[candidateOwnedUnitSlot]!=CAMPAIGN_UNIT_DROPSHIP) break;
            }
        }
        else if (key==CAMPAIGN_KEY_DOWN)
        {
            for (;;)
            {
                if (candidateOwnedUnitSlot==CAMPAIGN_OWNED_UNIT_SLOTS-1) goto pollAgain;
                ++candidateOwnedUnitSlot;
                /* Native dismiss->unassigned navigation filters the company
                 * unit count as a kind. Preserve this existing menu quirk. */
                uint16_t candidateMechKind;
                if (candidateOwnedUnitSlot==PILOT_ASSIGNMENT_NONE) candidateMechKind=campaignEconomy.ownedUnitCount;
                else
                {
                    if (candidateOwnedUnitSlot>=CAMPAIGN_OWNED_UNIT_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: candidateOwnedUnitSlot>=CAMPAIGN_OWNED_UNIT_SLOTS");
                    candidateMechKind=campaignEconomy.ownedUnitKinds[candidateOwnedUnitSlot];
                }
                if (candidateMechKind!=CAMPAIGN_VACANT_UNIT_KIND && candidateMechKind!=CAMPAIGN_UNIT_DROPSHIP) break;
            }
        }
        else continue;
        MwClearAssignmentConflictHighlight(); /* Saved BX survives redraw. */
        campaignPilots.assignmentCandidateOwnedSlot=candidateOwnedUnitSlot;
        MwHighlightSelectedPilotAssignment();
        MwFindAssignmentConflict();
pollAgain:;
    }
}

/* MW_MAIN081B:2A3B..2A97 falls2A98 on a match. Scan skips selected pilot,
 * first OTHER owner wins; zero count is not an invented no-op. */
void MwFindAssignmentConflict(void)
{
    uint16_t candidateOwnedUnitSlot=campaignPilots.assignmentCandidateOwnedSlot;
    if (candidateOwnedUnitSlot==PILOT_ASSIGNMENT_NONE) return;
    uint16_t rosterSlot=0;
    do
    {
        if (rosterSlot>=CAMPAIGN_ROSTER_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: rosterSlot>=CAMPAIGN_ROSTER_SLOTS");
        if (campaignMechanics.rosterMechAssignments[rosterSlot]==candidateOwnedUnitSlot && rosterSlot!=campaignPilots.assignmentSelectedPilotSlot)
        {
            campaignPilots.assignmentConflictHighlighted=1;
            campaignPilots.assignmentConflictingPilotSlot=rosterSlot;
            campaignPilots.assignmentConflictingOwnedSlot=candidateOwnedUnitSlot;
            EgaFillDisplayPageRectangle((uint16_t)(campaignPilots.pilotCardColumnOrigins[campaignPilots.assignmentConflictingPilotSlot]+PILOT_CARD_TEXT_INSET),
                PILOT_CARD_ASSIGNMENT_Y,ASSIGNMENT_TEXT_WIDTH,ASSIGNMENT_TEXT_HEIGHT,PILOT_CARD_BACKGROUND);
            MwDrawPilotAssignmentCandidate(campaignPilots.assignmentConflictingPilotSlot,campaignPilots.assignmentConflictingOwnedSlot,ASSIGNMENT_CONFLICT_COLOUR);
            return;
        }
        ++rosterSlot;
    } while (rosterSlot!=pilotRosterCount);
    /* No-match leaves prior conflict snapshots/latch untouched. */
}

/* MW_MAIN081B:2A98..2ACA, SI slot/BX candidate/DL colour, actual near CALL. */
void MwDrawPilotAssignmentCandidate(uint16_t pilotRosterSlot,uint16_t ownedUnitSlot,uint8_t foreground)
{
    uint16_t x=campaignPilots.pilotCardColumnOrigins[pilotRosterSlot]+PILOT_CARD_TEXT_INSET;
    uint16_t assignmentCaptionIndex;
    if (ownedUnitSlot==PILOT_ASSIGNMENT_NONE) assignmentCaptionIndex=ASSIGNMENT_NONE_CAPTION;
    else if (ownedUnitSlot==PILOT_ASSIGNMENT_DISMISS) assignmentCaptionIndex=ASSIGNMENT_DISMISS_CAPTION;
    else assignmentCaptionIndex=campaignEconomy.ownedUnitKinds[ownedUnitSlot]+ASSIGNMENT_MECH_CAPTION_BASE;
    if (assignmentCaptionIndex>=PILOT_ASSIGNMENT_CAPTION_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: assignmentCaptionIndex>=PILOT_ASSIGNMENT_CAPTION_COUNT");
    MwText text=MwArrayText(campaignPilots.pilotAssignmentCaptions[assignmentCaptionIndex]);
    EgaDrawTransparentText(x,PILOT_CARD_ASSIGNMENT_Y,foreground,text);
}

/* MW_MAIN081B:2ACB..2AE3 tails2A98. Clears any nonzero latch; redraw uses
 * CURRENT candidate2583, not the recorded conflicting-owned-slot2588. */
void MwClearAssignmentConflictHighlight(void)
{
    if (campaignPilots.assignmentConflictHighlighted==0) return;
    campaignPilots.assignmentConflictHighlighted=0;
    MwDrawPilotAssignmentCandidate(campaignPilots.assignmentConflictingPilotSlot,campaignPilots.assignmentCandidateOwnedSlot,PILOT_CARD_TEXT_COLOUR);
}

/* MW_MAIN081B:2AE4..2B05 falls2B06; no additional return word. */
void MwHighlightSelectedPilotAssignment(void)
{
    EgaFillDisplayPageRectangle((uint16_t)(campaignPilots.pilotCardColumnOrigins[campaignPilots.assignmentSelectedPilotSlot]+PILOT_CARD_TEXT_INSET),
        PILOT_CARD_ASSIGNMENT_Y,ASSIGNMENT_TEXT_WIDTH,ASSIGNMENT_TEXT_HEIGHT,PILOT_CARD_BACKGROUND);
    MwDrawSelectedPilotAssignment(ASSIGNMENT_SELECTED_COLOUR);
}

/* MW_MAIN081B:2B06..2B14, read candidate again AFTER the text CALL. */
void MwDrawSelectedPilotAssignment(uint8_t foreground)
{
    MwDrawPilotAssignmentCandidate(campaignPilots.assignmentSelectedPilotSlot,campaignPilots.assignmentCandidateOwnedSlot,foreground);
    MwDrawAssignmentMechPreview(campaignPilots.assignmentCandidateOwnedSlot);
}

/* MW_MAIN081B:2B15..2B9C. Saved origin survives all four negative-preview
 * draw calls. Arbitrary signed-negative candidates clear the preview. */
void MwDrawAssignmentMechPreview(uint16_t ownedUnitSlot)
{
    if ((int16_t)ownedUnitSlot>=0)
    {
        if (ownedUnitSlot>=CAMPAIGN_OWNED_UNIT_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: ownedUnitSlot>=CAMPAIGN_OWNED_UNIT_SLOTS");
        MwCampaignBuffer *mechPicture=MwLoadIndexedPicture((uint16_t)(campaignEconomy.ownedUnitKinds[ownedUnitSlot]+PILOT_MECH_PICTURE_BASE));
        EgaDrawPackedSprite(campaignPilots.pilotCardColumnOrigins[campaignPilots.assignmentSelectedPilotSlot],PILOT_CARD_MECH_Y,EGA_DISPLAY_PAGE,0,mechPicture);
        return;
    }
    uint16_t x=campaignPilots.pilotCardColumnOrigins[campaignPilots.assignmentSelectedPilotSlot];
    EgaFillDisplayPageRectangle((uint16_t)(x+1),PILOT_CARD_MECH_Y,ASSIGNMENT_MECH_WIDTH,ASSIGNMENT_MECH_HEIGHT,PILOT_CARD_BACKGROUND);
    EgaDrawLine(PILOT_CARD_TEXT_COLOUR,PILOT_CARD_MECH_Y,x,x,EGA_ROWS);
    EgaDrawLine(PILOT_CARD_TEXT_COLOUR,PILOT_CARD_MECH_Y,(uint16_t)(x+PILOT_CARD_COLUMN_WIDTH-1),(uint16_t)(x+PILOT_CARD_COLUMN_WIDTH-1),EGA_ROWS);
    EgaDrawLine(PILOT_CARD_TEXT_COLOUR,CREW_LAST_Y,(uint16_t)(x+ASSIGNMENT_MECH_WIDTH),x,CREW_LAST_Y);
}

/* MW_MAIN081B:2948..297E. Native RET normally, JMP29E3 for exactDISMISS.
 * Draw and conflict transfer precede the selected pilot's assignment write. */
uint16_t MwConfirmPilotMechAssignmentContinuation(void)
{
    MwDrawSelectedPilotAssignment(ASSIGNMENT_CONFIRMED_COLOUR);
    MwTransferPreviousMechToConflictingPilot();
    uint16_t candidateOwnedUnitSlot=campaignPilots.assignmentCandidateOwnedSlot;
    if (candidateOwnedUnitSlot==PILOT_ASSIGNMENT_DISMISS) return MwDismissSelectedPilotContinuation(campaignPilots.assignmentSelectedPilotSlot);
    campaignMechanics.rosterMechAssignments[campaignPilots.assignmentSelectedPilotSlot]=candidateOwnedUnitSlot;
    campaignPilots.pilotCardRosterSlot=campaignPilots.assignmentSelectedPilotSlot;
    uint16_t assignedMechKind=candidateOwnedUnitSlot==PILOT_ASSIGNMENT_NONE?PILOT_ASSIGNMENT_NONE:campaignEconomy.ownedUnitKinds[candidateOwnedUnitSlot];
    MwSetPilotAssignmentCaption(assignedMechKind);
    return 0;
}

/* MW_MAIN081B:2B9D..2C12. Saved old assignment and conflicting slot survive
 * graphics. Highlight latch is deliberately NOT cleared by confirmation. */
void MwTransferPreviousMechToConflictingPilot(void)
{
    if (campaignPilots.assignmentConflictHighlighted==0) return;
    uint16_t previousOwnedSlot=campaignMechanics.rosterMechAssignments[campaignPilots.assignmentSelectedPilotSlot];
    uint16_t conflictingPilotSlot=campaignPilots.assignmentConflictingPilotSlot;
    campaignMechanics.rosterMechAssignments[conflictingPilotSlot]=previousOwnedSlot;
    EgaFillDisplayPageRectangle((uint16_t)(campaignPilots.pilotCardColumnOrigins[campaignPilots.assignmentConflictingPilotSlot]+PILOT_CARD_TEXT_INSET),
        PILOT_CARD_ASSIGNMENT_Y,ASSIGNMENT_TEXT_WIDTH,ASSIGNMENT_TEXT_HEIGHT,PILOT_CARD_BACKGROUND);
    MwDrawPilotAssignmentCandidate(conflictingPilotSlot,previousOwnedSlot,PILOT_CARD_TEXT_COLOUR);
    campaignPilots.pilotCardRosterSlot=campaignPilots.assignmentConflictingPilotSlot;
    uint16_t previousMechKind=previousOwnedSlot==PILOT_ASSIGNMENT_NONE?PILOT_ASSIGNMENT_NONE:campaignEconomy.ownedUnitKinds[previousOwnedSlot];
    MwSetPilotAssignmentCaption(previousMechKind);
    uint16_t savedSelectedPilot=campaignPilots.assignmentSelectedPilotSlot;
    campaignPilots.assignmentSelectedPilotSlot=campaignPilots.assignmentConflictingPilotSlot;
    MwDrawAssignmentMechPreview(previousOwnedSlot);
    campaignPilots.assignmentSelectedPilotSlot=savedSelectedPilot;
}

/* MW_MAIN081B:29E3..2A3A. Voluntary dismissal leaves mission-status bytes alone;
 * unlike casualty removal it neither shifts those bytes nor deletes Mechs.
 * The C argument is a roster slot; native SI encodes twice this value. */
uint16_t MwDismissSelectedPilotContinuation(uint16_t rosterSlot)
{
    uint16_t pilotId=rosterPilotIds[rosterSlot];
    pilotUnavailableFlags[pilotId]=0;
    --pilotRosterCount;
    while (rosterSlot!=CAMPAIGN_ROSTER_SLOTS-1)
    {
        rosterPilotIds[rosterSlot]=rosterPilotIds[rosterSlot+1];
        campaignMechanics.rosterPayTiers[rosterSlot]=campaignMechanics.rosterPayTiers[rosterSlot+1];
        campaignMechanics.rosterPortraitIds[rosterSlot]=campaignMechanics.rosterPortraitIds[rosterSlot+1];
        campaignMechanics.rosterGunneryTiers[rosterSlot]=campaignMechanics.rosterGunneryTiers[rosterSlot+1];
        campaignMechanics.rosterPilotingTiers[rosterSlot]=campaignMechanics.rosterPilotingTiers[rosterSlot+1];
        campaignMechanics.rosterUnknownWords[rosterSlot]=campaignMechanics.rosterUnknownWords[rosterSlot+1];
        campaignMechanics.rosterMechAssignments[rosterSlot]=campaignMechanics.rosterMechAssignments[rosterSlot+1];
        ++rosterSlot;
    }
    campaignMechanics.rosterMechAssignments[rosterSlot]=PILOT_ASSIGNMENT_NONE;
    return CAMPAIGN_CONTINUE_CREW_MENU; /* Original roster rebuild charges time; not executed twice. */
}

/* MW_MAIN 081B:6CD5..6CF9. Definitions are variable-length; the directory,
 * not sizeof(prefix), selects each pilot. Pilot0/player is untouched. */
void MwInitializePilotRatingsFromDefinitions(void)
{
    for (uint16_t pilotId = 1; pilotId < CAMPAIGN_PILOT_ID_COUNT; ++pilotId)
    {
        uint16_t pilotDefinitionOffset = campaignMechanics.pilotDefinitionOffsets[pilotId];
        if (pilotDefinitionOffset < PILOT_DEFINITIONS_START
            || (uint32_t)pilotDefinitionOffset + sizeof(MwPilotDefinitionPrefix) > PILOT_DEFINITIONS_END) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: pilotDefinitionOffset < PILOT_DEFINITIONS_START || (uint32_t)pilotDefinitionOffset + sizeof(MwPilotDefinitionPrefix) > PILOT_DEFINITIONS_END");
        const MwPilotDefinitionPrefix *pilotDefinition = (const MwPilotDefinitionPrefix *)&campaignMechanics.pilotDefinitionStorage[pilotDefinitionOffset - PILOT_DEFINITIONS_START];
        campaignMechanics.pilotGunneryTiers[pilotId] = pilotDefinition->initialGunneryTier;
        campaignMechanics.pilotPilotingTiers[pilotId] = pilotDefinition->initialPilotingTier;
        campaignMechanics.pilotExperienceCounts[pilotId] = 0;
    }
}

/* MW_MAIN 081B:21D9..2232. BYTE experience uses signed JGE thresholds;
 * Gunnery promotion also sets Piloting, but does not update the wage tier. */
void MwAdvancePilotSkillFromExperience(uint16_t pilotRosterSlot)
{
    if (pilotRosterSlot >= CAMPAIGN_ROSTER_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: pilotRosterSlot >= CAMPAIGN_ROSTER_SLOTS");
    if (campaignMechanics.rosterMechAssignments[pilotRosterSlot] == CAMPAIGN_NO_MECH_ASSIGNMENT) return;
    uint16_t pilotId = rosterPilotIds[pilotRosterSlot];
    if (pilotId >= CAMPAIGN_PILOT_ID_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: pilotId >= CAMPAIGN_PILOT_ID_COUNT");
    campaignMechanics.pilotExperienceCounts[pilotId]++;
    uint8_t promotionThreshold;
    switch (campaignMechanics.pilotGunneryTiers[pilotId])
    {
    case PILOT_SKILL_POOR: promotionThreshold = PILOT_POOR_ADVANCE_COUNT; break;
    case PILOT_SKILL_AVERAGE: promotionThreshold = PILOT_AVERAGE_ADVANCE_COUNT; break;
    case PILOT_SKILL_GOOD: promotionThreshold = PILOT_GOOD_ADVANCE_COUNT; break;
    default: campaignMechanics.pilotExperienceCounts[pilotId] = 0; return;
    }
    /* Avoid implementation-defined unsigned-to-signed conversion in C17. */
    if (campaignMechanics.pilotExperienceCounts[pilotId] >= NATIVE_BYTE_SIGN_BIT
        || campaignMechanics.pilotExperienceCounts[pilotId] < promotionThreshold) return;
    campaignMechanics.pilotGunneryTiers[pilotId]++;
    campaignMechanics.pilotPilotingTiers[pilotId] = campaignMechanics.pilotGunneryTiers[pilotId];
    campaignMechanics.rosterGunneryTiers[pilotRosterSlot] = campaignMechanics.pilotGunneryTiers[pilotId];
    campaignMechanics.rosterPilotingTiers[pilotRosterSlot] = campaignMechanics.pilotGunneryTiers[pilotId];
    campaignMechanics.pilotExperienceCounts[pilotId] = 0;
}

/* MW_MAIN 081B:223F..22A8. Casualty removal also compacts mission status.
 * Clear only the final assignment; other trailing fields remain stale.
 * Native SI is twice the C roster-slot argument. */
uint16_t MwRemovePilotAndCompactRoster(uint16_t rosterSlot)
{
    if (rosterSlot >= CAMPAIGN_ROSTER_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: rosterSlot >= CAMPAIGN_ROSTER_SLOTS");
    uint16_t removedPilotId = rosterPilotIds[rosterSlot];
    if (campaignMechanics.rosterStatus[rosterSlot] == PILOT_STATUS_RELEASE_AVAILABILITY)
    {
        if (removedPilotId >= CAMPAIGN_PILOT_ID_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Removed MechWarrior ID is outside the campaign pilot definitions: removedPilotId >= CAMPAIGN_PILOT_ID_COUNT");
        pilotUnavailableFlags[removedPilotId]=0;
    }
    pilotRosterCount--;
    while (rosterSlot != CAMPAIGN_ROSTER_SLOTS - 1)
    {
        campaignMechanics.rosterStatus[rosterSlot] = campaignMechanics.rosterStatus[rosterSlot + 1];
        rosterPilotIds[rosterSlot] = rosterPilotIds[rosterSlot + 1];
        campaignMechanics.rosterPayTiers[rosterSlot] = campaignMechanics.rosterPayTiers[rosterSlot + 1];
        campaignMechanics.rosterPortraitIds[rosterSlot] = campaignMechanics.rosterPortraitIds[rosterSlot + 1];
        campaignMechanics.rosterGunneryTiers[rosterSlot] = campaignMechanics.rosterGunneryTiers[rosterSlot + 1];
        campaignMechanics.rosterPilotingTiers[rosterSlot] = campaignMechanics.rosterPilotingTiers[rosterSlot + 1];
        campaignMechanics.rosterUnknownWords[rosterSlot] = campaignMechanics.rosterUnknownWords[rosterSlot + 1];
        campaignMechanics.rosterMechAssignments[rosterSlot] = campaignMechanics.rosterMechAssignments[rosterSlot + 1];
        rosterSlot++;
    }
    campaignMechanics.rosterMechAssignments[rosterSlot] = CAMPAIGN_NO_MECH_ASSIGNMENT;
    return removedPilotId; /* Native DI result, not a new status code. */
}
