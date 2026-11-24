#include "mechwarrior/campaign.h"
#include "mechwarrior/read_word.h"
#include "mechwarrior/campaign_mechanics.h"
#include "mechwarrior/campaign_economy.h"
#include "mechwarrior/campaign_ui.h"
#include "mechwarrior/campaign_input.h"
#include "mechwarrior/campaign_service.h"
#include "mechwarrior/campaign_market.h"
#include "mechwarrior/original_data.h"
#include "mechwarrior/rules.h"
#include "mechwarrior/story.h"
#include <stdio.h>
#include "mechwarrior/campaign_startup.h"
#include "mechwarrior/campaign_events.h"
#include "mechwarrior/campaign_state.h"
#include "mechwarrior/campaign_pilots.h"
#include "mechwarrior/campaign_contracts.h"
#include "mechwarrior/campaign_travel.h"
#include "mechwarrior/campaign_options.h"
#include "mechwarrior/campaign_handoff.h"
#include "mechwarrior/ui_layout.h"
#include "mechwarrior/native_start.h"
#include "MW_MAIN_1090_LAYOUT.h"
#include <stdlib.h>
#include <string.h>

static_assert(sizeof(uint8_t) == 1, "Native BYTE width");
static_assert(sizeof(uint16_t) == 2, "Native WORD width");
CampaignInputState campaignInput;
CampaignHandoffState campaignHandoff={
    .afterActionFailureMessages={campaignHandoff.afterActionKuritaFailure,
    campaignHandoff.afterActionGeneralFailure,campaignHandoff.afterActionGeneralFailure,campaignHandoff.afterActionLiaoFailure,campaignHandoff.afterActionGeneralFailure},
    .afterActionSuccessMessages={campaignHandoff.afterActionKuritaSuccess,
    campaignHandoff.afterActionGeneralSuccess,campaignHandoff.afterActionGeneralSuccess,campaignHandoff.afterActionLiaoSuccess,campaignHandoff.afterActionGeneralSuccess},
    .settlementDeductionMessages={campaignHandoff.settlementSupplyDeductionText,
    campaignHandoff.settlementPerformanceDeductionText,campaignHandoff.settlementTransportDeductionText}
};
CampaignMissionState campaignMission;
CampaignEconomyState campaignEconomy;
CampaignCoreState campaignCore={
    /* Original 1090:03F5/03F7 executable initializers. Do not substitute libc rand
     * or randomize these before the native BIOS seed call. */
    .randomStateWord0=0x0615,
    .randomStateWord1=0x1988
};
const char indexedPictureFilename[] = "MW_PICS.BIN";
const char generalPictureFilename[] = "MW_GPICS.BIN";
const char cockpitPictureFilename[] = "MW_CPICS.BIN";
const char primaryPictureFilename[] = "MW_1PICS.BIN";
const char secondaryPictureFilename[] = "MW_2PICS.BIN";
const char tPictureFilename[] = "MW_TPICS.BIN";
const char alternatePictureFilename[] = "MW_APICS.BIN";
const char fPictureFilename[] = "MW_FPICS.BIN";
static_assert(sizeof(campaignCore.repairAttemptFailuresByKind) == 0x00DC, "Native reset extent");
CampaignMechanicsState campaignMechanics;
static MwUiRecord travelHouseBrowserRecords[TRAVEL_HOUSE_BROWSER_RECORD_COUNT];
static MwUiRecord travelHousePlanetRecords[TRAVEL_HOUSE_PLANET_RECORD_COUNT];
static MwUiRecord travelMapRecords[TRAVEL_MAP_MENU_RECORD_COUNT];
CampaignTravelState campaignTravel={
    .travelHouseBrowserMenu={travelHouseBrowserRecords,TRAVEL_HOUSE_BROWSER_RECORD_COUNT,
    &campaignCore.planetRecordStorage[0x686D-PLANET_RECORDS_START]},
    .travelHousePlanetMenu={travelHousePlanetRecords,TRAVEL_HOUSE_PLANET_RECORD_COUNT,
    NULL,&campaignCore.planetRecordOffsets[(0x65FA-0x6570)/sizeof(uint16_t)],8},
    .travelMapMenu={travelMapRecords,TRAVEL_MAP_MENU_RECORD_COUNT,
    &campaignCore.planetRecordStorage[0x697E -PLANET_RECORDS_START]}
};
/* Private backing arrays; shared access goes through the typed menus below. */
static MwUiRecord mechServiceUiRecords[0x0005];
static MwUiRecord dropShipServiceUiRecords[0x0002];
CampaignUiState campaignUi={
    .planetHubMenu={ campaignUi.planetHubRecords, HUB_UI_RECORD_COUNT, &campaignUi.hubFirstNavigationProbe },
    .contractHubMenu={ campaignUi.contractHubRecords, CONTRACT_HUB_UI_RECORD_COUNT, &campaignUi.hubSecondNavigationProbe },
    .remotePlanetHubMenu={ campaignUi.remotePlanetHubRecords, HUB_UI_RECORD_COUNT, &campaignUi.hubThirdNavigationProbe }
};
CampaignServiceState campaignService={
    /* FF probes alias362D (record15 X high),3B45 (kind9 failure8), and3B8B
     * (sensor price for kind8 low). Keep typed aliases, not host-endian BYTE casts. */
    .ownedUnitListMenu={ campaignService.ownedUnitListUiRecords, 0x0D, NULL, &campaignService.componentRepairUiRecords[15].x, 8 },
    .mechServiceMenu={ mechServiceUiRecords, 5, &campaignCore.repairAttemptFailuresByKind[9].failedByDamageIndex[8] },
    .dropShipServiceMenu={ dropShipServiceUiRecords, 2, NULL, &campaignMechanics.systemRepairBasePrices[REPAIR_SENSORS].basePriceByMechKind[CAMPAIGN_UNIT_MARAUDER], 0 },
    .componentRepairMenu={ campaignService.componentRepairUiRecords, REPAIR_UI_RECORD_COUNT, &marketCapacityNavigationProbe }
};
static MwUiRecord rearmConfirmationRecords[2], saleConfirmationRecords[2], marketUnitListRecords[0x0D];
static MwUiRecord marketMechRecords[3], marketDropShipRecords[2], marketComponentRecords[1], mechComplexRecords[4];
CampaignStartupState campaignStartup;
CampaignEventsState campaignEvents={
    .eventClassHandlers={MwShowCampaignEventText,MwShowCampaignEventText,
    MwShowCampaignEventText,MwShowClassThreeEventPages,MwShowCampaignEventText,
    MwShowCampaignEventText,MwShowCampaignEventText},
    /* FF-link probes alias existing planet text bytes, NOT new globals at6C0E/2A. */
    .eventTwoChoiceMenu={campaignEvents.eventTwoChoiceRecords,2,&campaignCore.planetRecordStorage[0x6C0E -PLANET_RECORDS_START],NULL,0},
    .eventThreeChoiceMenu={campaignEvents.eventThreeChoiceRecords,3,&campaignCore.planetRecordStorage[0x6C2A-PLANET_RECORDS_START],NULL,0}
};

/* Installation data import. Decode once into the canonical C objects;
 * the input is never retained. These are file-format operations, not new
 * native game methods. Pointer topology is the reviewed shipped revision. */
/* Month strings end at4B19.4B1A onward belongs to contract date/editor data. */
static uint8_t calendarMonthText[0x6E];
static void ImportInitialWords(const uint8_t *bytes, uint16_t offset, uint16_t *words, size_t count)
{
    for (size_t i=0;i<count;++i) words[i]=MwReadWord(bytes+offset+i*2);
}
/* A rule table, rebuilt from data/rules (src/Rules) as the exe lays it out. */
static bool ImportRuleBytes(const char *id, void *bytes, size_t size)
{
    return MwRulesCopyNative(id,(uint8_t *)bytes,size);
}
/* `count` words from byte `at` of the rule piece `id` (pieces up to 0x200 bytes). */
static bool ImportRuleWords(const char *id, uint16_t *words, size_t count, size_t at=0)
{
    const size_t size=MwRulesPieceSize(id);
    uint8_t bytes[0x200];
    if (!size || size>sizeof(bytes) || at+count*2>size || !MwRulesCopyNative(id,bytes,size)) return false;
    ImportInitialWords(bytes,(uint16_t)at,words,count);
    return true;
}
/* An intro caption's x and y (data/ui/intro.json), which the exe keeps before its text. */
static bool ImportIntroCaptionAt(const char *id, MwIntroPositionedText *caption)
{
    uint16_t at[2];
    if (!MwUiCopyWords(id,at,2,0)) return false;
    caption->x=at[0];caption->y=at[1];
    return true;
}
/* The story file's keys for the unit kinds (CampaignUnitKind order) and the
 * houses (HOUSE_* order), as src/Import/pieces.txt names them. */
static const char *const storyUnitKeys[CAMPAIGN_UNIT_KIND_COUNT]={"locust","wasp","jenner","phoenix_hawk",
    "shadow_hawk","wolverine","rifleman","warhammer","marauder","battlemaster","dropship"};
static const char *const storyHouseKeys[CAMPAIGN_HOUSE_COUNT]={"kurita","steiner","marik","liao","davion"};
static_assert(CAMPAIGN_UNIT_DROPSHIP==10 && HOUSE_DAVION==4,"the story keys follow the enums");
/* MW_MAIN's UI words that have left the exe (story/words.json: hub.word.*,
 * crew.word.*, contract_office.word.*, complex.word.*, dealer.word.*,
 * travel.word.*) at
 * their data-segment offsets, so the
 * importers below read them where the exe kept them. Filled first, by
 * MwImportCampaignInitialData; zero everywhere else. */
static uint8_t mainWords[0x10000];
static bool ImportMainWords(void)
{
    memset(mainWords,0,sizeof(mainWords));
    return MwWordsCopyInto("hub.word.",mainWords,sizeof(mainWords))
        && MwWordsCopyInto("crew.word.",mainWords,sizeof(mainWords))
        && MwWordsCopyInto("contract_office.word.",mainWords,sizeof(mainWords))
        && MwWordsCopyInto("complex.word.",mainWords,sizeof(mainWords))
        && MwWordsCopyInto("dealer.word.",mainWords,sizeof(mainWords))
        && MwWordsCopyInto("travel.word.",mainWords,sizeof(mainWords));
}
static bool ImportInitialString(const uint8_t *view, uint16_t offset, char *text, size_t capacity)
{
    if ((size_t)offset+capacity>0x10000) return false;
    const uint8_t *end=static_cast<const uint8_t *>(memchr(view+offset,0,capacity));
    if (!end) return false;
    memcpy(text,view+offset,(size_t)(end-(view+offset))+1);return true;
}
static bool ImportTravelUiData(void)
{
    /* data/ui/travel.json: the house's planet list (panel 54E4, menu 5809),
     * the house browser (5A0D, 5A7B) and the travel map (5BF9, 5C17, 5C35,
     * 5C75; menu 5B8C). */
    static const char *const planetIds[TRAVEL_PLANET_ROWS_PER_HOUSE]={"travel.caption.planet_1","travel.caption.planet_2",
        "travel.caption.planet_3","travel.caption.planet_4","travel.caption.planet_5","travel.caption.planet_6",
        "travel.caption.planet_7","travel.caption.planet_8","travel.caption.planet_9","travel.caption.planet_10",
        "travel.caption.planet_11","travel.caption.planet_12","travel.caption.planet_13","travel.caption.planet_14",
        "travel.caption.planet_15","travel.caption.planet_16","travel.caption.planet_17","travel.caption.planet_18",
        "travel.caption.planet_19","travel.caption.planet_20","travel.caption.planet_21","travel.caption.planet_22",
        "travel.caption.planet_23","travel.caption.planet_24","travel.caption.planet_25","travel.caption.planet_26",
        "travel.caption.planet_27","travel.caption.planet_28","travel.caption.planet_29","travel.caption.planet_30"};
    static const char *const browserIds[TRAVEL_HOUSE_BROWSER_LINE_COUNT]={"travel.caption.browser_title",
        "travel.caption.house_1","travel.caption.house_2","travel.caption.house_3","travel.caption.house_4",
        "travel.caption.house_5","travel.caption.browser_close"};
    UiCaptionOwner owners[2+TRAVEL_PLANET_ROWS_PER_HOUSE+TRAVEL_HOUSE_BROWSER_LINE_COUNT+5];
    size_t count=0;
    owners[count++]={"travel.caption.search",&campaignTravel.travelHouseSearchCaption.display,campaignTravel.travelHouseSearchCaption.text,sizeof(campaignTravel.travelHouseSearchCaption.text),0,0};
    for (uint16_t row=0;row<TRAVEL_PLANET_ROWS_PER_HOUSE;++row)
        owners[count++]={planetIds[row],&campaignTravel.travelHousePlanetRows[row].display,campaignTravel.travelHousePlanetRows[row].text,sizeof(campaignTravel.travelHousePlanetRows[row].text),0,0};
    owners[count++]={"travel.caption.planets_back",&campaignTravel.travelHousePlanetCancelLabel.display,campaignTravel.travelHousePlanetCancelLabel.text,sizeof(campaignTravel.travelHousePlanetCancelLabel.text),0,0};
    for (uint16_t line=0;line<TRAVEL_HOUSE_BROWSER_LINE_COUNT;++line)
        owners[count++]={browserIds[line],&campaignTravel.travelHouseBrowserLabels[line].display,campaignTravel.travelHouseBrowserLabels[line].text,sizeof(campaignTravel.travelHouseBrowserLabels[line].text),0,0};
    owners[count++]={"travel.caption.destination_name",&campaignTravel.travelDestinationHeaderLabels[0].display,campaignTravel.travelDestinationHeaderLabels[0].text,sizeof(campaignTravel.travelDestinationHeaderLabels[0].text),0,0};
    owners[count++]={"travel.caption.destination_detail",&campaignTravel.travelDestinationHeaderLabels[1].display,campaignTravel.travelDestinationHeaderLabels[1].text,sizeof(campaignTravel.travelDestinationHeaderLabels[1].text),0,0};
    owners[count++]={"travel.caption.confirm_travel",&campaignTravel.travelCostLabel.display,campaignTravel.travelCostLabel.text,sizeof(campaignTravel.travelCostLabel.text),0,0};
    owners[count++]={"travel.caption.leave",&campaignTravel.travelConfirmLabel.display,campaignTravel.travelConfirmLabel.text,sizeof(campaignTravel.travelConfirmLabel.text),0,0};
    owners[count++]={"travel.caption.browse_houses",&campaignTravel.travelBrowseLabel.display,campaignTravel.travelBrowseLabel.text,sizeof(campaignTravel.travelBrowseLabel.text),0,0};
    if (!MwImportUiCaptions(owners,count)) return false;
    /* DS:5C4E is one buffer in the original: 081B:5FEF copies the destination
     * name into it, the header panel's first caption draws it (5C49, colour at
     * 5C4D) and travel record 0 points at it, so the selection redraws the
     * name in the highlight colour. All three use travelDestinationName, so
     * the caption is pointed at it before the menu takes its text. */
    campaignTravel.travelDestinationHeaderLabels[0].display.text=campaignMechanics.travelDestinationName;
    if (!MwImportUiPanel("travel.panel.house_planets",&campaignTravel.travelHousePlanetPanel,TRAVEL_HOUSE_PLANET_LINE_COUNT,owners,count)
        || !MwImportUiPanel("travel.panel.house_browser",&campaignTravel.travelHouseBrowserPanel,TRAVEL_HOUSE_BROWSER_LINE_COUNT,owners,count)
        || !MwImportUiPanel("travel.panel.cost",&campaignTravel.travelCostPanel,1,owners,count)
        || !MwImportUiPanel("travel.panel.confirm",&campaignTravel.travelConfirmPanel,1,owners,count)
        || !MwImportUiPanel("travel.panel.destination",&campaignTravel.travelDestinationHeaderPanel,2,owners,count)
        || !MwImportUiPanel("travel.panel.browse",&campaignTravel.travelBrowsePanel,1,owners,count)
        || !MwImportUiMenu("travel.menu.house_planets",&campaignTravel.travelHousePlanetMenu,owners,count)
        || !MwImportUiMenu("travel.menu.house_browser",&campaignTravel.travelHouseBrowserMenu,owners,count)
        || !MwImportUiMenu("travel.menu.map",&campaignTravel.travelMapMenu,owners,count)) return false;
    /* The houses' search names (travel.word.house_search.*), by travel.json's house_search_words (59BB). */
    uint16_t houseSearchWords[TRAVEL_HOUSE_COUNT];
    if (!MwUiCopyWords("travel.table.house_search_words",houseSearchWords,TRAVEL_HOUSE_COUNT,0)) return false;
    for (uint16_t house=0;house<TRAVEL_HOUSE_COUNT;++house)
        if (!ImportInitialString(mainWords,houseSearchWords[house],campaignTravel.travelHouseSearchNames[house],sizeof(campaignTravel.travelHouseSearchNames[house]))) return false;
    if (!ImportInitialString(mainWords,0x5B83,campaignTravel.travelPopulationSuffix,sizeof(campaignTravel.travelPopulationSuffix))) return false;
    const uint16_t landscapeLabels[TRAVEL_LANDSCAPE_LABEL_COUNT]={0x5CA6,0x5CAD,0x5CB6};
    for (uint16_t label=0;label<TRAVEL_LANDSCAPE_LABEL_COUNT;++label)
        if (!ImportInitialString(mainWords,landscapeLabels[label],campaignTravel.travelLandscapeLabels[label],sizeof(campaignTravel.travelLandscapeLabels[label]))) return false;
    return true;
}
static MwUiRecord companyRecords[3],crewRecords[5],barRecords[3],recruitInterviewRecords[2];
/*0960 reads FF-link probes before rejecting the link. Options and restore
 * now own the mutable bytes also probed by Bar and recruitment. */
static uint8_t companyNavigationPadding;
CampaignPilotsState campaignPilots={
    .companyMenu={companyRecords,3,&companyNavigationPadding},
    .crewMenu={crewRecords,5,&rearmConfirmationRecords[1].textRecord},
    .barMenu={barRecords,3,NULL},
    .recruitInterviewMenu={recruitInterviewRecords,2,NULL,&campaignOptions.saveListRows[4].display.x,0}
};
static bool ImportPilotUiData(void);
static bool ImportReserveAmmoUiData(void);
static bool ImportContractData(void);
static bool CheckTextBoundTables(void);
bool MwImportCampaignInitialData(void)
{
    if (!ImportMainWords()) return false;
    /* The save block (1090:0448..0B5F, CAMPAIGN_SAVE_BYTES) from data/rules
     * (new_campaign.json), the commander's name from the story file, decoded
     * by the save codec as a loaded game's would be. */
    uint8_t newCampaign[CAMPAIGN_SAVE_BYTES];
    if (!ImportRuleBytes("new_campaign.before_name",newCampaign,0x003B)
        || !MwStoryCopyText("commander.default_name",(char *)newCampaign+0x003B,0x000A)
        || !ImportRuleBytes("new_campaign.after_name",newCampaign+0x0045,CAMPAIGN_SAVE_BYTES-0x0045)) return false;
    MwDecodeCampaignExchange(newCampaign);
    memset(&campaignInput.campaignTimerInhibited,0,sizeof(campaignInput.campaignTimerInhibited)); /*0007*/
    memset(&campaignInput.campaignTimerByte,0,sizeof(campaignInput.campaignTimerByte)); /*0008*/
    memcpy(campaignHandoff.combatExecutableFilename,mwCombatExecutableFilename,sizeof(campaignHandoff.combatExecutableFilename)); /*0EF6*/
    memset(&campaignHandoff.deploymentResolvedWithoutCombat,0,sizeof(campaignHandoff.deploymentResolvedWithoutCombat)); /*140C*/
    if (!MwStoryCopyText("dark_wing.victory",campaignHandoff.darkWingVictoryText,sizeof(campaignHandoff.darkWingVictoryText))) return false;
    memcpy(campaignHandoff.afterActionCrewHeading,mainWords+0x11DF,sizeof(campaignHandoff.afterActionCrewHeading));
    memcpy(campaignHandoff.afterActionMechHeading,mainWords+0x11EC,sizeof(campaignHandoff.afterActionMechHeading));
    memcpy(campaignHandoff.afterActionSalvageHeading,mainWords+0x11F9,sizeof(campaignHandoff.afterActionSalvageHeading));
    memcpy(campaignHandoff.afterActionPaymentHeading,mainWords+0x1202,sizeof(campaignHandoff.afterActionPaymentHeading));
    memcpy(campaignHandoff.afterActionPercentSuffix,mainWords+0x120B,sizeof(campaignHandoff.afterActionPercentSuffix));
    memcpy(campaignHandoff.afterActionPilotStatusText,mainWords+0x120D,sizeof(campaignHandoff.afterActionPilotStatusText));
    campaignHandoff.afterActionRosterRowOffset=0; /* 1229: Written before it's read; zero where the exe reloaded it. */
    if (!ImportRuleWords("units.tonnages",(uint16_t *)campaignHandoff.campaignMechTonnages,sizeof(campaignHandoff.campaignMechTonnages)/sizeof(uint16_t))) return false;
    campaignHandoff.settlementPayoutPercent=0; /* Written before it's read; zero where the exe reloaded it. */
    if (!ImportRuleWords("houses.deduction_roll_thresholds",(uint16_t *)campaignHandoff.houseDeductionRollThresholds,sizeof(campaignHandoff.houseDeductionRollThresholds)/sizeof(uint16_t))) return false;
    if (!ImportRuleWords("houses.reduced_payout_percent",(uint16_t *)campaignHandoff.houseReducedPayoutPercentages,sizeof(campaignHandoff.houseReducedPayoutPercentages)/sizeof(uint16_t))) return false;
    if (!ImportRuleWords("contracts.deployment_units_by_scenario",(uint16_t *)campaignHandoff.missionDeploymentUnitsByScenario,sizeof(campaignHandoff.missionDeploymentUnitsByScenario)/sizeof(uint16_t))) return false;
    if (!MwStoryCopyText("deployment.uneventful",campaignHandoff.deploymentUneventfulText,sizeof(campaignHandoff.deploymentUneventfulText))) return false;
    if (!MwStoryCopyText("after_action.failure.kurita",campaignHandoff.afterActionKuritaFailure,sizeof(campaignHandoff.afterActionKuritaFailure))) return false;
    if (!MwStoryCopyText("after_action.failure.liao",campaignHandoff.afterActionLiaoFailure,sizeof(campaignHandoff.afterActionLiaoFailure))) return false;
    if (!MwStoryCopyText("after_action.failure.general",campaignHandoff.afterActionGeneralFailure,sizeof(campaignHandoff.afterActionGeneralFailure))) return false;
    if (!MwStoryCopyText("after_action.success.kurita",campaignHandoff.afterActionKuritaSuccess,sizeof(campaignHandoff.afterActionKuritaSuccess))) return false;
    if (!MwStoryCopyText("after_action.success.liao",campaignHandoff.afterActionLiaoSuccess,sizeof(campaignHandoff.afterActionLiaoSuccess))) return false;
    if (!MwStoryCopyText("after_action.success.general",campaignHandoff.afterActionGeneralSuccess,sizeof(campaignHandoff.afterActionGeneralSuccess))) return false;
    if (!MwStoryCopyText("settlement.deduction.supply",campaignHandoff.settlementSupplyDeductionText,sizeof(campaignHandoff.settlementSupplyDeductionText))) return false;
    if (!MwStoryCopyText("settlement.deduction.performance",campaignHandoff.settlementPerformanceDeductionText,sizeof(campaignHandoff.settlementPerformanceDeductionText))) return false;
    if (!MwStoryCopyText("settlement.deduction.transport",campaignHandoff.settlementTransportDeductionText,sizeof(campaignHandoff.settlementTransportDeductionText))) return false;
    if (!MwStoryCopyText("settlement.no_payment",campaignHandoff.settlementNoPaymentText,sizeof(campaignHandoff.settlementNoPaymentText))) return false;
    if (!MwStoryCopyText("dark_wing.attack_failed",campaignHandoff.darkWingAttackFailedText,sizeof(campaignHandoff.darkWingAttackFailedText))) return false;
    campaignInput.campaignTimerTicks=0; /*000C*/
    memset(&campaignInput.campaignTimerAlternatingPhase,0,sizeof(campaignInput.campaignTimerAlternatingPhase)); /*000E*/
    memset(&campaignInput.campaignTimerPhaseTicks,0,sizeof(campaignInput.campaignTimerPhaseTicks)); /*000F*/
    memset(&campaignInput.mouseInstalled,0,sizeof(campaignInput.mouseInstalled)); /*02AC*/
    memset(&campaignInput.joystickInstalled,0,sizeof(campaignInput.joystickInstalled)); /*02AD*/
    memset(&campaignInput.inputSinglePoll,0,sizeof(campaignInput.inputSinglePoll)); /*03E2*/
    campaignInput.joystickDirection=0; /*02E6*/
    campaignInput.joystickButtonOnePressed=0; /*02E8*/
    campaignInput.joystickButtonTwoPressed=0; /*02EA*/
    campaignInput.joystickPreviousButtons=MW_JOYSTICK_PREVIOUS_BUTTONS_START;
    campaignInput.joystickXCount=0; /*02ED*/
    campaignInput.joystickYCount=0; /*02EF*/
    campaignInput.joystickDeflection=MW_JOYSTICK_DEFLECTION_START;
    campaignInput.mouseMotionX=0; /*03DA*/
    campaignInput.mouseMotionY=0; /*03DC*/
    memset(&campaignInput.joystickRepeatActive,0,sizeof(campaignInput.joystickRepeatActive)); /*02F3*/
    campaignInput.joystickRepeatDeadline=0; /*02F4*/
    memcpy(campaignInput.joystickNavigationKeys,mwJoystickNavigationKeysStart,sizeof(campaignInput.joystickNavigationKeys)); /*02F6*/
    campaignInput.joystickCentreX=0; /*0308*/
    campaignInput.joystickCentreY=0; /*030A*/
    campaignInput.joystickXDeflectionStep=0; /*031C*/
    campaignInput.joystickYDeflectionStep=0; /*031E*/
    campaignInput.joystickXLow=0; /*030C*/
    campaignInput.joystickXHigh=0; /*030E*/
    campaignInput.joystickYLow=0; /*0310*/
    campaignInput.joystickYHigh=0; /*0312*/
    campaignInput.joystickXFarLow=0; /*0314*/
    campaignInput.joystickXFarHigh=0; /*0316*/
    campaignInput.joystickYFarLow=0; /*0318*/
    campaignInput.joystickYFarHigh=0; /*031A*/
    campaignUi.selectedUiRecordIndex=0; /*03E5*/
    campaignUi.uiSelectionChangedCodeOffset=0; /*03E7*/
    campaignUi.uiIdleCodeOffset=0; /*03E9*/
    /* The hub menus' past-end probes (story/native.json, src/Import/pieces.txt). */
    if (!MwStoryCopyBytes("probe.hub_planet",&campaignUi.hubFirstNavigationProbe,1)
        || !MwStoryCopyBytes("probe.hub_contract",&campaignUi.hubSecondNavigationProbe,1)
        || !MwStoryCopyBytes("probe.hub_remote_planet",&campaignUi.hubThirdNavigationProbe,1)) return false;
    campaignUi.panelOriginX=0; /*03EB*/
    campaignUi.panelOriginY=0; /*03ED*/
    campaignUi.panelWidth=0; /*03EF*/
    campaignUi.panelHeight=0; /*03F1*/
    campaignUi.panelBorderPictureBase=0; /*0444*/
    campaignService.panelBorderEnabled=0; /*03F3*/
    campaignUi.textPanelInsetX=0; /*0432*/
    campaignUi.textPanelInsetY=0; /*0434*/
    campaignUi.textPanelOriginX=0; /*0436*/
    campaignUi.textPanelOriginY=0; /*0438*/
    campaignUi.textPanelWidthPixels=0; /*043A*/
    campaignUi.textPanelHeightPixels=0; /*043C*/
    campaignUi.textPanelPreviousFont=0; /*043E*/
    campaignUi.textPanelFont=0; /*0440*/
    campaignUi.textPanelContinuationMode=0; /*0442*/
    campaignUi.textPanelYOverride=0; /*0446*/
    /* The notification boxes (data/ui/hub.json): all insets x, then y, columns, rows (5EB3). */
    if (!MwUiCopyWords("hub.table.notification_boxes",campaignUi.notificationTextInsetX,4,0)
        || !MwUiCopyWords("hub.table.notification_boxes",campaignUi.notificationTextInsetY,4,8)
        || !MwUiCopyWords("hub.table.notification_boxes",campaignUi.notificationColumns,4,16)
        || !MwUiCopyWords("hub.table.notification_boxes",campaignUi.notificationRows,4,24)) return false;
    /* The notification texts (hub.word.notification.*) and the pointers to them (hub.json's notification_words). */
    if (!MwUiCopyWords("hub.table.notification_words",campaignUi.notificationTextOffsets,4,0)) return false; /*5ED3*/
    memcpy(campaignUi.notificationTextStorage,mainWords+0x5EDB,sizeof(campaignUi.notificationTextStorage));
    campaignCore.compressedPictureBuffer=NULL; /*016D*/
    /* 03F5, 03F7: the random generator's words as an EXEC starts them (data/rules/new_campaign.json "start"). */
    uint16_t randomSeed[2];
    if (!ImportRuleWords("new_campaign.start.random_seed",randomSeed,2)) return false;
    campaignCore.randomStateWord0=randomSeed[0];campaignCore.randomStateWord1=randomSeed[1];
    campaignCore.pictureCompressedByteCount=0; /*0F6D*/
    campaignCore.pictureOutput=NULL; /*0F6F*/
    memset(&campaignCore.pictureReadHighNibbleNext,0,sizeof(campaignCore.pictureReadHighNibbleNext)); /*0F71*/
    campaignCore.pictureReadByteOffset=0; /*0F72*/
    memset(&campaignCore.pictureWriteLowNibbleNext,0,sizeof(campaignCore.pictureWriteLowNibbleNext)); /*0F74*/
    campaignCore.pictureWriteByteOffset=0; /*0F75*/
    campaignCore.indexedPictureOutput=NULL; /*0161*/
    campaignCore.generalPictureBank=NULL; /*0163*/
    campaignCore.cockpitPictureBank=NULL; /*0165*/
    campaignCore.activePictureBank=NULL; /*0167*/
    campaignCore.secondaryPictureBank=NULL; /*0169*/
    memset(&campaignCore.indexedPictureDirectoryLoaded,0,sizeof(campaignCore.indexedPictureDirectoryLoaded)); /*0B62*/
    campaignCore.indexedPictureCachedIndex=MW_INDEXED_PICTURE_NO_CACHED_INDEX;
    campaignCore.indexedPictureStoredBytes=0; /*0B65*/
    memset(campaignCore.indexedPictureDirectory,0,sizeof(campaignCore.indexedPictureDirectory)); /*0B67*/
    /* The planets from story/planets.json, laid out as the exe holds them
     * (offsets at 6570, storage 67B6.., the alphabetical table at 6694). */
    if (!MwStoryBuildPlanets(campaignCore.planetRecordStorage,sizeof(campaignCore.planetRecordStorage),PLANET_RECORDS_START,
        campaignCore.planetRecordOffsets,CAMPAIGN_PLANET_COUNT,campaignMechanics.alphabeticPlanetRecordOffsets,TRAVEL_PLANET_DIRECTORY_COUNT)) return false;
    if (!MwUiCopyWords("hub.table.calendar_month_name_widths",campaignCore.calendarMonthNameWidths,CAMPAIGN_MONTH_COUNT,0)) return false; /*11BA*/
    /* 117E..1183: the calendar's place and colours. Written before it's read; zero where the exe reloaded it. */
    campaignCore.calendarTextX=0;campaignCore.calendarTextY=0;campaignCore.calendarBackground=0;campaignCore.calendarForeground=0;
    memset(&campaignCore.numericLeadingDigitSeen,0,sizeof(campaignCore.numericLeadingDigitSeen)); /*A136*/
    memset(&campaignCore.numericGroupingEnabled,0,sizeof(campaignCore.numericGroupingEnabled)); /*A137*/
    memset(campaignCore.numericDecimalDigits,0,sizeof(campaignCore.numericDecimalDigits)); /*A138*/
    campaignCore.numericRemainderHigh=0; /*A142*/
    campaignCore.numericRemainderLow=0; /*A144*/
    memset(&campaignCore.numericSuppressLeadingZeros,0,sizeof(campaignCore.numericSuppressLeadingZeros)); /*A146*/
    memset(&campaignCore.numericNegativeSign,0,sizeof(campaignCore.numericNegativeSign)); /*A149*/
    campaignCore.numericFieldDigitCount=0; /*A147*/
    campaignCore.numericCursorX=0; /*A14A*/
    campaignCore.numericCursorY=0; /*A14C*/
    memset(&campaignCore.numericBackgroundColour,0,sizeof(campaignCore.numericBackgroundColour)); /*A14E*/
    memset(&campaignCore.numericForegroundColour,0,sizeof(campaignCore.numericForegroundColour)); /*A14F*/
    if (!ImportRuleBytes("houses.negotiation_attempt_base",campaignCore.houseNegotiationAttemptBase,sizeof(campaignCore.houseNegotiationAttemptBase))) return false;
    if (!ImportRuleWords("market.ammunition_prices",(uint16_t *)campaignCore.ammunitionBasePrices,sizeof(campaignCore.ammunitionBasePrices)/sizeof(uint16_t))) return false;
    if (!ImportRuleWords("market.start.ammunition_transaction_prices",(uint16_t *)campaignCore.ammunitionTransactionPrices,sizeof(campaignCore.ammunitionTransactionPrices)/sizeof(uint16_t))) return false;
    /* 3A77: zero in the exe, written in play; not a rule (docs/phase2/PLAN.md 5.1). */
    memset(campaignCore.repairAttemptFailuresByKind,0,sizeof(campaignCore.repairAttemptFailuresByKind));
    memset(&campaignMechanics.generatedMechDamage,0,sizeof(campaignMechanics.generatedMechDamage)); /*3A41*/
    if (!ImportRuleBytes("units.weapons",campaignMechanics.unitWeaponCounts,sizeof(campaignMechanics.unitWeaponCounts))) return false;
    if (!ImportRuleBytes("market.price_tier_multipliers",campaignMechanics.unitPriceTierMultipliers,sizeof(campaignMechanics.unitPriceTierMultipliers))) return false;
    if (!ImportRuleWords("units.base_prices",(uint16_t *)campaignMechanics.unitBasePrices,sizeof(campaignMechanics.unitBasePrices)/sizeof(uint16_t))) return false;
    campaignMechanics.unitTransactionPriceLow=0; /*305B*/
    campaignMechanics.unitTransactionPriceHigh=0; /*305D*/
    campaignMechanics.selectedOwnedUnitKind=0; /*2AD5*/
    campaignMechanics.selectedMarketUnitKind=0; /*4322*/
    campaignMechanics.unitRepairQuoteLow=0; /*31E9*/
    campaignMechanics.unitRepairQuoteHigh=0; /*31EB*/
    campaignMechanics.repairQuoteMechKind=0; /*31ED*/
    campaignMechanics.componentRepairPriceLow=0; /*320C*/
    campaignMechanics.componentRepairPriceHigh=0; /*320E*/
    campaignMechanics.repairPriceCategory=0; /*3210*/
    if (!ImportRuleWords("units.weapon_repair_prices",(uint16_t *)campaignMechanics.weaponRepairPrices,sizeof(campaignMechanics.weaponRepairPrices)/sizeof(uint16_t))) return false;
    if (!ImportRuleBytes("units.body_repair_prices",campaignMechanics.bodyRepairBasePricesByKind,sizeof(campaignMechanics.bodyRepairBasePricesByKind))) return false;
    /* The pilots from story/pilots.json (offsets at 9121, storage 9177..). */
    if (!MwStoryBuildPilots(campaignMechanics.pilotDefinitionStorage,sizeof(campaignMechanics.pilotDefinitionStorage),PILOT_DEFINITIONS_START,
        campaignMechanics.pilotDefinitionOffsets,CAMPAIGN_PILOT_ID_COUNT)) return false;
    MwBuildTravelSquareRoots(campaignMechanics.travelSquareRootValues,TRAVEL_ROOT_TABLE_COUNT); /*4CE6*/
    campaignMechanics.travelDestinationMapPositionX=0; /*54D0*/
    campaignMechanics.travelDestinationMapPositionY=0; /*54D2*/
    campaignMechanics.travelMapDeltaX=0; /*54D4*/
    campaignMechanics.travelMapDeltaY=0; /*54D6*/
    campaignMechanics.travelDestinationTimeWeight=0; /* Written before it's read; zero where the exe reloaded it. */
    campaignMechanics.travelDestinationPlanetOffset=0; /*5AD3*/
    campaignMechanics.travelDistanceUnits=0; /*5AD9*/
    campaignMechanics.travelQuoteCbillsLow=0; /*5AD5*/
    campaignMechanics.travelQuoteCbillsHigh=0; /*5AD7*/
    campaignMechanics.travelMapSelection=0; /*5ACF*/
    campaignMechanics.travelDestinationPlanetIndex=0; /*5AD1*/
    campaignMechanics.travelMarkerX=0; /*5ADB*/
    campaignMechanics.travelMarkerY=0; /*5ADD*/
    campaignMechanics.travelSavedMarkerX=0; /*5ADF*/
    campaignMechanics.travelSavedMarkerY=0; /*5AE1*/
    campaignMechanics.travelJoystickLastTick=0; /*5AE3*/
    campaignMechanics.travelAnimationFrameStep=0; /*5AE6*/
    memset(&campaignMechanics.travelDropShipSelected,0,sizeof(campaignMechanics.travelDropShipSelected)); /*5AE5*/
    campaignMechanics.hiredTransportReverseTerminator=MW_HIRED_TRANSPORT_REVERSE_END;
    if (!MwUiCopyWords("travel.table.hired_transport_frames",(uint16_t *)campaignMechanics.hiredTransportFrames,sizeof(campaignMechanics.hiredTransportFrames)/sizeof(uint16_t),0) /*5AEA*/
        || !MwUiCopyWords("travel.table.dropship_frames",(uint16_t *)campaignMechanics.dropShipTravelFrames,sizeof(campaignMechanics.dropShipTravelFrames)/sizeof(uint16_t),0)) return false; /*5B2E*/
    /* DS:5C4E: the destination header's caption text (the original's buffer runs past it into the next caption's head, bytes after the NUL that nothing draws). */
    if (!MwWordsCopyText("travel.caption.destination_name.text",campaignMechanics.travelDestinationName,sizeof(campaignMechanics.travelDestinationName))) return false;
    campaignMechanics.travelSelectedHouseByteOffset=0; /*54DC*/
    campaignMechanics.travelHousePlanetStart=0; /*54DE*/
    campaignMechanics.travelHousePlanetCount=0; /*54E2*/
    campaignTravel.travelSelectedHouseRow=0; /*5A0B*/
    memcpy(campaignTravel.travelPopulationSuffix,mainWords+0x5B83,sizeof(campaignTravel.travelPopulationSuffix));
    memcpy(campaignTravel.travelLandscapeLabels,mainWords+0x5CA6,sizeof(campaignTravel.travelLandscapeLabels));
    memcpy(campaignTravel.travelInsufficientFundsText,mainWords+0x5B5A,sizeof(campaignTravel.travelInsufficientFundsText));
    if (!ImportRuleWords("market.freight_rates",(uint16_t *)campaignMechanics.reserveAmmoFreightRates,sizeof(campaignMechanics.reserveAmmoFreightRates)/sizeof(uint16_t))) return false;
    if (!ImportRuleBytes("market.stock_kind_range_maxima",campaignEconomy.generatedKindRangeMaxima,sizeof(campaignEconomy.generatedKindRangeMaxima))) return false;
    if (!ImportRuleBytes("market.stock_kind_range_starts",campaignEconomy.generatedKindRangeStarts,sizeof(campaignEconomy.generatedKindRangeStarts))) return false;
    campaignEconomy.generatedDamageMaximumIndex=0; /*3A3D*/
    campaignEconomy.generatedUnitKind=0; /*3A3F*/
    if (!ImportRuleWords("market.start.generated_ammo_kinds",(uint16_t *)campaignEconomy.generatedAmmoKinds,sizeof(campaignEconomy.generatedAmmoKinds)/sizeof(uint16_t))) return false;
    if (!ImportRuleWords("market.start.generated_ammo_quantities",(uint16_t *)campaignEconomy.generatedAmmoQuantities,sizeof(campaignEconomy.generatedAmmoQuantities)/sizeof(uint16_t))) return false;
    if (!ImportRuleBytes("market.start.generate_starting_jenner",&campaignEconomy.generateStartingJenner,sizeof(campaignEconomy.generateStartingJenner))) return false;
    if (!ImportRuleBytes("units.ammo_groups",campaignEconomy.unitAmmoGroupCounts,sizeof(campaignEconomy.unitAmmoGroupCounts))) return false;
    if (!ImportRuleBytes("units.jump_jet_groups",campaignEconomy.unitJumpJetGroupCounts,sizeof(campaignEconomy.unitJumpJetGroupCounts))) return false;
    campaignEconomy.generatingMissionOfferSlot=0; /*4B77*/
    campaignEconomy.missionForceBudget=0; /* Written before it's read; zero where the exe reloaded it. */
    campaignEconomy.missionPaymentBaseThousands=0; /* Written before it's read; zero where the exe reloaded it. */
    if (!ImportRuleBytes("units.force_weights",campaignEconomy.missionForceWeightByUnitKind,sizeof(campaignEconomy.missionForceWeightByUnitKind))) return false;
    if (!ImportRuleWords("houses.payment_roll_bias",(uint16_t *)campaignEconomy.housePaymentRollBias,sizeof(campaignEconomy.housePaymentRollBias)/sizeof(uint16_t))) return false;
    if (!ImportRuleWords("houses.payment_adjustment_percent",(uint16_t *)campaignEconomy.housePaymentAdjustmentPercent,sizeof(campaignEconomy.housePaymentAdjustmentPercent)/sizeof(uint16_t))) return false;
    if (!ImportRuleWords("houses.salvage_base_percent",(uint16_t *)campaignEconomy.houseSalvageBasePercent,sizeof(campaignEconomy.houseSalvageBasePercent)/sizeof(uint16_t))) return false;
    if (!ImportRuleWords("houses.advance_base_percent",(uint16_t *)campaignEconomy.houseAdvanceBasePercent,sizeof(campaignEconomy.houseAdvanceBasePercent)/sizeof(uint16_t))) return false;
    if (!ImportRuleWords("houses.planet_range_starts",(uint16_t *)campaignMechanics.housePlanetRangeStarts,sizeof(campaignMechanics.housePlanetRangeStarts)/sizeof(uint16_t))) return false;
    if (!ImportRuleWords("houses.planet_range_counts",(uint16_t *)campaignMechanics.housePlanetRangeCounts,sizeof(campaignMechanics.housePlanetRangeCounts)/sizeof(uint16_t))) return false;
    /* The mission descriptions from story/missions.json: the category tables
     * (8A16 maximum indices, 8A1B list offsets, 8A25 entries) and the storage
     * (8A69..). Saves keep entry offsets, so the original's text gives the
     * original's offsets. */
    if (!MwStoryBuildMissions(campaignEconomy.missionDescriptionStorage,sizeof(campaignEconomy.missionDescriptionStorage),MISSION_DESCRIPTION_STORAGE_START,
        campaignEconomy.missionDescriptionMaximumIndicesByCategory,campaignEconomy.missionDescriptionListOffsetsByCategory,MISSION_CATEGORY_COUNT,
        campaignEconomy.missionDescriptionListEntries,MISSION_DESCRIPTION_LIST_WORDS,MISSION_DESCRIPTION_LIST_START)) return false;
    if (!ImportRuleWords("houses.mission_planet_policy_offsets",(uint16_t *)campaignEconomy.houseMissionPlanetTableOffsets,sizeof(campaignEconomy.houseMissionPlanetTableOffsets)/sizeof(uint16_t))) return false;
    if (!ImportRuleBytes("houses.mission_planet_policies",campaignEconomy.houseMissionPlanetPolicies,sizeof(campaignEconomy.houseMissionPlanetPolicies))) return false;
    if (!ImportRuleWords("contracts.pilot_wages",(uint16_t *)campaignEconomy.pilotWageAmounts,sizeof(campaignEconomy.pilotWageAmounts)/sizeof(uint16_t))) return false;
    memset(campaignEconomy.notificationPending,0,sizeof(campaignEconomy.notificationPending)); /*5EAE*/
    campaignService.unitInspectionContextCode=0; /*25BD*/
    campaignService.ownedUnitListSelectedRow=0; /*25BF*/
    campaignService.marketUnitListSelectedRow=0; /*25C1*/
    campaignService.mechServiceMenuSelectedRow=0; /*25C3*/
    campaignService.selectedOwnedUnitSlot=0; /*2AD1*/
    campaignService.selectedOwnedUnitByteOffset=0; /*2AD3*/
    memset(campaignService.selectedMechAmmoFamilies,0,sizeof(campaignService.selectedMechAmmoFamilies)); /*2AD7*/
    memset(campaignService.selectedMechAmmoQuantities,0,sizeof(campaignService.selectedMechAmmoQuantities)); /*2ADD*/
    if (!ImportRuleBytes("units.condition_fields",campaignService.unitConditionSystemFieldCounts,sizeof(campaignService.unitConditionSystemFieldCounts))) return false;
    if (!ImportRuleBytes("units.condition_score_divisors",campaignService.unitConditionScoreDivisors,sizeof(campaignService.unitConditionScoreDivisors))) return false;
    memcpy(campaignService.mechNonfunctionalTextBytes,mainWords+0x2B55,sizeof(campaignService.mechNonfunctionalTextBytes));
    if (!ImportRuleBytes("units.display_tonnages",campaignService.unitDisplayTonnages,sizeof(campaignService.unitDisplayTonnages))) return false;
    if (!ImportRuleBytes("units.display_speeds",campaignService.unitDisplaySpeedsKph,sizeof(campaignService.unitDisplaySpeedsKph))) return false;
    if (!ImportRuleBytes("units.display_jumps",campaignService.unitDisplayJumpCapacitiesMetres,sizeof(campaignService.unitDisplayJumpCapacitiesMetres))) return false;
    if (!MwUiCopyWords("complex.table.detail_picture_x_offsets",(uint16_t *)campaignService.unitDetailPictureXOffsets,sizeof(campaignService.unitDetailPictureXOffsets)/sizeof(uint16_t),0) /*2B83*/
        || !MwUiCopyWords("complex.table.detail_picture_y_origins",(uint16_t *)campaignService.unitDetailPictureYOrigins,sizeof(campaignService.unitDetailPictureYOrigins)/sizeof(uint16_t),0)) return false; /*2B99*/
    memset(&campaignService.repairWorkIncomplete,0,sizeof(campaignService.repairWorkIncomplete)); /*2BAF*/
    campaignService.ownedListRowUnitByteOffset=0; /*3022*/
    campaignService.saleMarketSlotByteOffset=0; /*3055*/
    campaignService.selectedMechRearmQuoteLow=0; /*310C*/
    campaignService.selectedMechRearmQuoteHigh=0; /*310E*/
    campaignService.mechServiceNextAnimationTick=0; /*3189*/
    campaignService.mechServiceAnimationByteOffset=0; /*318B*/
    if (!MwUiCopyWords("complex.table.repair_title_leading_spaces",(uint16_t *)campaignService.repairTitleLeadingSpacesByKind,sizeof(campaignService.repairTitleLeadingSpacesByKind)/sizeof(uint16_t),0)) return false; /*31F2*/
    campaignService.repairSystemRowIndex=0; /*3208*/
    campaignService.repairMenuSelectedCategory=0; /*320A*/
    if (!ImportRuleBytes("units.heat_sinks",campaignService.unitHeatSinkCounts,sizeof(campaignService.unitHeatSinkCounts))) return false;
    memcpy(campaignService.repairNoJumpJetsTextBytes,mainWords+0x36A0,sizeof(campaignService.repairNoJumpJetsTextBytes));
    memcpy(campaignService.repairJumpJetCountTemplateBytes,mainWords+0x36A5,sizeof(campaignService.repairJumpJetCountTemplateBytes));
    if (!ImportRuleBytes("repair.success_roll_limits",campaignService.repairSuccessRollLimitsByMarketTier,sizeof(campaignService.repairSuccessRollLimitsByMarketTier))) return false;
    campaignService.mechDiagramUnitKind=0; /*3CCC*/
    if (!MwUiCopyWords("complex.table.body_damage_fill_colours",(uint16_t *)campaignService.bodyDamageTierFillColours,sizeof(campaignService.bodyDamageTierFillColours)/sizeof(uint16_t),0)) return false; /*3CD2*/
    memcpy(campaignService.unassignedMechWarriorText,mainWords+0x3024,sizeof(campaignService.unassignedMechWarriorText));
    memcpy(campaignService.ownedListDoneText,mainWords+0x302F,sizeof(campaignService.ownedListDoneText));
    memcpy(campaignService.cannotAffordRepairText,mainWords+0x2BB0,sizeof(campaignService.cannotAffordRepairText));
    memcpy(campaignService.noRepairWorkText,mainWords+0x2BE7,sizeof(campaignService.noRepairWorkText));
    memcpy(campaignService.repairCompleteText,mainWords+0x2C01,sizeof(campaignService.repairCompleteText));
    memcpy(campaignService.repairIncompleteText,mainWords+0x2C21,sizeof(campaignService.repairIncompleteText));
    memcpy(campaignService.repairFailedText,mainWords+0x3CB5,sizeof(campaignService.repairFailedText));
    campaignUi.hubSelectedRecordIndex=0; /*1080*/
    memcpy(campaignMarket.ammoBayFullText,mainWords+0x315B,sizeof(campaignMarket.ammoBayFullText));
    memcpy(campaignMarket.noMarketMechsText,mainWords+0x31BB,sizeof(campaignMarket.noMarketMechsText));
    memcpy(campaignMarket.dealerPurchaseLimitText,mainWords+0x258A,sizeof(campaignMarket.dealerPurchaseLimitText));
    memcpy(campaignMarket.purchaseInsufficientFundsText,mainWords+0x4330,sizeof(campaignMarket.purchaseInsufficientFundsText));
    memcpy(campaignMarket.ownedMechCapacityText,mainWords+0x4347,sizeof(campaignMarket.ownedMechCapacityText));
    memcpy(campaignMarket.marketListDoneText,mainWords+0x3049,sizeof(campaignMarket.marketListDoneText));
    campaignMarket.selectedMarketUnitSlot=0; /*431E*/
    campaignMarket.selectedMarketUnitByteOffset=0; /*4320*/
    memset(campaignMarket.selectedMarketAmmoFamilies,0,sizeof(campaignMarket.selectedMarketAmmoFamilies)); /*4324*/
    memset(campaignMarket.selectedMarketAmmoQuantities,0,sizeof(campaignMarket.selectedMarketAmmoQuantities)); /*432A*/
    campaignMarket.marketSystemRowIndex=0; /*44EF*/
    campaignMarket.purchaseOwnedSlotByteOffset=0; /*481D*/
    if (!MwUiCopyWords("complex.table.animation_frames",(uint16_t *)campaignMarket.mechComplexAnimationFrames,sizeof(campaignMarket.mechComplexAnimationFrames)/sizeof(uint16_t),0)) return false; /*318D*/
    campaignStartup.campaignMusicBuffer=NULL; /*016F*/
    campaignStartup.graphicsOverlay=NULL; /*002E*/
    memset(&campaignStartup.launcherExitCode,0,sizeof(campaignStartup.launcherExitCode)); /*0006*/
    memset(&campaignStartup.keyboardInterruptState,0,sizeof(campaignStartup.keyboardInterruptState)); /*02AB*/
    memset(campaignStartup.campaignTextInputBuffer,0,sizeof(campaignStartup.campaignTextInputBuffer)); /*03A5*/
    campaignInput.textInputCursorX=0; /*03CE*/
    campaignInput.textInputCursorY=0; /*03D0*/
    campaignInput.textInputMaximumIndex=0; /*03D2*/
    memset(&campaignInput.textInputForeground,0,sizeof(campaignInput.textInputForeground)); /*03D4*/
    memset(&campaignInput.textInputBackground,0,sizeof(campaignInput.textInputBackground)); /*03D5*/
    memset(&campaignInput.textInputLastPhase,0,sizeof(campaignInput.textInputLastPhase)); /*03D8*/
    campaignInput.textInputLength=0; /*03D6*/
    {
        /* The story-planet pools from story/events.json (1090:8FDA..8FF3). */
        uint8_t *const pools[6]={campaignStartup.eventPlanetChoicePool0,campaignStartup.eventPlanetChoicePool1,campaignStartup.eventPlanetChoicePool2,
            campaignStartup.eventPlanetChoicePool3,campaignStartup.eventPlanetChoicePool4,campaignStartup.eventPlanetChoicePool5};
        const size_t sizes[6]={sizeof(campaignStartup.eventPlanetChoicePool0),sizeof(campaignStartup.eventPlanetChoicePool1),sizeof(campaignStartup.eventPlanetChoicePool2),
            sizeof(campaignStartup.eventPlanetChoicePool3),sizeof(campaignStartup.eventPlanetChoicePool4),sizeof(campaignStartup.eventPlanetChoicePool5)};
        if (!MwStoryCopyPlanetPools(pools,sizes,6)) return false;
    }
    campaignEvents.campaignEventClass=0; /*5E62*/
    campaignEvents.currentEventId=0; /*5E64*/
    campaignEvents.dispatchedEventCount=0; /*5E68*/
    memset(&campaignEvents.eventConditionResult,0,sizeof(campaignEvents.eventConditionResult)); /*5E70*/
    memset(&campaignEvents.eventCombineWithAnd,0,sizeof(campaignEvents.eventCombineWithAnd)); /*5E71*/
    campaignEvents.eventScheduledMonthScratch=0; /*5E72*/
    campaignEvents.eventExpirySubmonthScratch=0; /*5E74*/
    campaignEvents.eventScheduledYearScratch=0; /*5E76*/
    memset(&campaignEvents.planetArrivalEventsPending,0,sizeof(campaignEvents.planetArrivalEventsPending)); /*1082*/
    memset(&campaignEvents.campaignRestartPending,0,sizeof(campaignEvents.campaignRestartPending)); /*5EB2*/
    memset(&campaignEvents.eventChoiceCount,0,sizeof(campaignEvents.eventChoiceCount)); /*5CEC*/
    memset(&campaignEvents.eventTextRowCount,0,sizeof(campaignEvents.eventTextRowCount)); /*5CED*/
    campaignEvents.eventChoiceSavedFontIndex=0; /*5E66*/
    if (!ImportRuleBytes("houses.mission_type_limits",campaignMission.defaultHouseMissionTypeLimits,sizeof(campaignMission.defaultHouseMissionTypeLimits))) return false;
    if (!MwStoryCopyText("commander.default_name",(char *)campaignMission.campaignDefaultCommanderNameBytes,sizeof(campaignMission.campaignDefaultCommanderNameBytes))) return false;
    campaignUi.companyMenuSelectedRow=0; /*21CC*/
    campaignUi.barMenuSelectedRow=0; /*8FF4*/
    campaignUi.optionsMenuSelection=0; /*9D50*/
    for (unsigned line=0;line<10;++line)
    {
        char id[24];snprintf(id,sizeof(id),"intro.narration.%u",line+1);
        if (!MwStoryCopyText(id,campaignStartup.introNarrationLines[line],sizeof(campaignStartup.introNarrationLines[line]))) return false;
    }
    if (!ImportIntroCaptionAt("intro.caption.1.left.at",&campaignStartup.introFirstLeftText)) return false;
    if (!MwStoryCopyText("intro.caption.1.left",campaignStartup.introFirstLeftText.text,sizeof(campaignStartup.introFirstLeftText.text))) return false;
    if (!ImportIntroCaptionAt("intro.caption.1.right.at",&campaignStartup.introFirstRightText)) return false;
    if (!MwStoryCopyText("intro.caption.1.right",campaignStartup.introFirstRightText.text,sizeof(campaignStartup.introFirstRightText.text))) return false;
    if (!ImportIntroCaptionAt("intro.caption.2.left.at",&campaignStartup.introSecondLeftText)) return false;
    if (!MwStoryCopyText("intro.caption.2.left",campaignStartup.introSecondLeftText.text,sizeof(campaignStartup.introSecondLeftText.text))) return false;
    if (!ImportIntroCaptionAt("intro.caption.2.right.at",&campaignStartup.introSecondRightText)) return false;
    if (!MwStoryCopyText("intro.caption.2.right",campaignStartup.introSecondRightText.text,sizeof(campaignStartup.introSecondRightText.text))) return false;
    if (!ImportIntroCaptionAt("intro.caption.3.left.at",&campaignStartup.introThirdLeftText)) return false;
    if (!MwStoryCopyText("intro.caption.3.left",campaignStartup.introThirdLeftText.text,sizeof(campaignStartup.introThirdLeftText.text))) return false;
    if (!ImportIntroCaptionAt("intro.caption.3.right.at",&campaignStartup.introThirdRightText)) return false;
    if (!MwStoryCopyText("intro.caption.3.right",campaignStartup.introThirdRightText.text,sizeof(campaignStartup.introThirdRightText.text))) return false;
    if (!ImportIntroCaptionAt("intro.caption.4.left.at",&campaignStartup.introFourthLeftText)) return false;
    if (!MwStoryCopyText("intro.caption.4.left",campaignStartup.introFourthLeftText.text,sizeof(campaignStartup.introFourthLeftText.text))) return false;
    if (!ImportIntroCaptionAt("intro.caption.4.right.at",&campaignStartup.introFourthRightText)) return false;
    if (!MwStoryCopyText("intro.caption.4.right",campaignStartup.introFourthRightText.text,sizeof(campaignStartup.introFourthRightText.text))) return false;
    if (!ImportIntroCaptionAt("intro.caption.5.left.at",&campaignStartup.introFifthLeftText)) return false;
    if (!MwStoryCopyText("intro.caption.5.left",campaignStartup.introFifthLeftText.text,sizeof(campaignStartup.introFifthLeftText.text))) return false;
    if (!ImportIntroCaptionAt("intro.caption.5.right.at",&campaignStartup.introFifthRightText)) return false;
    if (!MwStoryCopyText("intro.caption.5.right",campaignStartup.introFifthRightText.text,sizeof(campaignStartup.introFifthRightText.text))) return false;
    if (!ImportIntroCaptionAt("intro.caption.6.left.at",&campaignStartup.introSixthLeftText)) return false;
    if (!MwStoryCopyText("intro.caption.6.left",campaignStartup.introSixthLeftText.text,sizeof(campaignStartup.introSixthLeftText.text))) return false;
    if (!ImportIntroCaptionAt("intro.caption.6.right.at",&campaignStartup.introSixthRightText)) return false;
    if (!MwStoryCopyText("intro.caption.6.right",campaignStartup.introSixthRightText.text,sizeof(campaignStartup.introSixthRightText.text))) return false;
    if (!ImportIntroCaptionAt("intro.caption.7.left.at",&campaignStartup.introSeventhLeftText)) return false;
    if (!MwStoryCopyText("intro.caption.7.left",campaignStartup.introSeventhLeftText.text,sizeof(campaignStartup.introSeventhLeftText.text))) return false;
    if (!ImportIntroCaptionAt("intro.caption.7.right.at",&campaignStartup.introSeventhRightText)) return false;
    if (!MwStoryCopyText("intro.caption.7.right",campaignStartup.introSeventhRightText.text,sizeof(campaignStartup.introSeventhRightText.text))) return false;
    for (uint16_t row=0;row<CAMPAIGN_SYSTEM_COUNT;++row)
        if (!ImportRuleWords("repair.system_prices",campaignMechanics.systemRepairBasePrices[row].basePriceByMechKind,CAMPAIGN_MECH_KIND_COUNT,row*0x14)) return false;
    /* 2F3C, 2F28 and 3C1B come from the rules (data/rules/units.json). */
    uint8_t ammoProfileEntries[AMMO_PROFILE_ENTRY_COUNT*4],ammoProfiles[CAMPAIGN_MECH_KIND_COUNT*2];
    uint8_t weaponRepairPriceLists[CAMPAIGN_MECH_KIND_COUNT*2];
    if (!ImportRuleBytes("units.ammo_profile_entries",ammoProfileEntries,sizeof(ammoProfileEntries))
        || !ImportRuleBytes("units.ammo_profiles",ammoProfiles,sizeof(ammoProfiles))
        || !ImportRuleBytes("units.weapon_repair_price_lists",weaponRepairPriceLists,sizeof(weaponRepairPriceLists))) return false;
    for (uint16_t entry=0;entry<AMMO_PROFILE_ENTRY_COUNT;++entry)
    {
        campaignEconomy.unitAmmoProfileEntries[entry].ammoFamily=MwReadWord(ammoProfileEntries+entry*4);
        campaignEconomy.unitAmmoProfileEntries[entry].quantityMaximum=MwReadWord(ammoProfileEntries+entry*4+2);
    }
    /* complex.table.body_diagram_by_kind (3CDA): each kind's diagram at its
     * exe address in the body_diagrams table (3CEE), or 0 for none. */
    uint16_t diagramByKind[CAMPAIGN_MECH_KIND_COUNT],repairWeaponWords[CAMPAIGN_MECH_KIND_COUNT];
    /* Each kind's repair weapon labels (complex.word.repair_weapons.*), by complex.json's repair_weapon_words (36AE). */
    if (!MwUiCopyWords("complex.table.repair_weapon_words",repairWeaponWords,CAMPAIGN_MECH_KIND_COUNT,0)) return false;
    const uint16_t diagramBase=MwUiPieceOffset("complex.table.body_diagrams");
    if (!diagramBase || !MwUiCopyWords("complex.table.body_diagram_by_kind",diagramByKind,CAMPAIGN_MECH_KIND_COUNT,0)) return false;
    for (uint16_t kind=0;kind<CAMPAIGN_MECH_KIND_COUNT;++kind)
    {
        uint16_t profile=MwReadWord(ammoProfiles+kind*2);
        uint16_t prices=MwReadWord(weaponRepairPriceLists+kind*2);
        uint16_t layout=diagramByKind[kind];
        if (profile<0x2F3C || profile>=0x2F78 || (profile-0x2F3C)%4
            || prices<0x3C2F || prices>=0x3CAB || (prices-0x3C2F)%2
            || (layout && (layout<diagramBase || layout>=diagramBase+8*0x50 || (layout-diagramBase)%0x50))) return false;
        campaignEconomy.unitAmmoProfiles[kind]=&campaignEconomy.unitAmmoProfileEntries[(profile-0x2F3C)/4];
        campaignMechanics.weaponRepairPricesByKind[kind]=&campaignMechanics.weaponRepairPrices[(prices-0x3C2F)/2];
        campaignService.mechBodyDiagramLayoutsByKind[kind]=layout?&campaignService.mechBodyDiagramLayouts[(layout-diagramBase)/0x50]:NULL;
        uint16_t labelOffset=repairWeaponWords[kind],label=0;
        while (mainWords[labelOffset]!=0xFF && label<CAMPAIGN_WEAPON_SLOTS+1)
        {
            if (!ImportInitialString(mainWords,labelOffset,campaignService.repairWeaponLabelStorage[kind][label],0x0F)) return false;
            campaignService.repairWeaponLabels[kind][label]=campaignService.repairWeaponLabelStorage[kind][label];
            labelOffset+=(uint16_t)(strlen(campaignService.repairWeaponLabels[kind][label])+1);++label;
        }
        campaignService.repairWeaponLabels[kind][label]=NULL;
    }
    for (uint16_t i=0;i<CAMPAIGN_UNIT_KIND_COUNT;++i)
    {
        char id[40];
        snprintf(id,sizeof(id),"unit.%s.shop_name",storyUnitKeys[i]);
        if (!MwStoryCopyText(id,campaignService.shopUnitNames[i],sizeof(campaignService.shopUnitNames[i]))) return false;
        snprintf(id,sizeof(id),"unit.%s.name",storyUnitKeys[i]);
        if (!MwStoryCopyText(id,campaignService.unitDisplayNames[i],sizeof(campaignService.unitDisplayNames[i]))) return false;
    }
    if (!ImportReserveAmmoUiData()) return false;
    /* The ammo family names and damage tier labels (complex.word.*), by complex.json's ammo_family_words (2F78) and damage_tier_words (2B23). */
    uint16_t ammoFamilyWords[0x06],damageTierWords[0x04];
    if (!MwUiCopyWords("complex.table.ammo_family_words",ammoFamilyWords,0x06,0)
        || !MwUiCopyWords("complex.table.damage_tier_words",damageTierWords,0x04,0)) return false;
    for (uint16_t i=0;i<0x06;++i)
        if (!ImportInitialString(mainWords,ammoFamilyWords[i],campaignService.ammoFamilyDisplayNames[i],sizeof(campaignService.ammoFamilyDisplayNames[i]))) return false;
    for (uint16_t i=0;i<0x04;++i)
        if (!ImportInitialString(mainWords,damageTierWords[i],campaignService.campaignDamageTierLabels[i],sizeof(campaignService.campaignDamageTierLabels[i]))) return false;
    /* The month names (hub.word.month.*) and the pointers to them (hub.json's month_words, 4A94). */
    memcpy(calendarMonthText,mainWords+0x4AAC,sizeof(calendarMonthText));
    uint16_t monthWords[CAMPAIGN_MONTH_COUNT];
    if (!MwUiCopyWords("hub.table.month_words",monthWords,CAMPAIGN_MONTH_COUNT,0)) return false;
    for (uint16_t month=0;month<CAMPAIGN_MONTH_COUNT;++month)
    {
        uint16_t text=monthWords[month];
        if (text<0x4AAC || text>=0x4B1A || !memchr(mainWords+text,0,0x4B1A-text)) return false;
        campaignCore.calendarMonthNames[month].cursor=calendarMonthText+(text-0x4AAC);
        campaignCore.calendarMonthNames[month].end=calendarMonthText+sizeof(calendarMonthText);
    }
    /* The eighth planet row's caption (travel.caption.planet_8, 55EC): its head from the layout bytes, its 18-byte text from the words. */
    uint16_t positionedAt[2];
    if (!MwUiCopyWords("travel.caption.planet_8",positionedAt,2,0)
        || !MwUiCopyBytes("travel.caption.planet_8",&campaignMarket.positionedText.display.colour,1,4)) return false;
    campaignMarket.positionedText.display.x=positionedAt[0];campaignMarket.positionedText.display.y=positionedAt[1];
    campaignMarket.positionedText.display.text=campaignMarket.positionedText.text;
    if (!MwWordsCopyBlock("travel.caption.planet_8.text",(uint8_t *)campaignMarket.positionedText.text,18)) return false;
    /* data/ui/complex.json: the Mech Complex (panel 25C5, menu 2632), the owned
     * list (266A, 283B), 'Mech and DropShip service (2C3F, 2D53; 2D06, 2D99),
     * sale (3075, 30F0), rearm (3110, 316D) and component repair (3214, 3558).
     * Rows the code fills in keep their fixed-width templates. */
    UiCaptionOwner complexOwners[]={
        {"complex.caption.complex_title",&campaignMarket.mechComplexLabels[0].display,campaignMarket.mechComplexLabels[0].text,sizeof(campaignMarket.mechComplexLabels[0].text),0,0},
        {"complex.caption.complex_owned",&campaignMarket.mechComplexLabels[1].display,campaignMarket.mechComplexLabels[1].text,sizeof(campaignMarket.mechComplexLabels[1].text),0,0},
        {"complex.caption.complex_ammo",&campaignMarket.mechComplexLabels[2].display,campaignMarket.mechComplexLabels[2].text,sizeof(campaignMarket.mechComplexLabels[2].text),0,0},
        {"complex.caption.complex_market",&campaignMarket.mechComplexLabels[3].display,campaignMarket.mechComplexLabels[3].text,sizeof(campaignMarket.mechComplexLabels[3].text),0,0},
        {"complex.caption.complex_leave",&campaignMarket.mechComplexLabels[4].display,campaignMarket.mechComplexLabels[4].text,sizeof(campaignMarket.mechComplexLabels[4].text),0,0},
        {"complex.caption.owned_heading",&campaignService.ownedUnitListHeading.display,campaignService.ownedUnitListHeading.text,sizeof(campaignService.ownedUnitListHeading.text),0,26},
        {"complex.caption.owned_1",&campaignService.ownedUnitListRows[0].display,campaignService.ownedUnitListRows[0].caption.text,sizeof(campaignService.ownedUnitListRows[0].caption.text),0,25},
        {"complex.caption.owned_2",&campaignService.ownedUnitListRows[1].display,campaignService.ownedUnitListRows[1].caption.text,sizeof(campaignService.ownedUnitListRows[1].caption.text),0,25},
        {"complex.caption.owned_3",&campaignService.ownedUnitListRows[2].display,campaignService.ownedUnitListRows[2].caption.text,sizeof(campaignService.ownedUnitListRows[2].caption.text),0,25},
        {"complex.caption.owned_4",&campaignService.ownedUnitListRows[3].display,campaignService.ownedUnitListRows[3].caption.text,sizeof(campaignService.ownedUnitListRows[3].caption.text),0,25},
        {"complex.caption.owned_5",&campaignService.ownedUnitListRows[4].display,campaignService.ownedUnitListRows[4].caption.text,sizeof(campaignService.ownedUnitListRows[4].caption.text),0,25},
        {"complex.caption.owned_6",&campaignService.ownedUnitListRows[5].display,campaignService.ownedUnitListRows[5].caption.text,sizeof(campaignService.ownedUnitListRows[5].caption.text),0,25},
        {"complex.caption.owned_7",&campaignService.ownedUnitListRows[6].display,campaignService.ownedUnitListRows[6].caption.text,sizeof(campaignService.ownedUnitListRows[6].caption.text),0,25},
        {"complex.caption.owned_8",&campaignService.ownedUnitListRows[7].display,campaignService.ownedUnitListRows[7].caption.text,sizeof(campaignService.ownedUnitListRows[7].caption.text),0,25},
        {"complex.caption.owned_9",&campaignService.ownedUnitListRows[8].display,campaignService.ownedUnitListRows[8].caption.text,sizeof(campaignService.ownedUnitListRows[8].caption.text),0,25},
        {"complex.caption.owned_10",&campaignService.ownedUnitListRows[9].display,campaignService.ownedUnitListRows[9].caption.text,sizeof(campaignService.ownedUnitListRows[9].caption.text),0,25},
        {"complex.caption.owned_11",&campaignService.ownedUnitListRows[10].display,campaignService.ownedUnitListRows[10].caption.text,sizeof(campaignService.ownedUnitListRows[10].caption.text),0,25},
        {"complex.caption.owned_12",&campaignService.ownedUnitListRows[11].display,campaignService.ownedUnitListRows[11].caption.text,sizeof(campaignService.ownedUnitListRows[11].caption.text),0,25},
        {"complex.caption.owned_13",&campaignService.ownedUnitListRows[12].display,campaignService.ownedUnitListRows[12].caption.text,sizeof(campaignService.ownedUnitListRows[12].caption.text),0,25},
        {"complex.caption.mech_status",&campaignService.mechStatusLabels[0].display,campaignService.mechStatusLabels[0].text,sizeof(campaignService.mechStatusLabels[0].text),0,106},
        {"complex.caption.service_repair",&campaignService.mechStatusLabels[1].display,campaignService.mechStatusLabels[1].text,sizeof(campaignService.mechStatusLabels[1].text),0,7},
        {"complex.caption.service_repair_all",&campaignService.mechStatusLabels[2].display,campaignService.mechStatusLabels[2].text,sizeof(campaignService.mechStatusLabels[2].text),0,11},
        {"complex.caption.service_rearm",&campaignService.mechStatusLabels[3].display,campaignService.mechStatusLabels[3].text,sizeof(campaignService.mechStatusLabels[3].text),0,7},
        {"complex.caption.service_sell",&campaignService.mechStatusLabels[4].display,campaignService.mechStatusLabels[4].text,sizeof(campaignService.mechStatusLabels[4].text),0,5},
        {"complex.caption.service_leave",&campaignService.mechStatusLabels[5].display,campaignService.mechStatusLabels[5].text,sizeof(campaignService.mechStatusLabels[5].text),0,5},
        {"complex.caption.dropship_status",&campaignService.dropShipStatusLabels[0].display,campaignService.dropShipStatusLabels[0].text,sizeof(campaignService.dropShipStatusLabels[0].text),0,30},
        {"complex.caption.dropship_sell",&campaignService.dropShipStatusLabels[1].display,campaignService.dropShipStatusLabels[1].text,sizeof(campaignService.dropShipStatusLabels[1].text),0,5},
        {"complex.caption.dropship_leave",&campaignService.dropShipStatusLabels[2].display,campaignService.dropShipStatusLabels[2].text,sizeof(campaignService.dropShipStatusLabels[2].text),0,5},
        {"complex.caption.sale_prompt",&campaignMarket.saleConfirmationLabels[0].display,campaignMarket.saleConfirmationLabels[0].text,sizeof(campaignMarket.saleConfirmationLabels[0].text),0,0},
        {"complex.caption.sale_yes",&campaignMarket.saleConfirmationLabels[1].display,campaignMarket.saleConfirmationLabels[1].text,sizeof(campaignMarket.saleConfirmationLabels[1].text),0,0},
        {"complex.caption.sale_no",&campaignMarket.saleConfirmationLabels[2].display,campaignMarket.saleConfirmationLabels[2].text,sizeof(campaignMarket.saleConfirmationLabels[2].text),0,0},
        {"complex.caption.rearm_prompt",&campaignMarket.rearmConfirmationLabels[0].display,campaignMarket.rearmConfirmationLabels[0].text,sizeof(campaignMarket.rearmConfirmationLabels[0].text),0,0},
        {"complex.caption.rearm_yes",&campaignMarket.rearmConfirmationLabels[1].display,campaignMarket.rearmConfirmationLabels[1].text,sizeof(campaignMarket.rearmConfirmationLabels[1].text),0,0},
        {"complex.caption.rearm_no",&campaignMarket.rearmConfirmationLabels[2].display,campaignMarket.rearmConfirmationLabels[2].text,sizeof(campaignMarket.rearmConfirmationLabels[2].text),0,0},
        {"complex.caption.repair_title",&campaignService.repairTitle.display,campaignService.repairTitle.text,sizeof(campaignService.repairTitle.text),0,25},
        {"complex.caption.repair_system_1",&campaignService.repairSystemConditionRows[0].display,campaignService.repairSystemConditionRows[0].caption.text,sizeof(campaignService.repairSystemConditionRows[0].caption.text),0,26},
        {"complex.caption.repair_system_2",&campaignService.repairSystemConditionRows[1].display,campaignService.repairSystemConditionRows[1].caption.text,sizeof(campaignService.repairSystemConditionRows[1].caption.text),0,26},
        {"complex.caption.repair_system_3",&campaignService.repairSystemConditionRows[2].display,campaignService.repairSystemConditionRows[2].caption.text,sizeof(campaignService.repairSystemConditionRows[2].caption.text),0,26},
        {"complex.caption.repair_system_4",&campaignService.repairSystemConditionRows[3].display,campaignService.repairSystemConditionRows[3].caption.text,sizeof(campaignService.repairSystemConditionRows[3].caption.text),0,26},
        {"complex.caption.repair_system_5",&campaignService.repairSystemConditionRows[4].display,campaignService.repairSystemConditionRows[4].caption.text,sizeof(campaignService.repairSystemConditionRows[4].caption.text),0,26},
        {"complex.caption.repair_system_6",&campaignService.repairSystemConditionRows[5].display,campaignService.repairSystemConditionRows[5].caption.text,sizeof(campaignService.repairSystemConditionRows[5].caption.text),0,26},
        {"complex.caption.repair_system_7",&campaignService.repairSystemConditionRows[6].display,campaignService.repairSystemConditionRows[6].caption.text,sizeof(campaignService.repairSystemConditionRows[6].caption.text),0,26},
        {"complex.caption.repair_system_8",&campaignService.repairSystemConditionRows[7].display,campaignService.repairSystemConditionRows[7].caption.text,sizeof(campaignService.repairSystemConditionRows[7].caption.text),0,26},
        {"complex.caption.repair_system_9",&campaignService.repairSystemConditionRows[8].display,campaignService.repairSystemConditionRows[8].caption.text,sizeof(campaignService.repairSystemConditionRows[8].caption.text),0,26},
        {"complex.caption.repair_system_10",&campaignService.repairSystemConditionRows[9].display,campaignService.repairSystemConditionRows[9].caption.text,sizeof(campaignService.repairSystemConditionRows[9].caption.text),0,26},
        {"complex.caption.repair_body",&campaignService.repairBodyCondition.display,campaignService.repairBodyCondition.caption.text,sizeof(campaignService.repairBodyCondition.caption.text),0,19},
        {"complex.caption.repair_footer",&campaignService.repairFooter.display,campaignService.repairFooter.text,sizeof(campaignService.repairFooter.text),0,62},
        {"complex.caption.repair_weapon_1",&campaignService.repairWeaponConditionRows[0].display,campaignService.repairWeaponConditionRows[0].caption.text,sizeof(campaignService.repairWeaponConditionRows[0].caption.text),0,26},
        {"complex.caption.repair_weapon_2",&campaignService.repairWeaponConditionRows[1].display,campaignService.repairWeaponConditionRows[1].caption.text,sizeof(campaignService.repairWeaponConditionRows[1].caption.text),0,26},
        {"complex.caption.repair_weapon_3",&campaignService.repairWeaponConditionRows[2].display,campaignService.repairWeaponConditionRows[2].caption.text,sizeof(campaignService.repairWeaponConditionRows[2].caption.text),0,26},
        {"complex.caption.repair_weapon_4",&campaignService.repairWeaponConditionRows[3].display,campaignService.repairWeaponConditionRows[3].caption.text,sizeof(campaignService.repairWeaponConditionRows[3].caption.text),0,26},
        {"complex.caption.repair_weapon_5",&campaignService.repairWeaponConditionRows[4].display,campaignService.repairWeaponConditionRows[4].caption.text,sizeof(campaignService.repairWeaponConditionRows[4].caption.text),0,26},
        {"complex.caption.repair_weapon_6",&campaignService.repairWeaponConditionRows[5].display,campaignService.repairWeaponConditionRows[5].caption.text,sizeof(campaignService.repairWeaponConditionRows[5].caption.text),0,26},
        {"complex.caption.repair_weapon_7",&campaignService.repairWeaponConditionRows[6].display,campaignService.repairWeaponConditionRows[6].caption.text,sizeof(campaignService.repairWeaponConditionRows[6].caption.text),0,26},
        {"complex.caption.repair_weapon_8",&campaignService.repairWeaponConditionRows[7].display,campaignService.repairWeaponConditionRows[7].caption.text,sizeof(campaignService.repairWeaponConditionRows[7].caption.text),0,26},
        {"complex.caption.repair_weapon_9",&campaignService.repairWeaponConditionRows[8].display,campaignService.repairWeaponConditionRows[8].caption.text,sizeof(campaignService.repairWeaponConditionRows[8].caption.text),0,26},
        {"complex.caption.repair_weapon_10",&campaignService.repairWeaponConditionRows[9].display,campaignService.repairWeaponConditionRows[9].caption.text,sizeof(campaignService.repairWeaponConditionRows[9].caption.text),0,26},
        {"complex.caption.repair_weapon_11",&campaignService.repairWeaponConditionRows[10].display,campaignService.repairWeaponConditionRows[10].caption.text,sizeof(campaignService.repairWeaponConditionRows[10].caption.text),0,26}};
    const size_t complexCount=sizeof(complexOwners)/sizeof(complexOwners[0]);
    if (!MwImportUiCaptions(complexOwners,complexCount)
        || !MwImportUiPanel("complex.panel.complex",&campaignMarket.mechComplexPanel,5,complexOwners,complexCount)
        || !MwImportUiListPanel("complex.panel.owned",&campaignService.ownedUnitListPanel,14,complexOwners,complexCount)
        || !MwImportUiPanel("complex.panel.mech_status",&campaignService.mechStatusPanel,6,complexOwners,complexCount)
        || !MwImportUiPanel("complex.panel.dropship_status",&campaignService.dropShipStatusPanel,3,complexOwners,complexCount)
        || !MwImportUiPanel("complex.panel.sale",&campaignMarket.saleConfirmationPanel,3,complexOwners,complexCount)
        || !MwImportUiPanel("complex.panel.rearm",&campaignMarket.rearmConfirmationPanel,3,complexOwners,complexCount)
        || !MwImportUiListPanel("complex.panel.repair",&campaignService.componentRepairPanel,24,complexOwners,complexCount)
        || !MwImportUiMenu("complex.menu.complex",&campaignMarket.mechComplexMenu,complexOwners,complexCount)
        || !MwImportUiMenu("complex.menu.owned",&campaignService.ownedUnitListMenu,complexOwners,complexCount)
        || !MwImportUiMenu("complex.menu.mech_service",&campaignService.mechServiceMenu,complexOwners,complexCount)
        || !MwImportUiMenu("complex.menu.dropship_service",&campaignService.dropShipServiceMenu,complexOwners,complexCount)
        || !MwImportUiMenu("complex.menu.sale",&campaignMarket.saleConfirmationMenu,complexOwners,complexCount)
        || !MwImportUiMenu("complex.menu.rearm",&campaignMarket.rearmConfirmationMenu,complexOwners,complexCount)
        || !MwImportUiMenu("complex.menu.repair",&campaignService.componentRepairMenu,complexOwners,complexCount)) return false;
    /* data/ui/dealer.json: the market list (28F1, 2A1B), the 'Mech and
     * DropShip panels (437E, 44A9; 443E, 44D3) and the component panel (44F1,
     * 480F). Reserve ammunition is ImportReserveAmmoUiData's. */
    UiCaptionOwner dealerOwners[]={
        {"dealer.caption.market_heading",&campaignMarket.marketListHeading.display,campaignMarket.marketListHeading.text,sizeof(campaignMarket.marketListHeading.text),0,0},
        {"dealer.caption.market_1",&campaignMarket.marketUnitListRows[0].display,campaignMarket.marketUnitListRows[0].text,sizeof(campaignMarket.marketUnitListRows[0].text),0,13},
        {"dealer.caption.market_2",&campaignMarket.marketUnitListRows[1].display,campaignMarket.marketUnitListRows[1].text,sizeof(campaignMarket.marketUnitListRows[1].text),0,13},
        {"dealer.caption.market_3",&campaignMarket.marketUnitListRows[2].display,campaignMarket.marketUnitListRows[2].text,sizeof(campaignMarket.marketUnitListRows[2].text),0,13},
        {"dealer.caption.market_4",&campaignMarket.marketUnitListRows[3].display,campaignMarket.marketUnitListRows[3].text,sizeof(campaignMarket.marketUnitListRows[3].text),0,13},
        {"dealer.caption.market_5",&campaignMarket.marketUnitListRows[4].display,campaignMarket.marketUnitListRows[4].text,sizeof(campaignMarket.marketUnitListRows[4].text),0,13},
        {"dealer.caption.market_6",&campaignMarket.marketUnitListRows[5].display,campaignMarket.marketUnitListRows[5].text,sizeof(campaignMarket.marketUnitListRows[5].text),0,13},
        {"dealer.caption.market_7",&campaignMarket.marketUnitListRows[6].display,campaignMarket.marketUnitListRows[6].text,sizeof(campaignMarket.marketUnitListRows[6].text),0,13},
        {"dealer.caption.market_8",&campaignMarket.marketUnitListRows[7].display,campaignMarket.marketUnitListRows[7].text,sizeof(campaignMarket.marketUnitListRows[7].text),0,13},
        {"dealer.caption.market_9",&campaignMarket.marketUnitListRows[8].display,campaignMarket.marketUnitListRows[8].text,sizeof(campaignMarket.marketUnitListRows[8].text),0,13},
        {"dealer.caption.market_10",&campaignMarket.marketUnitListRows[9].display,campaignMarket.marketUnitListRows[9].text,sizeof(campaignMarket.marketUnitListRows[9].text),0,13},
        {"dealer.caption.market_11",&campaignMarket.marketUnitListRows[10].display,campaignMarket.marketUnitListRows[10].text,sizeof(campaignMarket.marketUnitListRows[10].text),0,13},
        {"dealer.caption.market_12",&campaignMarket.marketUnitListRows[11].display,campaignMarket.marketUnitListRows[11].text,sizeof(campaignMarket.marketUnitListRows[11].text),0,13},
        {"dealer.caption.market_13",&campaignMarket.marketUnitListRows[12].display,campaignMarket.marketUnitListRows[12].text,sizeof(campaignMarket.marketUnitListRows[12].text),0,13},
        {"dealer.caption.mech_title",&campaignMarket.marketMechStatusLabels[0].display,campaignMarket.marketMechStatusLabels[0].text,sizeof(campaignMarket.marketMechStatusLabels[0].text),0,0},
        {"dealer.caption.mech_buy",&campaignMarket.marketMechStatusLabels[1].display,campaignMarket.marketMechStatusLabels[1].text,sizeof(campaignMarket.marketMechStatusLabels[1].text),0,0},
        {"dealer.caption.mech_inspect",&campaignMarket.marketMechStatusLabels[2].display,campaignMarket.marketMechStatusLabels[2].text,sizeof(campaignMarket.marketMechStatusLabels[2].text),0,0},
        {"dealer.caption.mech_leave",&campaignMarket.marketMechStatusLabels[3].display,campaignMarket.marketMechStatusLabels[3].text,sizeof(campaignMarket.marketMechStatusLabels[3].text),0,0},
        {"dealer.caption.dropship_title",&campaignMarket.marketDropShipStatusLabels[0].display,campaignMarket.marketDropShipStatusLabels[0].text,sizeof(campaignMarket.marketDropShipStatusLabels[0].text),0,0},
        {"dealer.caption.dropship_buy",&campaignMarket.marketDropShipStatusLabels[1].display,campaignMarket.marketDropShipStatusLabels[1].text,sizeof(campaignMarket.marketDropShipStatusLabels[1].text),0,0},
        {"dealer.caption.dropship_leave",&campaignMarket.marketDropShipStatusLabels[2].display,campaignMarket.marketDropShipStatusLabels[2].text,sizeof(campaignMarket.marketDropShipStatusLabels[2].text),0,0},
        {"dealer.caption.component_title",&campaignMarket.marketComponentTitle.display,campaignMarket.marketComponentTitle.text,sizeof(campaignMarket.marketComponentTitle.text),0,0},
        {"dealer.caption.component_system_1",&campaignMarket.marketSystemConditionRows[0].display,campaignMarket.marketSystemConditionRows[0].caption.text,sizeof(campaignMarket.marketSystemConditionRows[0].caption.text),0,26},
        {"dealer.caption.component_system_2",&campaignMarket.marketSystemConditionRows[1].display,campaignMarket.marketSystemConditionRows[1].caption.text,sizeof(campaignMarket.marketSystemConditionRows[1].caption.text),0,26},
        {"dealer.caption.component_system_3",&campaignMarket.marketSystemConditionRows[2].display,campaignMarket.marketSystemConditionRows[2].caption.text,sizeof(campaignMarket.marketSystemConditionRows[2].caption.text),0,26},
        {"dealer.caption.component_system_4",&campaignMarket.marketSystemConditionRows[3].display,campaignMarket.marketSystemConditionRows[3].caption.text,sizeof(campaignMarket.marketSystemConditionRows[3].caption.text),0,26},
        {"dealer.caption.component_system_5",&campaignMarket.marketSystemConditionRows[4].display,campaignMarket.marketSystemConditionRows[4].caption.text,sizeof(campaignMarket.marketSystemConditionRows[4].caption.text),0,26},
        {"dealer.caption.component_system_6",&campaignMarket.marketSystemConditionRows[5].display,campaignMarket.marketSystemConditionRows[5].caption.text,sizeof(campaignMarket.marketSystemConditionRows[5].caption.text),0,26},
        {"dealer.caption.component_system_7",&campaignMarket.marketSystemConditionRows[6].display,campaignMarket.marketSystemConditionRows[6].caption.text,sizeof(campaignMarket.marketSystemConditionRows[6].caption.text),0,26},
        {"dealer.caption.component_system_8",&campaignMarket.marketSystemConditionRows[7].display,campaignMarket.marketSystemConditionRows[7].caption.text,sizeof(campaignMarket.marketSystemConditionRows[7].caption.text),0,26},
        {"dealer.caption.component_system_9",&campaignMarket.marketSystemConditionRows[8].display,campaignMarket.marketSystemConditionRows[8].caption.text,sizeof(campaignMarket.marketSystemConditionRows[8].caption.text),0,26},
        {"dealer.caption.component_system_10",&campaignMarket.marketSystemConditionRows[9].display,campaignMarket.marketSystemConditionRows[9].caption.text,sizeof(campaignMarket.marketSystemConditionRows[9].caption.text),0,26},
        {"dealer.caption.component_body",&campaignMarket.marketBodyCondition.display,campaignMarket.marketBodyCondition.caption.text,sizeof(campaignMarket.marketBodyCondition.caption.text),0,19},
        {"dealer.caption.component_weapon_heading",&campaignMarket.marketWeaponHeading.display,campaignMarket.marketWeaponHeading.text,sizeof(campaignMarket.marketWeaponHeading.text),0,0},
        {"dealer.caption.component_weapon_1",&campaignMarket.marketWeaponConditionRows[0].display,campaignMarket.marketWeaponConditionRows[0].caption.text,sizeof(campaignMarket.marketWeaponConditionRows[0].caption.text),0,26},
        {"dealer.caption.component_weapon_2",&campaignMarket.marketWeaponConditionRows[1].display,campaignMarket.marketWeaponConditionRows[1].caption.text,sizeof(campaignMarket.marketWeaponConditionRows[1].caption.text),0,26},
        {"dealer.caption.component_weapon_3",&campaignMarket.marketWeaponConditionRows[2].display,campaignMarket.marketWeaponConditionRows[2].caption.text,sizeof(campaignMarket.marketWeaponConditionRows[2].caption.text),0,26},
        {"dealer.caption.component_weapon_4",&campaignMarket.marketWeaponConditionRows[3].display,campaignMarket.marketWeaponConditionRows[3].caption.text,sizeof(campaignMarket.marketWeaponConditionRows[3].caption.text),0,26},
        {"dealer.caption.component_weapon_5",&campaignMarket.marketWeaponConditionRows[4].display,campaignMarket.marketWeaponConditionRows[4].caption.text,sizeof(campaignMarket.marketWeaponConditionRows[4].caption.text),0,26},
        {"dealer.caption.component_weapon_6",&campaignMarket.marketWeaponConditionRows[5].display,campaignMarket.marketWeaponConditionRows[5].caption.text,sizeof(campaignMarket.marketWeaponConditionRows[5].caption.text),0,26},
        {"dealer.caption.component_weapon_7",&campaignMarket.marketWeaponConditionRows[6].display,campaignMarket.marketWeaponConditionRows[6].caption.text,sizeof(campaignMarket.marketWeaponConditionRows[6].caption.text),0,26},
        {"dealer.caption.component_weapon_8",&campaignMarket.marketWeaponConditionRows[7].display,campaignMarket.marketWeaponConditionRows[7].caption.text,sizeof(campaignMarket.marketWeaponConditionRows[7].caption.text),0,26},
        {"dealer.caption.component_weapon_9",&campaignMarket.marketWeaponConditionRows[8].display,campaignMarket.marketWeaponConditionRows[8].caption.text,sizeof(campaignMarket.marketWeaponConditionRows[8].caption.text),0,26},
        {"dealer.caption.component_weapon_10",&campaignMarket.marketWeaponConditionRows[9].display,campaignMarket.marketWeaponConditionRows[9].caption.text,sizeof(campaignMarket.marketWeaponConditionRows[9].caption.text),0,26},
        {"dealer.caption.component_done",&campaignMarket.marketComponentDone.display,campaignMarket.marketComponentDone.text,sizeof(campaignMarket.marketComponentDone.text),0,0}};
    const size_t dealerCount=sizeof(dealerOwners)/sizeof(dealerOwners[0]);
    if (!MwImportUiCaptions(dealerOwners,dealerCount)
        || !MwImportUiListPanel("dealer.panel.market",&campaignMarket.marketUnitListPanel,14,dealerOwners,dealerCount)
        || !MwImportUiPanel("dealer.panel.mech",&campaignMarket.marketMechStatusPanel,4,dealerOwners,dealerCount)
        || !MwImportUiPanel("dealer.panel.dropship",&campaignMarket.marketDropShipStatusPanel,3,dealerOwners,dealerCount)
        || !MwImportUiListPanel("dealer.panel.component",&campaignMarket.marketComponentPanel,24,dealerOwners,dealerCount)
        || !MwImportUiMenu("dealer.menu.market",&campaignMarket.marketUnitListMenu,dealerOwners,dealerCount)
        || !MwImportUiMenu("dealer.menu.mech",&campaignMarket.marketMechMenu,dealerOwners,dealerCount)
        || !MwImportUiMenu("dealer.menu.dropship",&campaignMarket.marketDropShipMenu,dealerOwners,dealerCount)
        || !MwImportUiMenu("dealer.menu.component",&campaignMarket.marketComponentMenu,dealerOwners,dealerCount)) return false;
    if (!MwImportUiMenu("hub.menu.planet",&campaignUi.planetHubMenu,NULL,0)) return false; /*1083*/
    if (!MwImportUiMenu("hub.menu.contract",&campaignUi.contractHubMenu,NULL,0)) return false; /*10D7*/
    if (!MwImportUiMenu("hub.menu.remote_planet",&campaignUi.remotePlanetHubMenu,NULL,0)) return false; /*1101*/
    /* The body diagrams (3CEE): a background and nine regions each. */
    uint16_t diagrams[8*10*4];
    if (!MwUiCopyWords("complex.table.body_diagrams",diagrams,sizeof(diagrams)/sizeof(diagrams[0]),0)) return false;
    for (uint16_t chassis=0;chassis<8;++chassis)
        for (uint16_t region=0;region<10;++region)
        {
            MwMechDiagramRectangle *rectangle=region?&campaignService.mechBodyDiagramLayouts[chassis].bodyRegions[region-1]:&campaignService.mechBodyDiagramLayouts[chassis].background;
            const uint16_t *at=diagrams+(chassis*10+region)*4;
            rectangle->x=at[0];rectangle->y=at[1];rectangle->width=at[2];rectangle->height=at[3];
        }
    /* These initial near values have no live object until their callers set them. */
    campaignMechanics.repairQuoteDamage=NULL;campaignMechanics.componentRepairDamage=NULL;
    campaignService.mechDiagramBodyDamage=NULL;campaignService.mechDiagramRectangle=NULL;campaignUi.uiMenu=NULL;
    return ImportPilotUiData() && ImportTravelUiData() && MwImportCampaignEventData()
        && MwImportCampaignOptionsData()
        && CheckTextBoundTables();
}
/* Installation decoder, not an added native gameplay procedure. */
static bool ImportPilotUiData(void)
{
    /* data/ui/crew.json: company 21CE, commander profile 2258, crew 2325,
     * bar 8FF6 and recruit 9076, with their menus 222E, 238A, 904C, 90EA. */
    static const char *const profileIds[11]={"crew.caption.profile_1","crew.caption.profile_2","crew.caption.profile_3",
        "crew.caption.profile_4","crew.caption.profile_5","crew.caption.profile_6","crew.caption.profile_7",
        "crew.caption.profile_8","crew.caption.profile_9","crew.caption.profile_10","crew.caption.profile_11"};
    static const char *const companyIds[4]={"crew.caption.company_title","crew.caption.company_crew",
        "crew.caption.company_news","crew.caption.company_leave"};
    static const char *const crewIds[7]={"crew.caption.crew_1","crew.caption.crew_2","crew.caption.crew_3",
        "crew.caption.crew_4","crew.caption.crew_5","crew.caption.crew_6","crew.caption.crew_leave"};
    static const char *const barIds[4]={"crew.caption.bar_title","crew.caption.bar_drink","crew.caption.bar_recruit","crew.caption.bar_leave"};
    static const char *const recruitIds[3]={"crew.caption.recruit_prompt","crew.caption.recruit_hire","crew.caption.recruit_reject"};
    UiCaptionOwner owners[4+11+7+4+3];
    size_t count=0;
    for (uint16_t i=0;i<4;++i) owners[count++]={companyIds[i],&campaignPilots.companyMenuLabels[i].display,campaignPilots.companyMenuLabels[i].text,sizeof(campaignPilots.companyMenuLabels[i].text),0};
    for (uint16_t i=0;i<11;++i) owners[count++]={profileIds[i],&campaignPilots.commanderProfileLabels[i].display,campaignPilots.commanderProfileLabels[i].text,sizeof(campaignPilots.commanderProfileLabels[i].text),0};
    for (uint16_t i=0;i<7;++i) owners[count++]={crewIds[i],&campaignPilots.crewLabels[i].display,campaignPilots.crewLabels[i].text,sizeof(campaignPilots.crewLabels[i].text),0};
    for (uint16_t i=0;i<4;++i) owners[count++]={barIds[i],&campaignPilots.barMenuLabels[i].display,campaignPilots.barMenuLabels[i].text,sizeof(campaignPilots.barMenuLabels[i].text),0};
    for (uint16_t i=0;i<3;++i) owners[count++]={recruitIds[i],&campaignPilots.recruitInterviewLabels[i].display,campaignPilots.recruitInterviewLabels[i].text,sizeof(campaignPilots.recruitInterviewLabels[i].text),0};
    /* The crew menu's pilot rows (1..4) have no caption: 273E/28B2 assigns
     * them before any pilot record is selected. */
    if (!MwImportUiCaptions(owners,count)
        || !MwImportUiPanel("crew.panel.company",&campaignPilots.companyMenuPanel,4,owners,count)
        || !MwImportUiPanel("crew.panel.commander_profile",&campaignPilots.commanderProfilePanel,11,owners,count)
        || !MwImportUiPanel("crew.panel.crew",&campaignPilots.crewLabelsPanel,7,owners,count)
        || !MwImportUiPanel("crew.panel.bar",&campaignPilots.barMenuPanel,4,owners,count)
        || !MwImportUiPanel("crew.panel.recruit",&campaignPilots.recruitInterviewPanel,3,owners,count)
        || !MwImportUiMenu("crew.menu.company",&campaignPilots.companyMenu,owners,count)
        || !MwImportUiMenu("crew.menu.crew",&campaignPilots.crewMenu,owners,count)
        || !MwImportUiMenu("crew.menu.bar",&campaignPilots.barMenu,owners,count)
        || !MwImportUiMenu("crew.menu.recruit",&campaignPilots.recruitInterviewMenu,owners,count)) return false;
    campaignPilots.crewScreenRightEdge=0; /* 2438: Written before it's read; zero where the exe reloaded it. */
    campaignPilots.pilotCardRosterSlot=0; /*243A*/
    campaignPilots.pilotCardTextX=0; /* 243C: Written before it's read; zero where the exe reloaded it. */
    campaignPilots.displayedPilotId=0; /*243E*/
    campaignPilots.pilotCardGunneryTier=0; /* Written before it's read; zero where the exe reloaded it. */
    campaignPilots.pilotCardPilotingTier=0; /* Written before it's read; zero where the exe reloaded it. */
    campaignPilots.pilotCardPortraitId=0; /*2444*/
    campaignPilots.pilotCardQuotedWageTier=0; /* Written before it's read; zero where the exe reloaded it. */
    campaignPilots.pilotCardOwnedUnitSlot=0; /*2448*/
    campaignPilots.assignmentSelectedPilotSlot=0; /*2581*/
    campaignPilots.assignmentCandidateOwnedSlot=0; /*2583*/
    campaignPilots.assignmentConflictingPilotSlot=0; /*2586*/
    campaignPilots.assignmentConflictingOwnedSlot=0; /*2588*/
    campaignPilots.recruitOfferSlot=0; /*911D*/
    campaignPilots.selectedRecruitPilotId=0; /*911F*/
    campaignPilots.assignmentConflictHighlighted=0; /*2585*/
    if (!MwUiCopyWords("crew.table.pilot_card_column_origins",campaignPilots.pilotCardColumnOrigins,PILOT_CARD_COUNT,0)) return false; /*244A*/
    /* The crew's words (crew.word.*), through the pointer tables crew.json
     * names them by: reputation 23D0, skill 2452 and attitude 23FB. */
    uint16_t reputationWords[4],skillWords[4],attitudeWords[9];
    if (!MwUiCopyWords("crew.table.reputation_words",reputationWords,4,0)
        || !MwUiCopyWords("crew.table.skill_words",skillWords,4,0)
        || !MwUiCopyWords("crew.table.attitude_words",attitudeWords,9,0)) return false;
    for (uint16_t i=0;i<4;++i)
    {
        if (!ImportInitialString(mainWords,reputationWords[i],campaignPilots.commanderReputationLabels[i],sizeof(campaignPilots.commanderReputationLabels[i]))) return false;
        if (!ImportInitialString(mainWords,skillWords[i],campaignPilots.pilotSkillLabels[i],sizeof(campaignPilots.pilotSkillLabels[i]))) return false;
    }
    const uint16_t attitudeOffsets[5]={0x240D,0x2414,0x241D,0x2425,0x242E};
    for (uint16_t i=0;i<5;++i)
        if (!ImportInitialString(mainWords,attitudeOffsets[i],campaignPilots.houseAttitudeLabels[i],sizeof(campaignPilots.houseAttitudeLabels[i]))) return false;
    for (uint16_t i=0;i<9;++i)
    {
        uint16_t offset=attitudeWords[i],label=0;
        while (label<5 && attitudeOffsets[label]!=offset) ++label;
        if (label==5) return false;
        campaignPilots.houseAttitudeText[i]=campaignPilots.houseAttitudeLabels[label];
    }
    /* The pilot assignment captions: the pointer table (247E) names colour
     * captions, a colour byte each from the layout and the text after it. */
    uint16_t assignmentPointers[PILOT_ASSIGNMENT_CAPTION_COUNT];
    if (!MwUiCopyWords("crew.table.pilot_assignment_captions",assignmentPointers,PILOT_ASSIGNMENT_CAPTION_COUNT,0)) return false;
    for (uint16_t i=0;i<PILOT_ASSIGNMENT_CAPTION_COUNT;++i)
    {
        uint16_t caption=0;
        char id[32];
        while (caption<PILOT_ASSIGNMENT_CAPTION_COUNT)
        {
            snprintf(id,sizeof(id),"crew.caption.assignment_%u",(unsigned)(caption+1));
            if (MwUiCaptionOffset(id)==assignmentPointers[i]) break;
            ++caption;
        }
        if (caption==PILOT_ASSIGNMENT_CAPTION_COUNT || !MwUiCopyBytes(id,&campaignPilots.pilotAssignmentCaptionColours[i],1,0)) return false;
        char textId[40];
        snprintf(textId,sizeof(textId),"%s.text",id);  /* the caption's text, story/words.json */
        if (!MwWordsCopyText(textId,campaignPilots.pilotAssignmentCaptions[i],sizeof(campaignPilots.pilotAssignmentCaptions[i]))) return false;
    }
    campaignPilots.pilotDefinitions[0]=NULL;campaignPilots.pilotDialogue[0]=NULL;
    for (uint16_t pilotId=1;pilotId<CAMPAIGN_PILOT_ID_COUNT;++pilotId)
    {
        uint16_t offset=campaignMechanics.pilotDefinitionOffsets[pilotId];
        if (offset<PILOT_DEFINITIONS_START || (uint32_t)offset+sizeof(MwPilotDefinitionPrefix)>=PILOT_DEFINITIONS_END) return false;
        const uint8_t *record=&campaignMechanics.pilotDefinitionStorage[offset-PILOT_DEFINITIONS_START];
        if (!memchr(record+3,0,0x0B) || !memchr(record+0x0E,0,PILOT_DEFINITIONS_END-offset-0x0E)) return false;
        campaignPilots.pilotDefinitions[pilotId]=(const MwPilotDefinitionPrefix *)&campaignMechanics.pilotDefinitionStorage[offset-PILOT_DEFINITIONS_START];
        campaignPilots.pilotDialogue[pilotId]=(const char *)(campaignPilots.pilotDefinitions[pilotId]+1);
    }
    companyNavigationPadding=0; /*3020*/
    if (!ImportInitialString(mainWords,0x21C2,campaignPilots.commanderProfileName,sizeof(campaignPilots.commanderProfileName))) return false;
    if (!ImportInitialString(mainWords,0x9106,campaignPilots.recruitStatLabels,sizeof(campaignPilots.recruitStatLabels))) return false;
    if (!ImportInitialString(mainWords,0x9C39,campaignPilots.noAvailableRecruitsText,sizeof(campaignPilots.noAvailableRecruitsText))) return false;
    if (!ImportInitialString(mainWords,0x9C5A,campaignPilots.fullFourPilotRosterText,sizeof(campaignPilots.fullFourPilotRosterText))) return false;
    if (!ImportInitialString(mainWords,0x9C8D,campaignPilots.fullThreePilotRosterText,sizeof(campaignPilots.fullThreePilotRosterText))) return false;
    return ImportContractData();
}
static MwUiRecord contractOfficeRecords[2],contractActionRecords[6];
static uint8_t contractOfficeNavigationProbe,contractActionsNavigationProbe;
CampaignContractsState campaignContracts={
    .contractOfficeMenu={contractOfficeRecords,2,&contractOfficeNavigationProbe},
    .contractActionsMenu={contractActionRecords,6,&contractActionsNavigationProbe}
};
static uint8_t contractOfficePlanetGroupStorage[0x66];

/* Actual EXE import boundary: no runtime segment registry or copied live DS. */
static bool ImportContractData(void)
{
    campaignContracts.negotiatingMissionOfferSlot=0; /*488B*/
    campaignContracts.negotiatingMissionOfferByteOffset=0; /*488E*/
    campaignContracts.contractRequestEditorInitialized=0; /*488D*/
    campaignContracts.contractRequestTextColour=0; /* 4B34: Written before it's read; zero where the exe reloaded it. */
    campaignContracts.contractRequestTermByteOffset=0; /*4B32*/
    campaignContracts.contractNegotiationAttempts=0; /*4B7E*/
    campaignContracts.negotiationRequestsSatisfied=0; /*4B7D*/
    campaignContracts.acceptedContractAdvanceCbillsLow=0; /*4B73*/
    campaignContracts.acceptedContractAdvanceCbillsHigh=0; /*4B75*/
    campaignContracts.deploymentRefusalCount=0; /*0FBF*/campaignContracts.deploymentPenaltySuppressed=0; /*140B*/
    /* data/ui/contract_office.json: the year offsets (4B1A), and the requested
     * terms' x, then y, then digit counts (49CC). */
    if (!MwUiCopyWords("contract_office.table.date_year_x_offsets",campaignContracts.contractDateYearXOffsets,CAMPAIGN_MONTH_COUNT,0)
        || !MwUiCopyWords("contract_office.table.requested_terms",campaignContracts.requestedTermX,3,0)
        || !MwUiCopyWords("contract_office.table.requested_terms",campaignContracts.requestedTermY,3,6)
        || !MwUiCopyWords("contract_office.table.requested_terms",campaignContracts.requestedTermDigits,3,12)) return false;
    if (!ImportRuleWords("houses.extra_enemy_roll_thresholds",(uint16_t *)campaignContracts.houseExtraEnemyRollThresholds,CAMPAIGN_HOUSE_COUNT)) return false;
    memcpy(campaignContracts.submitContractCaption,mainWords+0x4B35,6);memcpy(campaignContracts.acceptContractCaption,mainWords+0x4B3B,6);
    /* The planet groups (4B8A) and each House's pointer (4B80), from data/rules/houses.json. */
    uint16_t planetGroupOffsets[CAMPAIGN_HOUSE_COUNT];
    if (!ImportRuleBytes("houses.contract_planet_groups",contractOfficePlanetGroupStorage,sizeof(contractOfficePlanetGroupStorage))
        || !ImportRuleWords("houses.contract_planet_group_offsets",planetGroupOffsets,CAMPAIGN_HOUSE_COUNT)) return false;
    for (uint16_t house=0;house<CAMPAIGN_HOUSE_COUNT;++house)
    {
        char id[24];snprintf(id,sizeof(id),"house.%s.name",storyHouseKeys[house]);
        if (!MwStoryCopyText(id,campaignContracts.houseContractNames[house],sizeof(campaignContracts.houseContractNames[house]))) return false;
        uint16_t group=planetGroupOffsets[house];
        if (group<0x4B8A || group>=0x4BF0
            || !memchr(contractOfficePlanetGroupStorage+group-0x4B8A,CONTRACT_REGION_END,0x4BF0-group)) return false;
        campaignContracts.contractOfficePlanetGroups[house]=contractOfficePlanetGroupStorage+group-0x4B8A;
    }
    /* The office (panel 4822, menu 486F) and the offer's actions (49DE, 4A15). */
    UiCaptionOwner owners[]={
        {"contract_office.caption.request_offer",&campaignContracts.contractOfficeLabels[0].display,campaignContracts.contractOfficeLabels[0].text,sizeof(campaignContracts.contractOfficeLabels[0].text),0},
        {"contract_office.caption.leave_office",&campaignContracts.contractOfficeLabels[1].display,campaignContracts.contractOfficeLabels[1].text,sizeof(campaignContracts.contractOfficeLabels[1].text),0},
        {"contract_office.caption.negotiate",&campaignContracts.contractActionLabels[0].display,campaignContracts.contractActionLabels[0].text,sizeof(campaignContracts.contractActionLabels[0].text),0},
        {"contract_office.caption.next_offer",&campaignContracts.contractActionLabels[1].display,campaignContracts.contractActionLabels[1].text,sizeof(campaignContracts.contractActionLabels[1].text),0},
        {"contract_office.caption.back_to_office",&campaignContracts.contractActionLabels[2].display,campaignContracts.contractActionLabels[2].text,sizeof(campaignContracts.contractActionLabels[2].text),0}};
    const size_t count=sizeof(owners)/sizeof(owners[0]);
    if (!MwImportUiCaptions(owners,count)
        || !MwImportUiPanel("contract_office.panel.office",&campaignContracts.contractOfficePanel,2,owners,count)
        || !MwImportUiPanel("contract_office.panel.actions",&campaignContracts.contractActionsPanel,3,owners,count)
        || !MwImportUiMenu("contract_office.menu.office",&campaignContracts.contractOfficeMenu,owners,count)
        || !MwImportUiMenu("contract_office.menu.actions",&campaignContracts.contractActionsMenu,owners,count)) return false;
    if (!MwStoryCopyBytes("probe.contract_office",&contractOfficeNavigationProbe,1)
        || !MwStoryCopyBytes("probe.contract_actions",&contractActionsNavigationProbe,1)) return false;
    return MwStoryCopyText("contract.narrative",campaignContracts.contractNarrative,sizeof(campaignContracts.contractNarrative))
        && ImportInitialString(mainWords,0x4BFA,campaignContracts.noContractsText,sizeof(campaignContracts.noContractsText))
        && ImportInitialString(mainWords,0x4C25,campaignContracts.negotiationTerminatedText,sizeof(campaignContracts.negotiationTerminatedText))
        && ImportInitialString(mainWords,0x4C6C,campaignContracts.contractAcceptedText,sizeof(campaignContracts.contractAcceptedText))
        && MwStoryCopyText("contract.refusal.house",campaignContracts.houseContractRefusalText,sizeof(campaignContracts.houseContractRefusalText))
        && ImportInitialString(mainWords,0x0FC0,campaignContracts.commanderNeedsMechText,sizeof(campaignContracts.commanderNeedsMechText))
        && ImportInitialString(mainWords,0x0FF7,campaignContracts.assignedMechUnreadyText,sizeof(campaignContracts.assignedMechUnreadyText))
        && ImportInitialString(mainWords,0x1031,campaignContracts.contractCancelledText,sizeof(campaignContracts.contractCancelledText));
}
CampaignMarketState campaignMarket={
    /* Native FF-neighbour probe is 1090:505A, the existing travel table WORD. */
    .reserveAmmoShopMenu={campaignMarket.reserveAmmoShopRecords,RESERVE_AMMO_MENU_ROWS,NULL,&campaignMechanics.travelSquareRootValues[0x1BA],0},
    /* FF navigation probes alias the canonical caption/diagram/root-table owners;
     * they are read before the dispatcher rejects the FF navigation link. */
    .rearmConfirmationMenu={rearmConfirmationRecords,2,NULL,&campaignService.mechBodyDiagramLayouts[7].bodyRegions[7].x,8},
    .saleConfirmationMenu={saleConfirmationRecords,2,NULL,&campaignService.mechBodyDiagramLayouts[6].bodyRegions[1].width,0},
    /*380D, DONE label byte. */
    .marketUnitListMenu={marketUnitListRecords,0x0D,(const uint8_t *)&campaignService.repairWeaponLabelStorage[CAMPAIGN_UNIT_SHADOW_HAWK][4][2]},
    /*529B, high BYTE. */
    .marketMechMenu={marketMechRecords,3,NULL,&campaignMechanics.travelSquareRootValues[0x02DA],8},
    /*52C5, high BYTE. */
    .marketDropShipMenu={marketDropShipRecords,2,NULL,&campaignMechanics.travelSquareRootValues[0x02EF],8},
    /*5601, final space. */
    .marketComponentMenu={marketComponentRecords,1,(const uint8_t *)&campaignMarket.positionedText.text[0x10]},
    .mechComplexMenu={mechComplexRecords,4,NULL,&campaignService.repairWeaponConditionRows[1].display.y,0}
};

static bool ImportReserveAmmoUiData(void)
{
    campaignMarket.reserveAmmoShopSelectedRow=0; /*4137*/
    campaignMarket.reserveAmmoPreviewSelection=0; /*3F6E*/
    campaignMarket.reserveAmmoSelling=0; /*4139*/
    memset(campaignMarket.reserveAmmoMechColours,0,sizeof(campaignMarket.reserveAmmoMechColours)); /* 3F70: Written before it's read; zero where the exe reloaded it. */
    if (!ImportInitialString(mainWords,0x413A,campaignMarket.reserveAmmoBuyLabel,sizeof(campaignMarket.reserveAmmoBuyLabel))
        || !ImportInitialString(mainWords,0x413F,campaignMarket.reserveAmmoSellLabel,sizeof(campaignMarket.reserveAmmoSellLabel))) return false;
    /* data/ui/dealer.json: the shop panel (4144) and menu (4268), and the six
     * compatibility panels the pointer table at 3F7B names, in family order. */
    static const char *const familyIds[CAMPAIGN_AMMO_FAMILY_COUNT]={"dealer.caption.ammo_family_1","dealer.caption.ammo_family_2",
        "dealer.caption.ammo_family_3","dealer.caption.ammo_family_4","dealer.caption.ammo_family_5","dealer.caption.ammo_family_6"};
    static const char *const quantityIds[CAMPAIGN_AMMO_FAMILY_COUNT]={"dealer.caption.ammo_quantity_1","dealer.caption.ammo_quantity_2",
        "dealer.caption.ammo_quantity_3","dealer.caption.ammo_quantity_4","dealer.caption.ammo_quantity_5","dealer.caption.ammo_quantity_6"};
    static const char *const compatibilityIds[CAMPAIGN_AMMO_FAMILY_COUNT]={"dealer.caption.compatibility_1","dealer.caption.compatibility_2",
        "dealer.caption.compatibility_3","dealer.caption.compatibility_4","dealer.caption.compatibility_5","dealer.caption.compatibility_6"};
    static const char *const compatibilityPanelIds[CAMPAIGN_AMMO_FAMILY_COUNT]={"dealer.panel.compatibility_1","dealer.panel.compatibility_2",
        "dealer.panel.compatibility_3","dealer.panel.compatibility_4","dealer.panel.compatibility_5","dealer.panel.compatibility_6"};
    UiCaptionOwner owners[2+3*CAMPAIGN_AMMO_FAMILY_COUNT];
    size_t count=0;
    owners[count++]={"dealer.caption.ammo_heading",&campaignMarket.reserveAmmoShopHeading.display,campaignMarket.reserveAmmoShopHeading.text,sizeof(campaignMarket.reserveAmmoShopHeading.text),0,0};
    for (uint16_t i=0;i<CAMPAIGN_AMMO_FAMILY_COUNT;++i)
        owners[count++]={familyIds[i],&campaignMarket.reserveAmmoFamilyLabels[i].display,campaignMarket.reserveAmmoFamilyLabels[i].text,sizeof(campaignMarket.reserveAmmoFamilyLabels[i].text),0,0};
    for (uint16_t i=0;i<CAMPAIGN_AMMO_FAMILY_COUNT;++i)
        owners[count++]={quantityIds[i],&campaignMarket.reserveAmmoQuantityRows[i].display,campaignMarket.reserveAmmoQuantityRows[i].text,sizeof(campaignMarket.reserveAmmoQuantityRows[i].text),0,0};
    owners[count++]={"dealer.caption.ammo_done",&campaignMarket.reserveAmmoDoneLabel.display,campaignMarket.reserveAmmoDoneLabel.text,sizeof(campaignMarket.reserveAmmoDoneLabel.text),0,0};
    for (uint16_t i=0;i<CAMPAIGN_AMMO_FAMILY_COUNT;++i)
        owners[count++]={compatibilityIds[i],&campaignMarket.reserveAmmoCompatibilityHeadings[i].display,campaignMarket.reserveAmmoCompatibilityHeadings[i].text,
            sizeof(campaignMarket.reserveAmmoCompatibilityHeadings[i].text),0,0};
    if (!MwImportUiCaptions(owners,count)
        || !MwImportUiPanel("dealer.panel.ammo",&campaignMarket.reserveAmmoShopPanel,14,owners,count)
        || !MwImportUiMenu("dealer.menu.ammo",&campaignMarket.reserveAmmoShopMenu,owners,count)) return false;
    /* The mechs each family fits (data/rules/units.json): a pointer per family
     * (3F87) into the lists at 3F9F, each ended by FFFF. */
    enum { AMMO_FAMILY_MECHS_START=0x3F9F, AMMO_FAMILY_MECH_WORDS=18 };
    uint16_t familyMechLists[CAMPAIGN_AMMO_FAMILY_COUNT],familyMechs[AMMO_FAMILY_MECH_WORDS];
    if (!ImportRuleWords("units.ammo_family_mech_lists",familyMechLists,CAMPAIGN_AMMO_FAMILY_COUNT)
        || !ImportRuleWords("units.ammo_family_mechs",familyMechs,AMMO_FAMILY_MECH_WORDS)) return false;
    uint16_t compatibilityPanels[CAMPAIGN_AMMO_FAMILY_COUNT];
    if (!MwUiCopyWords("dealer.table.compatibility_panels",compatibilityPanels,CAMPAIGN_AMMO_FAMILY_COUNT,0)) return false;
    for (uint16_t family=0;family<CAMPAIGN_AMMO_FAMILY_COUNT;++family)
    {
        uint16_t panel=0;
        while (panel<CAMPAIGN_AMMO_FAMILY_COUNT && MwUiPieceOffset(compatibilityPanelIds[panel])!=compatibilityPanels[family]) ++panel;
        if (panel==CAMPAIGN_AMMO_FAMILY_COUNT
            || !MwImportUiPanel(compatibilityPanelIds[panel],&campaignMarket.reserveAmmoCompatibilityPanels[family],1,owners,count)) return false;
        uint16_t kinds=familyMechLists[family],index=0;
        if (kinds<AMMO_FAMILY_MECHS_START || (kinds-AMMO_FAMILY_MECHS_START)%2) return false;
        do
        {
            const uint16_t word=(uint16_t)((kinds-AMMO_FAMILY_MECHS_START)/2+index);
            if (index>=6 || word>=AMMO_FAMILY_MECH_WORDS) return false;
            uint16_t kind=familyMechs[word];
            campaignMarket.reserveAmmoCompatibleMechs[family][index++]=kind;
            if (kind==UINT16_MAX) break;
            if (kind>=CAMPAIGN_MECH_KIND_COUNT) return false;
        } while (true);
    }
    return true;
}
/* The three layout tables that measure words (docs/phase2/PLAN.md 7.3). While
 * the words are the exe's they stay exact in data/ui, and are checked here
 * against the words they measure, so an edit that no longer fits is refused:
 * - a month's calendar width (hub.json) is its name's width in the 6-pixel
 *   campaign font, and its contract year offset (contract_office.json) the
 *   same in the 8-pixel contract font, both without the name's final space;
 * - a 'Mech kind's repair title padding (complex.json) centres its name in
 *   the name buffer's twelve columns, the odd space on the left. */
static bool CheckTextBoundTables(void)
{
    enum { CONTRACT_GLYPH_ADVANCE = 8 };
    for (uint16_t month=0;month<CAMPAIGN_MONTH_COUNT;++month)
    {
        const size_t length=strlen((const char *)campaignCore.calendarMonthNames[month].cursor);
        if (!length || campaignCore.calendarMonthNameWidths[month]!=(length-1)*CAMPAIGN_GLYPH_ADVANCE)
            return fprintf(stderr,"MechWarrior: hub.json's calendar_month_name_widths doesn't fit month %u's name\n",month+1),false;
        if (campaignContracts.contractDateYearXOffsets[month]!=(length-1)*CONTRACT_GLYPH_ADVANCE)
            return fprintf(stderr,"MechWarrior: contract_office.json's date_year_x_offsets doesn't fit month %u's name\n",month+1),false;
    }
    static_assert(sizeof(campaignService.repairTitleLeadingSpacesByKind)/sizeof(campaignService.repairTitleLeadingSpacesByKind[0])==CAMPAIGN_UNIT_KIND_COUNT,"one padding per kind");
    for (uint16_t kind=0;kind<CAMPAIGN_UNIT_KIND_COUNT;++kind)
        if (campaignService.repairTitleLeadingSpacesByKind[kind]!=(sizeof(campaignService.unitDisplayNames[kind])-strlen(campaignService.unitDisplayNames[kind]))/2)
            return fprintf(stderr,"MechWarrior: complex.json's repair_title_leading_spaces doesn't fit the %s's name\n",storyUnitKeys[kind]),false;
    return true;
}
