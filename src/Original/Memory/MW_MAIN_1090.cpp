#include "mechwarrior/campaign.h"
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
uint8_t campaignTimerInhibited, campaignTimerByte;
char combatExecutableFilename[10];
uint8_t deploymentResolvedWithoutCombat;
char darkWingVictoryText[0x185];
char afterActionCrewHeading[0x0D],afterActionMechHeading[0x0D];
char afterActionSalvageHeading[9],afterActionPaymentHeading[9];
char afterActionPercentSuffix[2],afterActionPilotStatusText[3][8];
uint16_t afterActionRosterRowOffset;
uint16_t campaignMechTonnages[CAMPAIGN_MECH_KIND_COUNT];
uint16_t settlementPayoutPercent;
uint16_t houseDeductionRollThresholds[CAMPAIGN_HOUSE_COUNT];
uint16_t houseReducedPayoutPercentages[CAMPAIGN_HOUSE_COUNT];
uint16_t missionDeploymentUnitsByScenario[DEPLOYMENT_SCENARIO_TIME_COUNT];
char deploymentUneventfulText[0x56];
char afterActionKuritaFailure[0x8D],afterActionLiaoFailure[0x73],afterActionGeneralFailure[0x91];
char afterActionKuritaSuccess[0x62],afterActionLiaoSuccess[0x6C],afterActionGeneralSuccess[0x44];
char settlementSupplyDeductionText[0xA8],settlementPerformanceDeductionText[0xAC];
char settlementTransportDeductionText[0x98],settlementNoPaymentText[0x95],darkWingAttackFailedText[0x49];
const char *afterActionFailureMessages[CAMPAIGN_HOUSE_COUNT]={afterActionKuritaFailure,
    afterActionGeneralFailure,afterActionGeneralFailure,afterActionLiaoFailure,afterActionGeneralFailure};
const char *afterActionSuccessMessages[CAMPAIGN_HOUSE_COUNT]={afterActionKuritaSuccess,
    afterActionGeneralSuccess,afterActionGeneralSuccess,afterActionLiaoSuccess,afterActionGeneralSuccess};
const char *settlementDeductionMessages[3]={settlementSupplyDeductionText,
    settlementPerformanceDeductionText,settlementTransportDeductionText};
uint16_t campaignTimerTicks;
uint8_t campaignTimerAlternatingPhase, campaignTimerPhaseTicks;
uint8_t mouseInstalled, joystickInstalled, inputSinglePoll;
uint16_t joystickDirection, joystickButtonOnePressed, joystickButtonTwoPressed;
uint8_t joystickPreviousButtons;
uint16_t joystickXCount, joystickYCount, joystickDeflection;
uint16_t mouseMotionX,mouseMotionY;
uint8_t joystickRepeatActive;
uint16_t joystickRepeatDeadline,joystickNavigationKeys[9];
uint16_t joystickCentreX,joystickCentreY;
uint16_t joystickXDeflectionStep,joystickYDeflectionStep;
uint16_t joystickXLow, joystickXHigh, joystickYLow, joystickYHigh;
uint16_t joystickXFarLow, joystickXFarHigh, joystickYFarLow, joystickYFarHigh;

/* Original UI tables occupy1083..1154 without an overlapping raw DS image.
 * Authored records/text still require native-image initialization. */
MwUiRecord planetHubRecords[HUB_UI_RECORD_COUNT];
MwUiRecord contractHubRecords[CONTRACT_HUB_UI_RECORD_COUNT];
MwUiRecord remotePlanetHubRecords[HUB_UI_RECORD_COUNT];
MwUiMenu *uiMenu;
uint16_t selectedUiRecordIndex;
uint16_t uiSelectionChangedCodeOffset, uiIdleCodeOffset;
/* FF link probes are read BEFORE sentinel rejection. No invented early guard. */
uint8_t hubFirstNavigationProbe,hubSecondNavigationProbe,hubThirdNavigationProbe;
uint16_t panelOriginX, panelOriginY, panelWidth, panelHeight;
uint16_t panelBorderPictureBase;
uint16_t panelBorderEnabled;
uint8_t commanderNameBytes[0x000A];
uint16_t ownedAmmoKindsByGroup[(int)CAMPAIGN_AMMO_GROUPS * CAMPAIGN_OWNED_UNIT_SLOTS];
uint16_t ownedAmmoQuantitiesByGroup[(int)CAMPAIGN_AMMO_GROUPS * CAMPAIGN_OWNED_UNIT_SLOTS];
uint16_t textPanelInsetX, textPanelInsetY;
uint16_t textPanelOriginX, textPanelOriginY;
uint16_t textPanelWidthPixels, textPanelHeightPixels;
uint16_t textPanelPreviousFont, textPanelFont;
uint16_t textPanelContinuationMode, textPanelYOverride;
uint16_t notificationTextInsetX[4], notificationTextInsetY[4];
uint16_t notificationColumns[4], notificationRows[4];
uint16_t notificationTextOffsets[4];
uint8_t notificationTextStorage[NOTIFICATION_TEXT_END - NOTIFICATION_TEXT_START];

/* Original 1090:016D. The allocator/loader assigns this selector later. */
uint16_t compressedPictureBufferSelector;

/* Original 1090:03F5/03F7 executable initializers. Do not substitute libc rand
 * or randomize these before the native BIOS seed call. */
uint16_t randomStateWord0 = 0x0615;
uint16_t randomStateWord1 = 0x1988;

/* Original 1090:0F6D..0F76; decoder-private storage. BYTE phases remain BYTEs,
 * not bools: direct helpers treat every nonzero phase alike. One canonical
 * owner, not a second copy of a raw DS image or serialized host structure. */
uint16_t pictureCompressedByteCount;
uint16_t pictureOutputSelector;
uint8_t pictureReadHighNibbleNext;
uint16_t pictureReadByteOffset;
uint8_t pictureWriteLowNibbleNext;
uint16_t pictureWriteByteOffset;

/* Native allocated-buffer WORDs; backing buffers belong to the allocator. */
uint16_t indexedPictureOutputSelector, generalPictureBankSelector;
uint16_t cockpitPictureBankSelector, activePictureBankSelector;
uint16_t secondaryPictureBankSelector;
uint8_t indexedPictureDirectoryLoaded;
uint16_t indexedPictureCachedIndex, indexedPictureStoredBytes;
uint8_t indexedPictureDirectory[INDEXED_PICTURE_DIRECTORY_BYTES];
uint16_t indexedPictureFileHandle, generalPictureFileHandle;
uint16_t selectedPictureFileHandle;
const char indexedPictureFilename[] = "MW_PICS.BIN";
const char generalPictureFilename[] = "MW_GPICS.BIN";
const char cockpitPictureFilename[] = "MW_CPICS.BIN";
const char primaryPictureFilename[] = "MW_1PICS.BIN";
const char secondaryPictureFilename[] = "MW_2PICS.BIN";
const char tPictureFilename[] = "MW_TPICS.BIN";
const char alternatePictureFilename[] = "MW_APICS.BIN";
const char fPictureFilename[] = "MW_FPICS.BIN";
/* Native private filename pointer; identity is tested only against 0E2C.
 * Host pointer keeps that identity; never serialize this as a native WORD. */
const char *selectedPictureFilename;

/* Native record region, not another full DS mirror. Initial data/near offsets
 * come from the original image; future mutable views must use this same owner. */
uint16_t planetRecordOffsets[CAMPAIGN_PLANET_COUNT];
uint8_t planetRecordStorage[PLANET_RECORDS_END - PLANET_RECORDS_START];
const uint8_t *calendarMonthNames[CAMPAIGN_MONTH_COUNT];
uint16_t calendarMonthNameWidths[CAMPAIGN_MONTH_COUNT];
uint16_t calendarMonthIndex, calendarYear;
uint16_t calendarTextX, calendarTextY;
uint8_t calendarBackground, calendarForeground;
uint8_t numericLeadingDigitSeen, numericGroupingEnabled;
uint8_t numericDecimalDigits[DECIMAL_DIGIT_COUNT];
uint16_t numericRemainderHigh, numericRemainderLow;
uint8_t numericSuppressLeadingZeros, numericNegativeSign;
uint16_t numericFieldDigitCount, numericCursorX, numericCursorY;
uint8_t numericBackgroundColour, numericForegroundColour;

/* 1ACE's planet/market outputs are original WORDs even for BYTE record fields.
 * Original image tables are populated by native data initialization, not by a
 * substitute safe/default market. The compiled startup is still unconverted. */
uint16_t currentPlanetIndex, currentPlanetLandscapeIndex;
uint16_t currentPlanetHouseIndex, currentPlanetMarketTier;
uint16_t currentPlanetFacilityDisplayFlag;
uint16_t currentPlanetMapPositionX, currentPlanetMapPositionY;
uint16_t planetMarketPremiumPercent, contractNegotiationAttemptLimit;
uint8_t houseNegotiationAttemptBase[CAMPAIGN_HOUSE_COUNT];
uint16_t ammunitionBasePrices[AMMUNITION_PRICE_COUNT];
uint16_t ammunitionTransactionPrices[AMMUNITION_PRICE_COUNT];
/* Shared by kind/component, NOT by owned Mech slot. 3FFD clears all 00DC bytes,
 * including the two trailing slots per row not reached by normal repair work. */
CampaignRepairAttemptRow repairAttemptFailuresByKind[REPAIR_ATTEMPT_KIND_COUNT];
static_assert(sizeof(repairAttemptFailuresByKind) == 0x00DC, "Native reset extent");

/* Native game tables/state. Original image initialization remains required;
 * no default prices, authored lookup values or pilot definitions are invented. */
CampaignUnitDamage ownedUnitDamage[CAMPAIGN_OWNED_UNIT_SLOTS];
CampaignUnitDamage marketUnitDamage[CAMPAIGN_MARKET_UNIT_SLOTS];
CampaignUnitDamage generatedMechDamage;
uint8_t unitWeaponCounts[CAMPAIGN_MECH_KIND_COUNT];
uint8_t unitPriceTierMultipliers[UNIT_PRICE_TIER_COUNT];
uint16_t unitBasePrices[CAMPAIGN_UNIT_KIND_COUNT];
uint16_t unitTransactionPriceLow, unitTransactionPriceHigh;
uint16_t selectedOwnedUnitKind, selectedMarketUnitKind;
uint16_t unitRepairQuoteLow, unitRepairQuoteHigh;
uint16_t repairQuoteMechKind;
CampaignUnitDamage *repairQuoteDamage;
uint16_t componentRepairPriceLow, componentRepairPriceHigh;
uint16_t repairPriceCategory;
CampaignUnitDamage *componentRepairDamage;
CampaignSystemRepairPriceRow systemRepairBasePrices[CAMPAIGN_SYSTEM_COUNT];
uint16_t *weaponRepairPricesByKind[CAMPAIGN_MECH_KIND_COUNT];
uint16_t weaponRepairPrices[REPAIR_WEAPON_PRICE_WORD_COUNT];
uint8_t bodyRepairBasePricesByKind[CAMPAIGN_MECH_KIND_COUNT];

CampaignPilotIdentityStorage pilotIdentityStorage;
uint8_t rosterStatus[CAMPAIGN_ROSTER_SLOTS];
uint16_t rosterPayTiers[CAMPAIGN_ROSTER_SLOTS];
uint16_t rosterGunneryTiers[CAMPAIGN_ROSTER_SLOTS];
uint16_t rosterPilotingTiers[CAMPAIGN_ROSTER_SLOTS];
uint16_t rosterUnknownWords[CAMPAIGN_ROSTER_SLOTS];
uint16_t rosterMechAssignments[CAMPAIGN_ROSTER_SLOTS];
uint16_t rosterPortraitIds[CAMPAIGN_ROSTER_SLOTS];
uint8_t pilotGunneryTiers[CAMPAIGN_PILOT_ID_COUNT];
uint8_t pilotPilotingTiers[CAMPAIGN_PILOT_ID_COUNT];
uint8_t pilotExperienceCounts[CAMPAIGN_PILOT_ID_COUNT];
uint16_t pilotDefinitionOffsets[CAMPAIGN_PILOT_ID_COUNT];
uint8_t pilotDefinitionStorage[PILOT_DEFINITIONS_END - PILOT_DEFINITIONS_START];

uint16_t travelSquareRootValues[TRAVEL_ROOT_TABLE_COUNT];
uint16_t travelDestinationMapPositionX, travelDestinationMapPositionY;
uint16_t travelMapDeltaX, travelMapDeltaY, travelDestinationTimeWeight;
uint16_t travelDestinationPlanetOffset, travelDistanceUnits;
uint16_t travelQuoteCbillsLow, travelQuoteCbillsHigh;
uint16_t travelMapSelection, travelDestinationPlanetIndex;
uint16_t travelMarkerX, travelMarkerY;
uint16_t travelSavedMarkerX, travelSavedMarkerY;
uint16_t travelJoystickLastTick, travelAnimationFrameStep;
uint8_t travelDropShipSelected;
uint16_t hiredTransportReverseTerminator;
uint16_t hiredTransportFrames[HIRED_TRANSPORT_FRAME_COUNT];
uint16_t dropShipTravelFrames[DROPSHIP_TRAVEL_FRAME_COUNT];
char travelDestinationName[TRAVEL_PLANET_NAME_CAPACITY];
uint16_t alphabeticPlanetRecordOffsets[TRAVEL_PLANET_DIRECTORY_COUNT];
uint16_t travelSelectedHouseByteOffset, travelHousePlanetStart;
uint16_t travelHousePlanetCount;
uint16_t travelSelectedHouseRow;
char travelHouseSearchNames[TRAVEL_HOUSE_COUNT][0x10];
MwPanelDescriptor travelHouseBrowserPanel, travelHousePlanetPanel;
MwServiceCaption travelHouseBrowserLabels[TRAVEL_HOUSE_BROWSER_LINE_COUNT];
MwServiceCaption travelHouseSearchCaption;
MwServiceCaption travelHousePlanetRows[TRAVEL_PLANET_ROWS_PER_HOUSE];
MwServiceCaption travelHousePlanetCancelLabel;
static MwUiRecord travelHouseBrowserRecords[TRAVEL_HOUSE_BROWSER_RECORD_COUNT];
static MwUiRecord travelHousePlanetRecords[TRAVEL_HOUSE_PLANET_RECORD_COUNT];
static MwUiRecord travelMapRecords[TRAVEL_MAP_MENU_RECORD_COUNT];
MwUiMenu travelHouseBrowserMenu={travelHouseBrowserRecords,TRAVEL_HOUSE_BROWSER_RECORD_COUNT,
    &planetRecordStorage[0x686D-PLANET_RECORDS_START]};
MwUiMenu travelHousePlanetMenu={travelHousePlanetRecords,TRAVEL_HOUSE_PLANET_RECORD_COUNT,
    NULL,&planetRecordOffsets[(0x65FA-0x6570)/sizeof(uint16_t)],8};
MwUiMenu travelMapMenu={travelMapRecords,TRAVEL_MAP_MENU_RECORD_COUNT,
    &planetRecordStorage[0x697E -PLANET_RECORDS_START]};
MwPanelDescriptor travelDestinationHeaderPanel;
MwPanelDescriptor travelCostPanel, travelConfirmPanel, travelBrowsePanel;
MwServiceCaption travelDestinationHeaderLabels[2];
MwServiceCaption travelCostLabel, travelConfirmLabel, travelBrowseLabel;
char travelPopulationSuffix[9];
char travelLandscapeLabels[TRAVEL_LANDSCAPE_LABEL_COUNT][9];
char travelInsufficientFundsText[0x29];
uint16_t reserveAmmoQuantitiesByFamily[CAMPAIGN_AMMO_FAMILY_COUNT];
uint16_t reserveAmmoFreightRates[CAMPAIGN_AMMO_FAMILY_COUNT];

/* Native stock generator policy and scratch; populated from the original image. */
uint8_t generatedKindRangeMaxima[STOCK_KIND_RANGE_COUNT];
uint8_t generatedKindRangeStarts[STOCK_KIND_RANGE_COUNT];
uint16_t generatedDamageMaximumIndex, generatedUnitKind;
uint16_t generatedAmmoKinds[CAMPAIGN_AMMO_GROUPS];
uint16_t generatedAmmoQuantities[CAMPAIGN_AMMO_GROUPS];
uint8_t generateStartingJenner;
uint8_t unitAmmoGroupCounts[CAMPAIGN_MECH_KIND_COUNT];
CampaignAmmoProfileEntry *unitAmmoProfiles[CAMPAIGN_MECH_KIND_COUNT];
CampaignAmmoProfileEntry unitAmmoProfileEntries[AMMO_PROFILE_ENTRY_COUNT];
uint8_t unitJumpJetGroupCounts[CAMPAIGN_MECH_KIND_COUNT];
uint16_t dealerUnitPurchaseLimit, unitsSoldSinceMarketRefresh;
uint16_t marketUnitCount, marketUnitKinds[CAMPAIGN_MARKET_UNIT_SLOTS];
uint16_t marketAmmoKindsByGroup[(int)CAMPAIGN_AMMO_GROUPS * CAMPAIGN_MARKET_UNIT_SLOTS];
uint16_t marketAmmoQuantitiesByGroup[(int)CAMPAIGN_AMMO_GROUPS * CAMPAIGN_MARKET_UNIT_SLOTS];
uint16_t availableRecruitCount, availableRecruitIds[RECRUIT_OFFER_SLOTS];
uint16_t ownedUnitCount, ownedUnitKinds[CAMPAIGN_OWNED_UNIT_SLOTS];
uint16_t houseFavorPoints[CAMPAIGN_HOUSE_COUNT], houseGrievancePoints[CAMPAIGN_HOUSE_COUNT];
uint16_t playerCombatCredit, generatingMissionOfferSlot;
uint16_t missionForceBudget, missionPaymentBaseThousands;
uint8_t missionForceWeightByUnitKind[CAMPAIGN_MECH_KIND_COUNT];
uint16_t housePaymentRollBias[CAMPAIGN_HOUSE_COUNT];
int16_t housePaymentAdjustmentPercent[CAMPAIGN_HOUSE_COUNT];
uint16_t houseSalvageBasePercent[CAMPAIGN_HOUSE_COUNT], houseAdvanceBasePercent[CAMPAIGN_HOUSE_COUNT];
uint8_t houseMissionTypeLimits[(int)CAMPAIGN_HOUSE_COUNT * MISSION_TARGET_COLUMNS];
uint8_t majorCampaignMissionRollThreshold;
uint16_t missionDescriptionOffsets[MISSION_OFFER_SLOTS], missionPlanetRecordIndices[MISSION_OFFER_SLOTS];
uint16_t missionTargetHouseIndices[MISSION_OFFER_SLOTS], missionTypeCodes[MISSION_OFFER_SLOTS];
uint16_t missionEnemyHeavyCounts[MISSION_OFFER_SLOTS], missionEnemyMediumCounts[MISSION_OFFER_SLOTS];
uint16_t missionEnemyLightCounts[MISSION_OFFER_SLOTS], missionPaymentThousands[MISSION_OFFER_SLOTS];
uint16_t missionSalvagePercentages[MISSION_OFFER_SLOTS], missionAdvancePercentages[MISSION_OFFER_SLOTS];
uint16_t missionPaymentCeilingsThousands[MISSION_OFFER_SLOTS], missionSalvagePercentageCeilings[MISSION_OFFER_SLOTS];
uint16_t missionAdvancePercentageCeilings[MISSION_OFFER_SLOTS], missionScenarioCodes[MISSION_OFFER_SLOTS];
uint16_t housePlanetRangeStarts[CAMPAIGN_HOUSE_COUNT], housePlanetRangeCounts[CAMPAIGN_HOUSE_COUNT];
uint8_t missionDescriptionMaximumIndicesByCategory[MISSION_CATEGORY_COUNT];
uint16_t missionDescriptionListOffsetsByCategory[MISSION_CATEGORY_COUNT];
uint16_t missionDescriptionListEntries[MISSION_DESCRIPTION_LIST_WORDS];
uint8_t missionDescriptionStorage[MISSION_DESCRIPTION_STORAGE_END - MISSION_DESCRIPTION_STORAGE_START];
uint16_t houseMissionPlanetTableOffsets[CAMPAIGN_HOUSE_COUNT];
MwHouseMissionPlanetPolicy houseMissionPlanetPolicies[CAMPAIGN_HOUSE_COUNT];
uint16_t calendarSubmonthUnits, missionOfferCount;
uint16_t marketRefreshUnits, elapsedBillingMonths;
uint16_t campaignFundsLow, campaignFundsHigh;
uint16_t dropShipMissedUpkeepPayments, unpaidPilotWageMarker;
uint16_t pilotWageAmounts[CAMPAIGN_ROSTER_SLOTS];
/* 0D49 can clear04BF+pilot42 =04E9, immediately after the identity union.
 * Do not extend that aligned union through04EA and duplicate pay-tier storage. */
uint8_t pilotDepartureFlagTail;
uint8_t notificationPending[CAMPAIGN_NOTIFICATION_COUNT];

/* Mech Complex data: one owner per object, no emulated DS or address registry. */
uint16_t unitInspectionContextCode;
uint16_t ownedUnitListSelectedRow;
uint16_t marketUnitListSelectedRow;
uint16_t mechServiceMenuSelectedRow;
MwPanelDescriptor ownedUnitListPanel;
MwOwnedUnitListRow ownedUnitListRows[0x000D];
uint16_t selectedOwnedUnitSlot;
uint16_t selectedOwnedUnitByteOffset;
uint16_t selectedMechAmmoFamilies[0x0003];
uint16_t selectedMechAmmoQuantities[0x0003];
uint8_t unitConditionSystemFieldCounts[0x000A];
uint8_t unitConditionScoreDivisors[0x000B];
uint8_t mechNonfunctionalTextBytes[0x000E];
uint8_t unitDisplayTonnages[0x000A];
uint8_t unitDisplaySpeedsKph[0x000A];
uint8_t unitDisplayJumpCapacitiesMetres[0x000A];
uint16_t unitDetailPictureXOffsets[0x000B];
uint16_t unitDetailPictureYOrigins[0x000B];
uint8_t repairWorkIncomplete;
MwPanelDescriptor mechStatusPanel;
MwPanelDescriptor dropShipStatusPanel;
/* Private backing arrays; shared access goes through the typed menus below. */
static MwUiRecord mechServiceUiRecords[0x0005];
static MwUiRecord dropShipServiceUiRecords[0x0002];
uint16_t ownedListRowUnitByteOffset;
uint16_t saleMarketSlotByteOffset;
uint16_t selectedMechRearmQuoteLow;
uint16_t selectedMechRearmQuoteHigh;
uint16_t mechServiceNextAnimationTick;
uint16_t mechServiceAnimationByteOffset;
uint16_t repairTitleLeadingSpacesByKind[0x000B];
uint16_t repairSystemRowIndex;
uint16_t repairMenuSelectedCategory;
MwPanelDescriptor componentRepairPanel;
MwComponentConditionTextRow repairSystemConditionRows[0x000A];
MwComponentConditionTextRow repairWeaponConditionRows[0x000B];
MwUiRecord componentRepairUiRecords[REPAIR_UI_RECORD_COUNT];
uint8_t unitHeatSinkCounts[0x000A];
uint8_t repairNoJumpJetsTextBytes[0x0005];
uint8_t repairJumpJetCountTemplateBytes[0x0009];
uint8_t repairSuccessRollLimitsByMarketTier[0x0004];
uint16_t mechDiagramUnitKind;
uint16_t bodyDamageTierFillColours[0x0004];
MwMechBodyDiagramLayout mechBodyDiagramLayouts[0x0008];
MwUiRecord ownedUnitListUiRecords[0x000D];
MwServiceCaption ownedUnitListHeading, mechStatusLabels[6], dropShipStatusLabels[3];
MwServiceCaption repairTitle, repairFooter;
MwBodyConditionText repairBodyCondition;
char shopUnitNames[CAMPAIGN_UNIT_KIND_COUNT][0x0F];
char unitDisplayNames[CAMPAIGN_UNIT_KIND_COUNT][0x0D];
char ammoFamilyDisplayNames[CAMPAIGN_AMMO_FAMILY_COUNT][0x0D];
char campaignDamageTierLabels[4][0x0D];
char unassignedMechWarriorText[0x0B], ownedListDoneText[0x18];
char cannotAffordRepairText[0x37], noRepairWorkText[0x1A];
char repairCompleteText[0x20], repairIncompleteText[0x1E], repairFailedText[0x17];
char repairWeaponLabelStorage[CAMPAIGN_MECH_KIND_COUNT][CAMPAIGN_WEAPON_SLOTS+1][0x0F];
char *repairWeaponLabels[CAMPAIGN_MECH_KIND_COUNT][CAMPAIGN_WEAPON_SLOTS+2];
uint8_t *mechDiagramBodyDamage;
MwMechDiagramRectangle *mechDiagramRectangle;
MwMechBodyDiagramLayout *mechBodyDiagramLayoutsByKind[CAMPAIGN_MECH_KIND_COUNT];

MwUiMenu planetHubMenu = { planetHubRecords, HUB_UI_RECORD_COUNT, &hubFirstNavigationProbe };
MwUiMenu contractHubMenu = { contractHubRecords, CONTRACT_HUB_UI_RECORD_COUNT, &hubSecondNavigationProbe };
MwUiMenu remotePlanetHubMenu = { remotePlanetHubRecords, HUB_UI_RECORD_COUNT, &hubThirdNavigationProbe };
/* FF probes alias362D (record15 X high),3B45 (kind9 failure8), and3B8B
 * (sensor price for kind8 low). Keep typed aliases, not host-endian BYTE casts. */
MwUiMenu ownedUnitListMenu = { ownedUnitListUiRecords, 0x0D, NULL, &componentRepairUiRecords[15].x, 8 };
MwUiMenu mechServiceMenu = { mechServiceUiRecords, 5, &repairAttemptFailuresByKind[9].failedByDamageIndex[8] };
MwUiMenu dropShipServiceMenu = { dropShipServiceUiRecords, 2, NULL, &systemRepairBasePrices[REPAIR_SENSORS].basePriceByMechKind[CAMPAIGN_UNIT_MARAUDER], 0 };
MwUiMenu componentRepairMenu = { componentRepairUiRecords, REPAIR_UI_RECORD_COUNT, &marketCapacityNavigationProbe };

/* Rearm/dealer objects are ordinary C data, independent of their disk encoding. */
MwPanelDescriptor rearmConfirmationPanel, saleConfirmationPanel;
MwPanelDescriptor marketUnitListPanel, marketMechStatusPanel, marketDropShipStatusPanel;
MwPanelDescriptor marketComponentPanel, mechComplexPanel;
MwServiceCaption rearmConfirmationLabels[3], saleConfirmationLabels[3];
MwServiceCaption marketListHeading, marketMechStatusLabels[4], marketDropShipStatusLabels[3];
MwServiceCaption mechComplexLabels[5], marketComponentTitle;
MwMarketUnitListRow marketUnitListRows[0x0D];
MwComponentConditionTextRow marketSystemConditionRows[0x0A], marketWeaponConditionRows[0x0A];
MwServiceCaption marketComponentDone;
MwServiceCaption marketWeaponHeading;
MwServiceCaption positionedText;
uint16_t hubSelectedRecordIndex;
MwBodyConditionText marketBodyCondition;
static MwUiRecord rearmConfirmationRecords[2], saleConfirmationRecords[2], marketUnitListRecords[0x0D];
static MwUiRecord marketMechRecords[3], marketDropShipRecords[2], marketComponentRecords[1], mechComplexRecords[4];
/* FF navigation probes alias the canonical caption/diagram/root-table owners;
 * they are read before the dispatcher rejects the FF navigation link. */
MwUiMenu rearmConfirmationMenu={rearmConfirmationRecords,2,NULL,&mechBodyDiagramLayouts[7].bodyRegions[7].x,8};
MwUiMenu saleConfirmationMenu={saleConfirmationRecords,2,NULL,&mechBodyDiagramLayouts[6].bodyRegions[1].width,0};
MwUiMenu marketUnitListMenu={marketUnitListRecords,0x0D,(const uint8_t *)&repairWeaponLabelStorage[CAMPAIGN_UNIT_SHADOW_HAWK][4][2]}; /*380D, DONE label byte. */
MwUiMenu marketMechMenu={marketMechRecords,3,NULL,&travelSquareRootValues[0x02DA],8}; /*529B, high BYTE. */
MwUiMenu marketDropShipMenu={marketDropShipRecords,2,NULL,&travelSquareRootValues[0x02EF],8}; /*52C5, high BYTE. */
MwUiMenu marketComponentMenu={marketComponentRecords,1,(const uint8_t *)&positionedText.text[0x10]}; /*5601, final space. */
MwUiMenu mechComplexMenu={mechComplexRecords,4,NULL,&repairWeaponConditionRows[1].display.y,0};
char ammoBayFullText[0x12], noMarketMechsText[0x2E];
char dealerPurchaseLimitText[0x33], purchaseInsufficientFundsText[0x17], ownedMechCapacityText[0x37];
char marketListDoneText[DEALER_LIST_NAME_COLUMNS];
uint16_t selectedMarketUnitSlot, selectedMarketUnitByteOffset;
uint16_t selectedMarketAmmoFamilies[CAMPAIGN_AMMO_GROUPS], selectedMarketAmmoQuantities[CAMPAIGN_AMMO_GROUPS];
uint16_t marketSystemRowIndex, purchaseOwnedSlotByteOffset;
uint16_t mechComplexAnimationFrames[MECH_COMPLEX_ANIMATION_FRAMES];

uint16_t campaignMusicBufferSelector, graphicsOverlaySelector;
uint16_t launcherMemoryTier;
uint8_t launcherExitCode, savedVideoMode;
uint8_t keyboardInterruptState;
uint16_t musicFileHandle;
char introNarrationLines[10][0x50];
MwIntroPositionedText introFirstLeftText,introFirstRightText;
MwIntroPositionedText introSecondLeftText,introSecondRightText;
MwIntroPositionedText introThirdLeftText,introThirdRightText;
MwIntroPositionedText introFourthLeftText,introFourthRightText;
MwIntroPositionedText introFifthLeftText,introFifthRightText;
MwIntroPositionedText introSixthLeftText,introSixthRightText;
MwIntroPositionedText introSeventhLeftText,introSeventhRightText;
uint8_t campaignTextInputBuffer[0x29];
uint16_t textInputCursorX,textInputCursorY,textInputMaximumIndex;
uint8_t textInputForeground,textInputBackground,textInputLastPhase;
uint16_t textInputLength;
uint16_t campaignEventPlanetSelectors[6];
uint8_t eventPlanetChoicePool0[8],eventPlanetChoicePool1[3];
uint8_t eventPlanetChoicePool2[4],eventPlanetChoicePool3[3];
uint8_t eventPlanetChoicePool4[4],eventPlanetChoicePool5[3];

MwCampaignEvent campaignEventRecords[CAMPAIGN_EVENT_RECORD_COUNT];
MwEventCommand campaignEventCommands[CAMPAIGN_EVENT_COMMAND_COUNT];
MwCampaignEvent *eventListEntries[CAMPAIGN_EVENT_LIST_ENTRIES];
MwCampaignEvent **eventLists[CAMPAIGN_EVENT_LIST_COUNT];
MwCampaignEvent *fixedCampaignEvent;
MwEventCaption eventChoiceCaptions[CAMPAIGN_EVENT_CAPTION_COUNT];
uint16_t eventDirectoryIndex,campaignEventClass,currentEventId,dispatchedEventCount;
uint8_t eventSeenFlags[CAMPAIGN_EVENT_SEEN_FLAGS];
uint8_t eventConditionResult,eventCombineWithAnd;
uint16_t eventScheduledMonthScratch,eventExpirySubmonthScratch,eventScheduledYearScratch;
uint8_t planetArrivalEventsPending,darkWingBaseVictoryRecorded,campaignRestartPending;
uint8_t eventChoiceCount,eventTextRowCount;
MwCampaignEvent *eventChoiceBranches[3];
uint16_t eventChoiceSavedFontIndex;
MwUiRecord eventTwoChoiceRecords[2],eventThreeChoiceRecords[3];
/* FF-link probes alias existing planet text bytes, NOT new globals at6C0E/2A. */
MwUiMenu eventTwoChoiceMenu={eventTwoChoiceRecords,2,&planetRecordStorage[0x6C0E -PLANET_RECORDS_START],NULL,0};
MwUiMenu eventThreeChoiceMenu={eventThreeChoiceRecords,3,&planetRecordStorage[0x6C2A-PLANET_RECORDS_START],NULL,0};
MwEventClassHandler eventClassHandlers[7]={MwShowCampaignEventText,MwShowCampaignEventText,
    MwShowCampaignEventText,MwShowClassThreeEventPages,MwShowCampaignEventText,
    MwShowCampaignEventText,MwShowCampaignEventText};
const MwEventCommand *eventOperandCursor;

uint8_t unidentifiedField18,missionCategoryOrMemoryTier,deploymentLandscapeIndex;
uint8_t combatEnemyHeavyCount,combatEnemyMediumCount,combatEnemyLightCount;
uint8_t combatPlanetSelector,combatScenarioCode,deploymentTerrainDetailLevel;
CampaignSalvageEntry salvageEntries[4];
uint16_t returnedDisabledEnemyCount;
uint8_t combatTransitionResult,acceptedContractActive;
uint8_t unidentifiedField19[2];
uint16_t commanderAgeYears,houseNetStandings[CAMPAIGN_HOUSE_COUNT];
uint16_t requestedContractPaymentThousands,requestedContractSalvagePercent;
uint16_t requestedContractAdvancePercent,requestContractPaymentEnabled;
uint16_t requestContractSalvageEnabled,requestContractAdvanceEnabled;
uint8_t unidentifiedField20[6];
uint16_t acceptedContractPaymentThousands,acceptedContractSalvagePercent;
uint16_t acceptedContractAdvancePercent,remainingContractPaymentThousands;
uint8_t audioOptionLabelIndex,campaignTerrainDetailLevel;
uint16_t acceptedContractTargetHouseIndex,playerSkillProgress;
uint8_t defaultHouseMissionTypeLimits[0x28],campaignDefaultCommanderNameBytes[0x0A];
uint16_t companyMenuSelectedRow,barMenuSelectedRow,optionsMenuSelection;

/* Installation data import. Decode once into the canonical C objects;
 * the input is never retained. These are file-format operations, not new
 * native game methods. Pointer topology is the reviewed shipped revision. */
/* Month strings end at4B19.4B1A onward belongs to contract date/editor data. */
static uint8_t calendarMonthText[0x6E];
static void ImportInitialWords(const uint8_t *bytes, uint16_t offset, uint16_t *words, size_t count)
{
    for (size_t i=0;i<count;++i) words[i]=MwReadNativeWord(bytes,(uint16_t)(offset+i*2));
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
static uint8_t mainWords[NATIVE_SEGMENT_BYTE_COUNT];
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
    if ((size_t)offset+capacity>NATIVE_SEGMENT_BYTE_COUNT) return false;
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
    owners[count++]={"travel.caption.search",&travelHouseSearchCaption.display,travelHouseSearchCaption.text,sizeof(travelHouseSearchCaption.text),0,0};
    for (uint16_t row=0;row<TRAVEL_PLANET_ROWS_PER_HOUSE;++row)
        owners[count++]={planetIds[row],&travelHousePlanetRows[row].display,travelHousePlanetRows[row].text,sizeof(travelHousePlanetRows[row].text),0,0};
    owners[count++]={"travel.caption.planets_back",&travelHousePlanetCancelLabel.display,travelHousePlanetCancelLabel.text,sizeof(travelHousePlanetCancelLabel.text),0,0};
    for (uint16_t line=0;line<TRAVEL_HOUSE_BROWSER_LINE_COUNT;++line)
        owners[count++]={browserIds[line],&travelHouseBrowserLabels[line].display,travelHouseBrowserLabels[line].text,sizeof(travelHouseBrowserLabels[line].text),0,0};
    owners[count++]={"travel.caption.destination_name",&travelDestinationHeaderLabels[0].display,travelDestinationHeaderLabels[0].text,sizeof(travelDestinationHeaderLabels[0].text),0,0};
    owners[count++]={"travel.caption.destination_detail",&travelDestinationHeaderLabels[1].display,travelDestinationHeaderLabels[1].text,sizeof(travelDestinationHeaderLabels[1].text),0,0};
    owners[count++]={"travel.caption.confirm_travel",&travelCostLabel.display,travelCostLabel.text,sizeof(travelCostLabel.text),0,0};
    owners[count++]={"travel.caption.leave",&travelConfirmLabel.display,travelConfirmLabel.text,sizeof(travelConfirmLabel.text),0,0};
    owners[count++]={"travel.caption.browse_houses",&travelBrowseLabel.display,travelBrowseLabel.text,sizeof(travelBrowseLabel.text),0,0};
    if (!MwImportUiCaptions(owners,count)) return false;
    /* DS:5C4E is one buffer in the original: 081B:5FEF copies the destination
     * name into it, the header panel's first caption draws it (5C49, colour at
     * 5C4D) and travel record 0 points at it, so the selection redraws the
     * name in the highlight colour. All three use travelDestinationName, so
     * the caption is pointed at it before the menu takes its text. */
    travelDestinationHeaderLabels[0].display.text=travelDestinationName;
    if (!MwImportUiPanel("travel.panel.house_planets",&travelHousePlanetPanel,TRAVEL_HOUSE_PLANET_LINE_COUNT,owners,count)
        || !MwImportUiPanel("travel.panel.house_browser",&travelHouseBrowserPanel,TRAVEL_HOUSE_BROWSER_LINE_COUNT,owners,count)
        || !MwImportUiPanel("travel.panel.cost",&travelCostPanel,1,owners,count)
        || !MwImportUiPanel("travel.panel.confirm",&travelConfirmPanel,1,owners,count)
        || !MwImportUiPanel("travel.panel.destination",&travelDestinationHeaderPanel,2,owners,count)
        || !MwImportUiPanel("travel.panel.browse",&travelBrowsePanel,1,owners,count)
        || !MwImportUiMenu("travel.menu.house_planets",&travelHousePlanetMenu,owners,count)
        || !MwImportUiMenu("travel.menu.house_browser",&travelHouseBrowserMenu,owners,count)
        || !MwImportUiMenu("travel.menu.map",&travelMapMenu,owners,count)) return false;
    /* The houses' search names (travel.word.house_search.*), by travel.json's house_search_words (59BB). */
    uint16_t houseSearchWords[TRAVEL_HOUSE_COUNT];
    if (!MwUiCopyWords("travel.table.house_search_words",houseSearchWords,TRAVEL_HOUSE_COUNT,0)) return false;
    for (uint16_t house=0;house<TRAVEL_HOUSE_COUNT;++house)
        if (!ImportInitialString(mainWords,houseSearchWords[house],travelHouseSearchNames[house],sizeof(travelHouseSearchNames[house]))) return false;
    if (!ImportInitialString(mainWords,0x5B83,travelPopulationSuffix,sizeof(travelPopulationSuffix))) return false;
    const uint16_t landscapeLabels[TRAVEL_LANDSCAPE_LABEL_COUNT]={0x5CA6,0x5CAD,0x5CB6};
    for (uint16_t label=0;label<TRAVEL_LANDSCAPE_LABEL_COUNT;++label)
        if (!ImportInitialString(mainWords,landscapeLabels[label],travelLandscapeLabels[label],sizeof(travelLandscapeLabels[label]))) return false;
    return true;
}
/* Native UI directories become typed references. Persistent pilot tables
 * remain parallel arrays; these objects do not invent a packed pilot record. */
MwPanelDescriptor companyMenuPanel,commanderProfilePanel,crewLabelsPanel,barMenuPanel,recruitInterviewPanel;
MwServiceCaption companyMenuLabels[4],commanderProfileLabels[11],crewLabels[7],barMenuLabels[4],recruitInterviewLabels[3];
char commanderProfileName[0x0A],commanderReputationLabels[COMMANDER_REPUTATION_COUNT][0x10];
char houseAttitudeLabels[5][0x0A],pilotSkillLabels[4][0x0A];
const char *houseAttitudeText[9];
char pilotAssignmentCaptions[PILOT_ASSIGNMENT_CAPTION_COUNT][PILOT_ASSIGNMENT_CAPTION_BYTES];
uint8_t pilotAssignmentCaptionColours[PILOT_ASSIGNMENT_CAPTION_COUNT];
uint16_t crewScreenRightEdge,pilotCardRosterSlot,pilotCardTextX,displayedPilotId;
uint16_t pilotCardGunneryTier,pilotCardPilotingTier,pilotCardPortraitId,pilotCardQuotedWageTier,pilotCardOwnedUnitSlot,pilotCardColumnOrigins[PILOT_CARD_COUNT];
uint16_t assignmentSelectedPilotSlot,assignmentCandidateOwnedSlot,assignmentConflictingPilotSlot,assignmentConflictingOwnedSlot;
uint8_t assignmentConflictHighlighted;
uint16_t recruitOfferSlot,selectedRecruitPilotId;
char recruitStatLabels[0x17],noAvailableRecruitsText[0x21],fullFourPilotRosterText[0x33],fullThreePilotRosterText[0x35];
const MwPilotDefinitionPrefix *pilotDefinitions[CAMPAIGN_PILOT_ID_COUNT];
const char *pilotDialogue[CAMPAIGN_PILOT_ID_COUNT];
static MwUiRecord companyRecords[3],crewRecords[5],barRecords[3],recruitInterviewRecords[2];
/*0960 reads FF-link probes before rejecting the link. Options and restore
 * now own the mutable bytes also probed by Bar and recruitment. */
static uint8_t companyNavigationPadding;
MwUiMenu companyMenu={companyRecords,3,&companyNavigationPadding};
MwUiMenu crewMenu={crewRecords,5,&rearmConfirmationRecords[1].textRecord};
MwUiMenu barMenu={barRecords,3,NULL};
MwUiMenu recruitInterviewMenu={recruitInterviewRecords,2,NULL,&saveListRows[4].display.x,0};
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
    memset(&campaignTimerInhibited,0,sizeof(campaignTimerInhibited)); /*0007*/
    memset(&campaignTimerByte,0,sizeof(campaignTimerByte)); /*0008*/
    memcpy(combatExecutableFilename,mwCombatExecutableFilename,sizeof(combatExecutableFilename)); /*0EF6*/
    memset(&deploymentResolvedWithoutCombat,0,sizeof(deploymentResolvedWithoutCombat)); /*140C*/
    if (!MwStoryCopyText("dark_wing.victory",darkWingVictoryText,sizeof(darkWingVictoryText))) return false;
    memcpy(afterActionCrewHeading,mainWords+0x11DF,sizeof(afterActionCrewHeading));
    memcpy(afterActionMechHeading,mainWords+0x11EC,sizeof(afterActionMechHeading));
    memcpy(afterActionSalvageHeading,mainWords+0x11F9,sizeof(afterActionSalvageHeading));
    memcpy(afterActionPaymentHeading,mainWords+0x1202,sizeof(afterActionPaymentHeading));
    memcpy(afterActionPercentSuffix,mainWords+0x120B,sizeof(afterActionPercentSuffix));
    memcpy(afterActionPilotStatusText,mainWords+0x120D,sizeof(afterActionPilotStatusText));
    afterActionRosterRowOffset=0; /* 1229: Written before it's read; zero where the exe reloaded it. */
    if (!ImportRuleWords("units.tonnages",(uint16_t *)campaignMechTonnages,sizeof(campaignMechTonnages)/sizeof(uint16_t))) return false;
    settlementPayoutPercent=0; /* Written before it's read; zero where the exe reloaded it. */
    if (!ImportRuleWords("houses.deduction_roll_thresholds",(uint16_t *)houseDeductionRollThresholds,sizeof(houseDeductionRollThresholds)/sizeof(uint16_t))) return false;
    if (!ImportRuleWords("houses.reduced_payout_percent",(uint16_t *)houseReducedPayoutPercentages,sizeof(houseReducedPayoutPercentages)/sizeof(uint16_t))) return false;
    if (!ImportRuleWords("contracts.deployment_units_by_scenario",(uint16_t *)missionDeploymentUnitsByScenario,sizeof(missionDeploymentUnitsByScenario)/sizeof(uint16_t))) return false;
    if (!MwStoryCopyText("deployment.uneventful",deploymentUneventfulText,sizeof(deploymentUneventfulText))) return false;
    if (!MwStoryCopyText("after_action.failure.kurita",afterActionKuritaFailure,sizeof(afterActionKuritaFailure))) return false;
    if (!MwStoryCopyText("after_action.failure.liao",afterActionLiaoFailure,sizeof(afterActionLiaoFailure))) return false;
    if (!MwStoryCopyText("after_action.failure.general",afterActionGeneralFailure,sizeof(afterActionGeneralFailure))) return false;
    if (!MwStoryCopyText("after_action.success.kurita",afterActionKuritaSuccess,sizeof(afterActionKuritaSuccess))) return false;
    if (!MwStoryCopyText("after_action.success.liao",afterActionLiaoSuccess,sizeof(afterActionLiaoSuccess))) return false;
    if (!MwStoryCopyText("after_action.success.general",afterActionGeneralSuccess,sizeof(afterActionGeneralSuccess))) return false;
    if (!MwStoryCopyText("settlement.deduction.supply",settlementSupplyDeductionText,sizeof(settlementSupplyDeductionText))) return false;
    if (!MwStoryCopyText("settlement.deduction.performance",settlementPerformanceDeductionText,sizeof(settlementPerformanceDeductionText))) return false;
    if (!MwStoryCopyText("settlement.deduction.transport",settlementTransportDeductionText,sizeof(settlementTransportDeductionText))) return false;
    if (!MwStoryCopyText("settlement.no_payment",settlementNoPaymentText,sizeof(settlementNoPaymentText))) return false;
    if (!MwStoryCopyText("dark_wing.attack_failed",darkWingAttackFailedText,sizeof(darkWingAttackFailedText))) return false;
    campaignTimerTicks=0; /*000C*/
    memset(&campaignTimerAlternatingPhase,0,sizeof(campaignTimerAlternatingPhase)); /*000E*/
    memset(&campaignTimerPhaseTicks,0,sizeof(campaignTimerPhaseTicks)); /*000F*/
    memset(&mouseInstalled,0,sizeof(mouseInstalled)); /*02AC*/
    memset(&joystickInstalled,0,sizeof(joystickInstalled)); /*02AD*/
    memset(&inputSinglePoll,0,sizeof(inputSinglePoll)); /*03E2*/
    joystickDirection=0; /*02E6*/
    joystickButtonOnePressed=0; /*02E8*/
    joystickButtonTwoPressed=0; /*02EA*/
    joystickPreviousButtons=MW_JOYSTICK_PREVIOUS_BUTTONS_START;
    joystickXCount=0; /*02ED*/
    joystickYCount=0; /*02EF*/
    joystickDeflection=MW_JOYSTICK_DEFLECTION_START;
    mouseMotionX=0; /*03DA*/
    mouseMotionY=0; /*03DC*/
    memset(&joystickRepeatActive,0,sizeof(joystickRepeatActive)); /*02F3*/
    joystickRepeatDeadline=0; /*02F4*/
    memcpy(joystickNavigationKeys,mwJoystickNavigationKeysStart,sizeof(joystickNavigationKeys)); /*02F6*/
    joystickCentreX=0; /*0308*/
    joystickCentreY=0; /*030A*/
    joystickXDeflectionStep=0; /*031C*/
    joystickYDeflectionStep=0; /*031E*/
    joystickXLow=0; /*030C*/
    joystickXHigh=0; /*030E*/
    joystickYLow=0; /*0310*/
    joystickYHigh=0; /*0312*/
    joystickXFarLow=0; /*0314*/
    joystickXFarHigh=0; /*0316*/
    joystickYFarLow=0; /*0318*/
    joystickYFarHigh=0; /*031A*/
    selectedUiRecordIndex=0; /*03E5*/
    uiSelectionChangedCodeOffset=0; /*03E7*/
    uiIdleCodeOffset=0; /*03E9*/
    /* The hub menus' past-end probes (story/native.json, src/Import/pieces.txt). */
    if (!MwStoryCopyBytes("probe.hub_planet",&hubFirstNavigationProbe,1)
        || !MwStoryCopyBytes("probe.hub_contract",&hubSecondNavigationProbe,1)
        || !MwStoryCopyBytes("probe.hub_remote_planet",&hubThirdNavigationProbe,1)) return false;
    panelOriginX=0; /*03EB*/
    panelOriginY=0; /*03ED*/
    panelWidth=0; /*03EF*/
    panelHeight=0; /*03F1*/
    panelBorderPictureBase=0; /*0444*/
    panelBorderEnabled=0; /*03F3*/
    textPanelInsetX=0; /*0432*/
    textPanelInsetY=0; /*0434*/
    textPanelOriginX=0; /*0436*/
    textPanelOriginY=0; /*0438*/
    textPanelWidthPixels=0; /*043A*/
    textPanelHeightPixels=0; /*043C*/
    textPanelPreviousFont=0; /*043E*/
    textPanelFont=0; /*0440*/
    textPanelContinuationMode=0; /*0442*/
    textPanelYOverride=0; /*0446*/
    /* The notification boxes (data/ui/hub.json): all insets x, then y, columns, rows (5EB3). */
    if (!MwUiCopyWords("hub.table.notification_boxes",notificationTextInsetX,4,0)
        || !MwUiCopyWords("hub.table.notification_boxes",notificationTextInsetY,4,8)
        || !MwUiCopyWords("hub.table.notification_boxes",notificationColumns,4,16)
        || !MwUiCopyWords("hub.table.notification_boxes",notificationRows,4,24)) return false;
    /* The notification texts (hub.word.notification.*) and the pointers to them (hub.json's notification_words). */
    if (!MwUiCopyWords("hub.table.notification_words",notificationTextOffsets,4,0)) return false; /*5ED3*/
    memcpy(notificationTextStorage,mainWords+0x5EDB,sizeof(notificationTextStorage));
    compressedPictureBufferSelector=0; /*016D*/
    /* 03F5, 03F7: the random generator's words as an EXEC starts them (data/rules/new_campaign.json "start"). */
    uint16_t randomSeed[2];
    if (!ImportRuleWords("new_campaign.start.random_seed",randomSeed,2)) return false;
    randomStateWord0=randomSeed[0];randomStateWord1=randomSeed[1];
    pictureCompressedByteCount=0; /*0F6D*/
    pictureOutputSelector=0; /*0F6F*/
    memset(&pictureReadHighNibbleNext,0,sizeof(pictureReadHighNibbleNext)); /*0F71*/
    pictureReadByteOffset=0; /*0F72*/
    memset(&pictureWriteLowNibbleNext,0,sizeof(pictureWriteLowNibbleNext)); /*0F74*/
    pictureWriteByteOffset=0; /*0F75*/
    indexedPictureOutputSelector=0; /*0161*/
    generalPictureBankSelector=0; /*0163*/
    cockpitPictureBankSelector=0; /*0165*/
    activePictureBankSelector=0; /*0167*/
    secondaryPictureBankSelector=0; /*0169*/
    memset(&indexedPictureDirectoryLoaded,0,sizeof(indexedPictureDirectoryLoaded)); /*0B62*/
    indexedPictureCachedIndex=MW_INDEXED_PICTURE_NO_CACHED_INDEX;
    indexedPictureStoredBytes=0; /*0B65*/
    memset(indexedPictureDirectory,0,sizeof(indexedPictureDirectory)); /*0B67*/
    indexedPictureFileHandle=0; /*0DBF*/
    generalPictureFileHandle=0; /*0DDA*/
    selectedPictureFileHandle=0; /*0DE9*/
    /* The planets from story/planets.json, laid out as the exe holds them
     * (offsets at 6570, storage 67B6.., the alphabetical table at 6694). */
    if (!MwStoryBuildPlanets(planetRecordStorage,sizeof(planetRecordStorage),PLANET_RECORDS_START,
        planetRecordOffsets,CAMPAIGN_PLANET_COUNT,alphabeticPlanetRecordOffsets,TRAVEL_PLANET_DIRECTORY_COUNT)) return false;
    if (!MwUiCopyWords("hub.table.calendar_month_name_widths",calendarMonthNameWidths,CAMPAIGN_MONTH_COUNT,0)) return false; /*11BA*/
    /* 117E..1183: the calendar's place and colours. Written before it's read; zero where the exe reloaded it. */
    calendarTextX=0;calendarTextY=0;calendarBackground=0;calendarForeground=0;
    memset(&numericLeadingDigitSeen,0,sizeof(numericLeadingDigitSeen)); /*A136*/
    memset(&numericGroupingEnabled,0,sizeof(numericGroupingEnabled)); /*A137*/
    memset(numericDecimalDigits,0,sizeof(numericDecimalDigits)); /*A138*/
    numericRemainderHigh=0; /*A142*/
    numericRemainderLow=0; /*A144*/
    memset(&numericSuppressLeadingZeros,0,sizeof(numericSuppressLeadingZeros)); /*A146*/
    memset(&numericNegativeSign,0,sizeof(numericNegativeSign)); /*A149*/
    numericFieldDigitCount=0; /*A147*/
    numericCursorX=0; /*A14A*/
    numericCursorY=0; /*A14C*/
    memset(&numericBackgroundColour,0,sizeof(numericBackgroundColour)); /*A14E*/
    memset(&numericForegroundColour,0,sizeof(numericForegroundColour)); /*A14F*/
    if (!ImportRuleBytes("houses.negotiation_attempt_base",houseNegotiationAttemptBase,sizeof(houseNegotiationAttemptBase))) return false;
    if (!ImportRuleWords("market.ammunition_prices",(uint16_t *)ammunitionBasePrices,sizeof(ammunitionBasePrices)/sizeof(uint16_t))) return false;
    if (!ImportRuleWords("market.start.ammunition_transaction_prices",(uint16_t *)ammunitionTransactionPrices,sizeof(ammunitionTransactionPrices)/sizeof(uint16_t))) return false;
    /* 3A77: zero in the exe, written in play; not a rule (docs/phase2/PLAN.md 5.1). */
    memset(repairAttemptFailuresByKind,0,sizeof(repairAttemptFailuresByKind));
    memset(&generatedMechDamage,0,sizeof(generatedMechDamage)); /*3A41*/
    if (!ImportRuleBytes("units.weapons",unitWeaponCounts,sizeof(unitWeaponCounts))) return false;
    if (!ImportRuleBytes("market.price_tier_multipliers",unitPriceTierMultipliers,sizeof(unitPriceTierMultipliers))) return false;
    if (!ImportRuleWords("units.base_prices",(uint16_t *)unitBasePrices,sizeof(unitBasePrices)/sizeof(uint16_t))) return false;
    unitTransactionPriceLow=0; /*305B*/
    unitTransactionPriceHigh=0; /*305D*/
    selectedOwnedUnitKind=0; /*2AD5*/
    selectedMarketUnitKind=0; /*4322*/
    unitRepairQuoteLow=0; /*31E9*/
    unitRepairQuoteHigh=0; /*31EB*/
    repairQuoteMechKind=0; /*31ED*/
    componentRepairPriceLow=0; /*320C*/
    componentRepairPriceHigh=0; /*320E*/
    repairPriceCategory=0; /*3210*/
    if (!ImportRuleWords("units.weapon_repair_prices",(uint16_t *)weaponRepairPrices,sizeof(weaponRepairPrices)/sizeof(uint16_t))) return false;
    if (!ImportRuleBytes("units.body_repair_prices",bodyRepairBasePricesByKind,sizeof(bodyRepairBasePricesByKind))) return false;
    /* The pilots from story/pilots.json (offsets at 9121, storage 9177..). */
    if (!MwStoryBuildPilots(pilotDefinitionStorage,sizeof(pilotDefinitionStorage),PILOT_DEFINITIONS_START,
        pilotDefinitionOffsets,CAMPAIGN_PILOT_ID_COUNT)) return false;
    MwBuildTravelSquareRoots(travelSquareRootValues,TRAVEL_ROOT_TABLE_COUNT); /*4CE6*/
    travelDestinationMapPositionX=0; /*54D0*/
    travelDestinationMapPositionY=0; /*54D2*/
    travelMapDeltaX=0; /*54D4*/
    travelMapDeltaY=0; /*54D6*/
    travelDestinationTimeWeight=0; /* Written before it's read; zero where the exe reloaded it. */
    travelDestinationPlanetOffset=0; /*5AD3*/
    travelDistanceUnits=0; /*5AD9*/
    travelQuoteCbillsLow=0; /*5AD5*/
    travelQuoteCbillsHigh=0; /*5AD7*/
    travelMapSelection=0; /*5ACF*/
    travelDestinationPlanetIndex=0; /*5AD1*/
    travelMarkerX=0; /*5ADB*/
    travelMarkerY=0; /*5ADD*/
    travelSavedMarkerX=0; /*5ADF*/
    travelSavedMarkerY=0; /*5AE1*/
    travelJoystickLastTick=0; /*5AE3*/
    travelAnimationFrameStep=0; /*5AE6*/
    memset(&travelDropShipSelected,0,sizeof(travelDropShipSelected)); /*5AE5*/
    hiredTransportReverseTerminator=MW_HIRED_TRANSPORT_REVERSE_END;
    if (!MwUiCopyWords("travel.table.hired_transport_frames",(uint16_t *)hiredTransportFrames,sizeof(hiredTransportFrames)/sizeof(uint16_t),0) /*5AEA*/
        || !MwUiCopyWords("travel.table.dropship_frames",(uint16_t *)dropShipTravelFrames,sizeof(dropShipTravelFrames)/sizeof(uint16_t),0)) return false; /*5B2E*/
    /* DS:5C4E: the destination header's caption text (the original's buffer runs past it into the next caption's head, bytes after the NUL that nothing draws). */
    if (!MwWordsCopyText("travel.caption.destination_name.text",travelDestinationName,sizeof(travelDestinationName))) return false;
    travelSelectedHouseByteOffset=0; /*54DC*/
    travelHousePlanetStart=0; /*54DE*/
    travelHousePlanetCount=0; /*54E2*/
    travelSelectedHouseRow=0; /*5A0B*/
    memcpy(travelPopulationSuffix,mainWords+0x5B83,sizeof(travelPopulationSuffix));
    memcpy(travelLandscapeLabels,mainWords+0x5CA6,sizeof(travelLandscapeLabels));
    memcpy(travelInsufficientFundsText,mainWords+0x5B5A,sizeof(travelInsufficientFundsText));
    if (!ImportRuleWords("market.freight_rates",(uint16_t *)reserveAmmoFreightRates,sizeof(reserveAmmoFreightRates)/sizeof(uint16_t))) return false;
    if (!ImportRuleBytes("market.stock_kind_range_maxima",generatedKindRangeMaxima,sizeof(generatedKindRangeMaxima))) return false;
    if (!ImportRuleBytes("market.stock_kind_range_starts",generatedKindRangeStarts,sizeof(generatedKindRangeStarts))) return false;
    generatedDamageMaximumIndex=0; /*3A3D*/
    generatedUnitKind=0; /*3A3F*/
    if (!ImportRuleWords("market.start.generated_ammo_kinds",(uint16_t *)generatedAmmoKinds,sizeof(generatedAmmoKinds)/sizeof(uint16_t))) return false;
    if (!ImportRuleWords("market.start.generated_ammo_quantities",(uint16_t *)generatedAmmoQuantities,sizeof(generatedAmmoQuantities)/sizeof(uint16_t))) return false;
    if (!ImportRuleBytes("market.start.generate_starting_jenner",&generateStartingJenner,sizeof(generateStartingJenner))) return false;
    if (!ImportRuleBytes("units.ammo_groups",unitAmmoGroupCounts,sizeof(unitAmmoGroupCounts))) return false;
    if (!ImportRuleBytes("units.jump_jet_groups",unitJumpJetGroupCounts,sizeof(unitJumpJetGroupCounts))) return false;
    generatingMissionOfferSlot=0; /*4B77*/
    missionForceBudget=0; /* Written before it's read; zero where the exe reloaded it. */
    missionPaymentBaseThousands=0; /* Written before it's read; zero where the exe reloaded it. */
    if (!ImportRuleBytes("units.force_weights",missionForceWeightByUnitKind,sizeof(missionForceWeightByUnitKind))) return false;
    if (!ImportRuleWords("houses.payment_roll_bias",(uint16_t *)housePaymentRollBias,sizeof(housePaymentRollBias)/sizeof(uint16_t))) return false;
    if (!ImportRuleWords("houses.payment_adjustment_percent",(uint16_t *)housePaymentAdjustmentPercent,sizeof(housePaymentAdjustmentPercent)/sizeof(uint16_t))) return false;
    if (!ImportRuleWords("houses.salvage_base_percent",(uint16_t *)houseSalvageBasePercent,sizeof(houseSalvageBasePercent)/sizeof(uint16_t))) return false;
    if (!ImportRuleWords("houses.advance_base_percent",(uint16_t *)houseAdvanceBasePercent,sizeof(houseAdvanceBasePercent)/sizeof(uint16_t))) return false;
    if (!ImportRuleWords("houses.planet_range_starts",(uint16_t *)housePlanetRangeStarts,sizeof(housePlanetRangeStarts)/sizeof(uint16_t))) return false;
    if (!ImportRuleWords("houses.planet_range_counts",(uint16_t *)housePlanetRangeCounts,sizeof(housePlanetRangeCounts)/sizeof(uint16_t))) return false;
    /* The mission descriptions from story/missions.json: the category tables
     * (8A16 maximum indices, 8A1B list offsets, 8A25 entries) and the storage
     * (8A69..). Saves keep entry offsets, so the original's text gives the
     * original's offsets. */
    if (!MwStoryBuildMissions(missionDescriptionStorage,sizeof(missionDescriptionStorage),MISSION_DESCRIPTION_STORAGE_START,
        missionDescriptionMaximumIndicesByCategory,missionDescriptionListOffsetsByCategory,MISSION_CATEGORY_COUNT,
        missionDescriptionListEntries,MISSION_DESCRIPTION_LIST_WORDS,MISSION_DESCRIPTION_LIST_START)) return false;
    if (!ImportRuleWords("houses.mission_planet_policy_offsets",(uint16_t *)houseMissionPlanetTableOffsets,sizeof(houseMissionPlanetTableOffsets)/sizeof(uint16_t))) return false;
    if (!ImportRuleBytes("houses.mission_planet_policies",houseMissionPlanetPolicies,sizeof(houseMissionPlanetPolicies))) return false;
    if (!ImportRuleWords("contracts.pilot_wages",(uint16_t *)pilotWageAmounts,sizeof(pilotWageAmounts)/sizeof(uint16_t))) return false;
    memset(notificationPending,0,sizeof(notificationPending)); /*5EAE*/
    unitInspectionContextCode=0; /*25BD*/
    ownedUnitListSelectedRow=0; /*25BF*/
    marketUnitListSelectedRow=0; /*25C1*/
    mechServiceMenuSelectedRow=0; /*25C3*/
    selectedOwnedUnitSlot=0; /*2AD1*/
    selectedOwnedUnitByteOffset=0; /*2AD3*/
    memset(selectedMechAmmoFamilies,0,sizeof(selectedMechAmmoFamilies)); /*2AD7*/
    memset(selectedMechAmmoQuantities,0,sizeof(selectedMechAmmoQuantities)); /*2ADD*/
    if (!ImportRuleBytes("units.condition_fields",unitConditionSystemFieldCounts,sizeof(unitConditionSystemFieldCounts))) return false;
    if (!ImportRuleBytes("units.condition_score_divisors",unitConditionScoreDivisors,sizeof(unitConditionScoreDivisors))) return false;
    memcpy(mechNonfunctionalTextBytes,mainWords+0x2B55,sizeof(mechNonfunctionalTextBytes));
    if (!ImportRuleBytes("units.display_tonnages",unitDisplayTonnages,sizeof(unitDisplayTonnages))) return false;
    if (!ImportRuleBytes("units.display_speeds",unitDisplaySpeedsKph,sizeof(unitDisplaySpeedsKph))) return false;
    if (!ImportRuleBytes("units.display_jumps",unitDisplayJumpCapacitiesMetres,sizeof(unitDisplayJumpCapacitiesMetres))) return false;
    if (!MwUiCopyWords("complex.table.detail_picture_x_offsets",(uint16_t *)unitDetailPictureXOffsets,sizeof(unitDetailPictureXOffsets)/sizeof(uint16_t),0) /*2B83*/
        || !MwUiCopyWords("complex.table.detail_picture_y_origins",(uint16_t *)unitDetailPictureYOrigins,sizeof(unitDetailPictureYOrigins)/sizeof(uint16_t),0)) return false; /*2B99*/
    memset(&repairWorkIncomplete,0,sizeof(repairWorkIncomplete)); /*2BAF*/
    ownedListRowUnitByteOffset=0; /*3022*/
    saleMarketSlotByteOffset=0; /*3055*/
    selectedMechRearmQuoteLow=0; /*310C*/
    selectedMechRearmQuoteHigh=0; /*310E*/
    mechServiceNextAnimationTick=0; /*3189*/
    mechServiceAnimationByteOffset=0; /*318B*/
    if (!MwUiCopyWords("complex.table.repair_title_leading_spaces",(uint16_t *)repairTitleLeadingSpacesByKind,sizeof(repairTitleLeadingSpacesByKind)/sizeof(uint16_t),0)) return false; /*31F2*/
    repairSystemRowIndex=0; /*3208*/
    repairMenuSelectedCategory=0; /*320A*/
    if (!ImportRuleBytes("units.heat_sinks",unitHeatSinkCounts,sizeof(unitHeatSinkCounts))) return false;
    memcpy(repairNoJumpJetsTextBytes,mainWords+0x36A0,sizeof(repairNoJumpJetsTextBytes));
    memcpy(repairJumpJetCountTemplateBytes,mainWords+0x36A5,sizeof(repairJumpJetCountTemplateBytes));
    if (!ImportRuleBytes("repair.success_roll_limits",repairSuccessRollLimitsByMarketTier,sizeof(repairSuccessRollLimitsByMarketTier))) return false;
    mechDiagramUnitKind=0; /*3CCC*/
    if (!MwUiCopyWords("complex.table.body_damage_fill_colours",(uint16_t *)bodyDamageTierFillColours,sizeof(bodyDamageTierFillColours)/sizeof(uint16_t),0)) return false; /*3CD2*/
    memcpy(unassignedMechWarriorText,mainWords+0x3024,sizeof(unassignedMechWarriorText));
    memcpy(ownedListDoneText,mainWords+0x302F,sizeof(ownedListDoneText));
    memcpy(cannotAffordRepairText,mainWords+0x2BB0,sizeof(cannotAffordRepairText));
    memcpy(noRepairWorkText,mainWords+0x2BE7,sizeof(noRepairWorkText));
    memcpy(repairCompleteText,mainWords+0x2C01,sizeof(repairCompleteText));
    memcpy(repairIncompleteText,mainWords+0x2C21,sizeof(repairIncompleteText));
    memcpy(repairFailedText,mainWords+0x3CB5,sizeof(repairFailedText));
    hubSelectedRecordIndex=0; /*1080*/
    memcpy(ammoBayFullText,mainWords+0x315B,sizeof(ammoBayFullText));
    memcpy(noMarketMechsText,mainWords+0x31BB,sizeof(noMarketMechsText));
    memcpy(dealerPurchaseLimitText,mainWords+0x258A,sizeof(dealerPurchaseLimitText));
    memcpy(purchaseInsufficientFundsText,mainWords+0x4330,sizeof(purchaseInsufficientFundsText));
    memcpy(ownedMechCapacityText,mainWords+0x4347,sizeof(ownedMechCapacityText));
    memcpy(marketListDoneText,mainWords+0x3049,sizeof(marketListDoneText));
    selectedMarketUnitSlot=0; /*431E*/
    selectedMarketUnitByteOffset=0; /*4320*/
    memset(selectedMarketAmmoFamilies,0,sizeof(selectedMarketAmmoFamilies)); /*4324*/
    memset(selectedMarketAmmoQuantities,0,sizeof(selectedMarketAmmoQuantities)); /*432A*/
    marketSystemRowIndex=0; /*44EF*/
    purchaseOwnedSlotByteOffset=0; /*481D*/
    if (!MwUiCopyWords("complex.table.animation_frames",(uint16_t *)mechComplexAnimationFrames,sizeof(mechComplexAnimationFrames)/sizeof(uint16_t),0)) return false; /*318D*/
    campaignMusicBufferSelector=0; /*016F*/
    graphicsOverlaySelector=0; /*002E*/
    memset(&launcherExitCode,0,sizeof(launcherExitCode)); /*0006*/
    memset(&keyboardInterruptState,0,sizeof(keyboardInterruptState)); /*02AB*/
    musicFileHandle=0; /*0E39*/
    memset(campaignTextInputBuffer,0,sizeof(campaignTextInputBuffer)); /*03A5*/
    textInputCursorX=0; /*03CE*/
    textInputCursorY=0; /*03D0*/
    textInputMaximumIndex=0; /*03D2*/
    memset(&textInputForeground,0,sizeof(textInputForeground)); /*03D4*/
    memset(&textInputBackground,0,sizeof(textInputBackground)); /*03D5*/
    memset(&textInputLastPhase,0,sizeof(textInputLastPhase)); /*03D8*/
    textInputLength=0; /*03D6*/
    {
        /* The story-planet pools from story/events.json (1090:8FDA..8FF3). */
        uint8_t *const pools[6]={eventPlanetChoicePool0,eventPlanetChoicePool1,eventPlanetChoicePool2,
            eventPlanetChoicePool3,eventPlanetChoicePool4,eventPlanetChoicePool5};
        const size_t sizes[6]={sizeof(eventPlanetChoicePool0),sizeof(eventPlanetChoicePool1),sizeof(eventPlanetChoicePool2),
            sizeof(eventPlanetChoicePool3),sizeof(eventPlanetChoicePool4),sizeof(eventPlanetChoicePool5)};
        if (!MwStoryCopyPlanetPools(pools,sizes,6)) return false;
    }
    campaignEventClass=0; /*5E62*/
    currentEventId=0; /*5E64*/
    dispatchedEventCount=0; /*5E68*/
    memset(&eventConditionResult,0,sizeof(eventConditionResult)); /*5E70*/
    memset(&eventCombineWithAnd,0,sizeof(eventCombineWithAnd)); /*5E71*/
    eventScheduledMonthScratch=0; /*5E72*/
    eventExpirySubmonthScratch=0; /*5E74*/
    eventScheduledYearScratch=0; /*5E76*/
    memset(&planetArrivalEventsPending,0,sizeof(planetArrivalEventsPending)); /*1082*/
    memset(&campaignRestartPending,0,sizeof(campaignRestartPending)); /*5EB2*/
    memset(&eventChoiceCount,0,sizeof(eventChoiceCount)); /*5CEC*/
    memset(&eventTextRowCount,0,sizeof(eventTextRowCount)); /*5CED*/
    eventChoiceSavedFontIndex=0; /*5E66*/
    if (!ImportRuleBytes("houses.mission_type_limits",defaultHouseMissionTypeLimits,sizeof(defaultHouseMissionTypeLimits))) return false;
    if (!MwStoryCopyText("commander.default_name",(char *)campaignDefaultCommanderNameBytes,sizeof(campaignDefaultCommanderNameBytes))) return false;
    companyMenuSelectedRow=0; /*21CC*/
    barMenuSelectedRow=0; /*8FF4*/
    optionsMenuSelection=0; /*9D50*/
    for (unsigned line=0;line<10;++line)
    {
        char id[24];snprintf(id,sizeof(id),"intro.narration.%u",line+1);
        if (!MwStoryCopyText(id,introNarrationLines[line],sizeof(introNarrationLines[line]))) return false;
    }
    if (!ImportIntroCaptionAt("intro.caption.1.left.at",&introFirstLeftText)) return false;
    if (!MwStoryCopyText("intro.caption.1.left",introFirstLeftText.text,sizeof(introFirstLeftText.text))) return false;
    if (!ImportIntroCaptionAt("intro.caption.1.right.at",&introFirstRightText)) return false;
    if (!MwStoryCopyText("intro.caption.1.right",introFirstRightText.text,sizeof(introFirstRightText.text))) return false;
    if (!ImportIntroCaptionAt("intro.caption.2.left.at",&introSecondLeftText)) return false;
    if (!MwStoryCopyText("intro.caption.2.left",introSecondLeftText.text,sizeof(introSecondLeftText.text))) return false;
    if (!ImportIntroCaptionAt("intro.caption.2.right.at",&introSecondRightText)) return false;
    if (!MwStoryCopyText("intro.caption.2.right",introSecondRightText.text,sizeof(introSecondRightText.text))) return false;
    if (!ImportIntroCaptionAt("intro.caption.3.left.at",&introThirdLeftText)) return false;
    if (!MwStoryCopyText("intro.caption.3.left",introThirdLeftText.text,sizeof(introThirdLeftText.text))) return false;
    if (!ImportIntroCaptionAt("intro.caption.3.right.at",&introThirdRightText)) return false;
    if (!MwStoryCopyText("intro.caption.3.right",introThirdRightText.text,sizeof(introThirdRightText.text))) return false;
    if (!ImportIntroCaptionAt("intro.caption.4.left.at",&introFourthLeftText)) return false;
    if (!MwStoryCopyText("intro.caption.4.left",introFourthLeftText.text,sizeof(introFourthLeftText.text))) return false;
    if (!ImportIntroCaptionAt("intro.caption.4.right.at",&introFourthRightText)) return false;
    if (!MwStoryCopyText("intro.caption.4.right",introFourthRightText.text,sizeof(introFourthRightText.text))) return false;
    if (!ImportIntroCaptionAt("intro.caption.5.left.at",&introFifthLeftText)) return false;
    if (!MwStoryCopyText("intro.caption.5.left",introFifthLeftText.text,sizeof(introFifthLeftText.text))) return false;
    if (!ImportIntroCaptionAt("intro.caption.5.right.at",&introFifthRightText)) return false;
    if (!MwStoryCopyText("intro.caption.5.right",introFifthRightText.text,sizeof(introFifthRightText.text))) return false;
    if (!ImportIntroCaptionAt("intro.caption.6.left.at",&introSixthLeftText)) return false;
    if (!MwStoryCopyText("intro.caption.6.left",introSixthLeftText.text,sizeof(introSixthLeftText.text))) return false;
    if (!ImportIntroCaptionAt("intro.caption.6.right.at",&introSixthRightText)) return false;
    if (!MwStoryCopyText("intro.caption.6.right",introSixthRightText.text,sizeof(introSixthRightText.text))) return false;
    if (!ImportIntroCaptionAt("intro.caption.7.left.at",&introSeventhLeftText)) return false;
    if (!MwStoryCopyText("intro.caption.7.left",introSeventhLeftText.text,sizeof(introSeventhLeftText.text))) return false;
    if (!ImportIntroCaptionAt("intro.caption.7.right.at",&introSeventhRightText)) return false;
    if (!MwStoryCopyText("intro.caption.7.right",introSeventhRightText.text,sizeof(introSeventhRightText.text))) return false;
    for (uint16_t row=0;row<CAMPAIGN_SYSTEM_COUNT;++row)
        if (!ImportRuleWords("repair.system_prices",systemRepairBasePrices[row].basePriceByMechKind,CAMPAIGN_MECH_KIND_COUNT,row*0x14)) return false;
    /* 2F3C, 2F28 and 3C1B come from the rules (data/rules/units.json). */
    uint8_t ammoProfileEntries[AMMO_PROFILE_ENTRY_COUNT*4],ammoProfiles[CAMPAIGN_MECH_KIND_COUNT*2];
    uint8_t weaponRepairPriceLists[CAMPAIGN_MECH_KIND_COUNT*2];
    if (!ImportRuleBytes("units.ammo_profile_entries",ammoProfileEntries,sizeof(ammoProfileEntries))
        || !ImportRuleBytes("units.ammo_profiles",ammoProfiles,sizeof(ammoProfiles))
        || !ImportRuleBytes("units.weapon_repair_price_lists",weaponRepairPriceLists,sizeof(weaponRepairPriceLists))) return false;
    for (uint16_t entry=0;entry<AMMO_PROFILE_ENTRY_COUNT;++entry)
    {
        unitAmmoProfileEntries[entry].ammoFamily=MwReadNativeWord(ammoProfileEntries,(uint16_t)(entry*4));
        unitAmmoProfileEntries[entry].quantityMaximum=MwReadNativeWord(ammoProfileEntries,(uint16_t)(entry*4+2));
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
        uint16_t profile=MwReadNativeWord(ammoProfiles,(uint16_t)(kind*2));
        uint16_t prices=MwReadNativeWord(weaponRepairPriceLists,(uint16_t)(kind*2));
        uint16_t layout=diagramByKind[kind];
        if (profile<0x2F3C || profile>=0x2F78 || (profile-0x2F3C)%4
            || prices<0x3C2F || prices>=0x3CAB || (prices-0x3C2F)%2
            || (layout && (layout<diagramBase || layout>=diagramBase+8*0x50 || (layout-diagramBase)%0x50))) return false;
        unitAmmoProfiles[kind]=&unitAmmoProfileEntries[(profile-0x2F3C)/4];
        weaponRepairPricesByKind[kind]=&weaponRepairPrices[(prices-0x3C2F)/2];
        mechBodyDiagramLayoutsByKind[kind]=layout?&mechBodyDiagramLayouts[(layout-diagramBase)/0x50]:NULL;
        uint16_t labelOffset=repairWeaponWords[kind],label=0;
        while (mainWords[labelOffset]!=0xFF && label<CAMPAIGN_WEAPON_SLOTS+1)
        {
            if (!ImportInitialString(mainWords,labelOffset,repairWeaponLabelStorage[kind][label],0x0F)) return false;
            repairWeaponLabels[kind][label]=repairWeaponLabelStorage[kind][label];
            labelOffset+=(uint16_t)(strlen(repairWeaponLabels[kind][label])+1);++label;
        }
        repairWeaponLabels[kind][label]=NULL;
    }
    for (uint16_t i=0;i<CAMPAIGN_UNIT_KIND_COUNT;++i)
    {
        char id[40];
        snprintf(id,sizeof(id),"unit.%s.shop_name",storyUnitKeys[i]);
        if (!MwStoryCopyText(id,shopUnitNames[i],sizeof(shopUnitNames[i]))) return false;
        snprintf(id,sizeof(id),"unit.%s.name",storyUnitKeys[i]);
        if (!MwStoryCopyText(id,unitDisplayNames[i],sizeof(unitDisplayNames[i]))) return false;
    }
    if (!ImportReserveAmmoUiData()) return false;
    /* The ammo family names and damage tier labels (complex.word.*), by complex.json's ammo_family_words (2F78) and damage_tier_words (2B23). */
    uint16_t ammoFamilyWords[0x06],damageTierWords[0x04];
    if (!MwUiCopyWords("complex.table.ammo_family_words",ammoFamilyWords,0x06,0)
        || !MwUiCopyWords("complex.table.damage_tier_words",damageTierWords,0x04,0)) return false;
    for (uint16_t i=0;i<0x06;++i)
        if (!ImportInitialString(mainWords,ammoFamilyWords[i],ammoFamilyDisplayNames[i],sizeof(ammoFamilyDisplayNames[i]))) return false;
    for (uint16_t i=0;i<0x04;++i)
        if (!ImportInitialString(mainWords,damageTierWords[i],campaignDamageTierLabels[i],sizeof(campaignDamageTierLabels[i]))) return false;
    /* The month names (hub.word.month.*) and the pointers to them (hub.json's month_words, 4A94). */
    memcpy(calendarMonthText,mainWords+0x4AAC,sizeof(calendarMonthText));
    uint16_t monthWords[CAMPAIGN_MONTH_COUNT];
    if (!MwUiCopyWords("hub.table.month_words",monthWords,CAMPAIGN_MONTH_COUNT,0)) return false;
    for (uint16_t month=0;month<CAMPAIGN_MONTH_COUNT;++month)
    {
        uint16_t text=monthWords[month];
        if (text<0x4AAC || text>=0x4B1A || !memchr(mainWords+text,0,0x4B1A-text)) return false;
        calendarMonthNames[month]=calendarMonthText+text-0x4AAC;
    }
    /* The eighth planet row's caption (travel.caption.planet_8, 55EC): its head from the layout bytes, its 18-byte text from the words. */
    uint16_t positionedAt[2];
    if (!MwUiCopyWords("travel.caption.planet_8",positionedAt,2,0)
        || !MwUiCopyBytes("travel.caption.planet_8",&positionedText.display.colour,1,4)) return false;
    positionedText.display.x=positionedAt[0];positionedText.display.y=positionedAt[1];
    positionedText.display.text=positionedText.text;
    if (!MwWordsCopyBlock("travel.caption.planet_8.text",(uint8_t *)positionedText.text,18)) return false;
    /* data/ui/complex.json: the Mech Complex (panel 25C5, menu 2632), the owned
     * list (266A, 283B), 'Mech and DropShip service (2C3F, 2D53; 2D06, 2D99),
     * sale (3075, 30F0), rearm (3110, 316D) and component repair (3214, 3558).
     * Rows the code fills in keep their fixed-width templates. */
    UiCaptionOwner complexOwners[]={
        {"complex.caption.complex_title",&mechComplexLabels[0].display,mechComplexLabels[0].text,sizeof(mechComplexLabels[0].text),0,0},
        {"complex.caption.complex_owned",&mechComplexLabels[1].display,mechComplexLabels[1].text,sizeof(mechComplexLabels[1].text),0,0},
        {"complex.caption.complex_ammo",&mechComplexLabels[2].display,mechComplexLabels[2].text,sizeof(mechComplexLabels[2].text),0,0},
        {"complex.caption.complex_market",&mechComplexLabels[3].display,mechComplexLabels[3].text,sizeof(mechComplexLabels[3].text),0,0},
        {"complex.caption.complex_leave",&mechComplexLabels[4].display,mechComplexLabels[4].text,sizeof(mechComplexLabels[4].text),0,0},
        {"complex.caption.owned_heading",&ownedUnitListHeading.display,ownedUnitListHeading.text,sizeof(ownedUnitListHeading.text),0,26},
        {"complex.caption.owned_1",&ownedUnitListRows[0].display,ownedUnitListRows[0].caption.text,sizeof(ownedUnitListRows[0].caption.text),0,25},
        {"complex.caption.owned_2",&ownedUnitListRows[1].display,ownedUnitListRows[1].caption.text,sizeof(ownedUnitListRows[1].caption.text),0,25},
        {"complex.caption.owned_3",&ownedUnitListRows[2].display,ownedUnitListRows[2].caption.text,sizeof(ownedUnitListRows[2].caption.text),0,25},
        {"complex.caption.owned_4",&ownedUnitListRows[3].display,ownedUnitListRows[3].caption.text,sizeof(ownedUnitListRows[3].caption.text),0,25},
        {"complex.caption.owned_5",&ownedUnitListRows[4].display,ownedUnitListRows[4].caption.text,sizeof(ownedUnitListRows[4].caption.text),0,25},
        {"complex.caption.owned_6",&ownedUnitListRows[5].display,ownedUnitListRows[5].caption.text,sizeof(ownedUnitListRows[5].caption.text),0,25},
        {"complex.caption.owned_7",&ownedUnitListRows[6].display,ownedUnitListRows[6].caption.text,sizeof(ownedUnitListRows[6].caption.text),0,25},
        {"complex.caption.owned_8",&ownedUnitListRows[7].display,ownedUnitListRows[7].caption.text,sizeof(ownedUnitListRows[7].caption.text),0,25},
        {"complex.caption.owned_9",&ownedUnitListRows[8].display,ownedUnitListRows[8].caption.text,sizeof(ownedUnitListRows[8].caption.text),0,25},
        {"complex.caption.owned_10",&ownedUnitListRows[9].display,ownedUnitListRows[9].caption.text,sizeof(ownedUnitListRows[9].caption.text),0,25},
        {"complex.caption.owned_11",&ownedUnitListRows[10].display,ownedUnitListRows[10].caption.text,sizeof(ownedUnitListRows[10].caption.text),0,25},
        {"complex.caption.owned_12",&ownedUnitListRows[11].display,ownedUnitListRows[11].caption.text,sizeof(ownedUnitListRows[11].caption.text),0,25},
        {"complex.caption.owned_13",&ownedUnitListRows[12].display,ownedUnitListRows[12].caption.text,sizeof(ownedUnitListRows[12].caption.text),0,25},
        {"complex.caption.mech_status",&mechStatusLabels[0].display,mechStatusLabels[0].text,sizeof(mechStatusLabels[0].text),0,106},
        {"complex.caption.service_repair",&mechStatusLabels[1].display,mechStatusLabels[1].text,sizeof(mechStatusLabels[1].text),0,7},
        {"complex.caption.service_repair_all",&mechStatusLabels[2].display,mechStatusLabels[2].text,sizeof(mechStatusLabels[2].text),0,11},
        {"complex.caption.service_rearm",&mechStatusLabels[3].display,mechStatusLabels[3].text,sizeof(mechStatusLabels[3].text),0,7},
        {"complex.caption.service_sell",&mechStatusLabels[4].display,mechStatusLabels[4].text,sizeof(mechStatusLabels[4].text),0,5},
        {"complex.caption.service_leave",&mechStatusLabels[5].display,mechStatusLabels[5].text,sizeof(mechStatusLabels[5].text),0,5},
        {"complex.caption.dropship_status",&dropShipStatusLabels[0].display,dropShipStatusLabels[0].text,sizeof(dropShipStatusLabels[0].text),0,30},
        {"complex.caption.dropship_sell",&dropShipStatusLabels[1].display,dropShipStatusLabels[1].text,sizeof(dropShipStatusLabels[1].text),0,5},
        {"complex.caption.dropship_leave",&dropShipStatusLabels[2].display,dropShipStatusLabels[2].text,sizeof(dropShipStatusLabels[2].text),0,5},
        {"complex.caption.sale_prompt",&saleConfirmationLabels[0].display,saleConfirmationLabels[0].text,sizeof(saleConfirmationLabels[0].text),0,0},
        {"complex.caption.sale_yes",&saleConfirmationLabels[1].display,saleConfirmationLabels[1].text,sizeof(saleConfirmationLabels[1].text),0,0},
        {"complex.caption.sale_no",&saleConfirmationLabels[2].display,saleConfirmationLabels[2].text,sizeof(saleConfirmationLabels[2].text),0,0},
        {"complex.caption.rearm_prompt",&rearmConfirmationLabels[0].display,rearmConfirmationLabels[0].text,sizeof(rearmConfirmationLabels[0].text),0,0},
        {"complex.caption.rearm_yes",&rearmConfirmationLabels[1].display,rearmConfirmationLabels[1].text,sizeof(rearmConfirmationLabels[1].text),0,0},
        {"complex.caption.rearm_no",&rearmConfirmationLabels[2].display,rearmConfirmationLabels[2].text,sizeof(rearmConfirmationLabels[2].text),0,0},
        {"complex.caption.repair_title",&repairTitle.display,repairTitle.text,sizeof(repairTitle.text),0,25},
        {"complex.caption.repair_system_1",&repairSystemConditionRows[0].display,repairSystemConditionRows[0].caption.text,sizeof(repairSystemConditionRows[0].caption.text),0,26},
        {"complex.caption.repair_system_2",&repairSystemConditionRows[1].display,repairSystemConditionRows[1].caption.text,sizeof(repairSystemConditionRows[1].caption.text),0,26},
        {"complex.caption.repair_system_3",&repairSystemConditionRows[2].display,repairSystemConditionRows[2].caption.text,sizeof(repairSystemConditionRows[2].caption.text),0,26},
        {"complex.caption.repair_system_4",&repairSystemConditionRows[3].display,repairSystemConditionRows[3].caption.text,sizeof(repairSystemConditionRows[3].caption.text),0,26},
        {"complex.caption.repair_system_5",&repairSystemConditionRows[4].display,repairSystemConditionRows[4].caption.text,sizeof(repairSystemConditionRows[4].caption.text),0,26},
        {"complex.caption.repair_system_6",&repairSystemConditionRows[5].display,repairSystemConditionRows[5].caption.text,sizeof(repairSystemConditionRows[5].caption.text),0,26},
        {"complex.caption.repair_system_7",&repairSystemConditionRows[6].display,repairSystemConditionRows[6].caption.text,sizeof(repairSystemConditionRows[6].caption.text),0,26},
        {"complex.caption.repair_system_8",&repairSystemConditionRows[7].display,repairSystemConditionRows[7].caption.text,sizeof(repairSystemConditionRows[7].caption.text),0,26},
        {"complex.caption.repair_system_9",&repairSystemConditionRows[8].display,repairSystemConditionRows[8].caption.text,sizeof(repairSystemConditionRows[8].caption.text),0,26},
        {"complex.caption.repair_system_10",&repairSystemConditionRows[9].display,repairSystemConditionRows[9].caption.text,sizeof(repairSystemConditionRows[9].caption.text),0,26},
        {"complex.caption.repair_body",&repairBodyCondition.display,repairBodyCondition.caption.text,sizeof(repairBodyCondition.caption.text),0,19},
        {"complex.caption.repair_footer",&repairFooter.display,repairFooter.text,sizeof(repairFooter.text),0,62},
        {"complex.caption.repair_weapon_1",&repairWeaponConditionRows[0].display,repairWeaponConditionRows[0].caption.text,sizeof(repairWeaponConditionRows[0].caption.text),0,26},
        {"complex.caption.repair_weapon_2",&repairWeaponConditionRows[1].display,repairWeaponConditionRows[1].caption.text,sizeof(repairWeaponConditionRows[1].caption.text),0,26},
        {"complex.caption.repair_weapon_3",&repairWeaponConditionRows[2].display,repairWeaponConditionRows[2].caption.text,sizeof(repairWeaponConditionRows[2].caption.text),0,26},
        {"complex.caption.repair_weapon_4",&repairWeaponConditionRows[3].display,repairWeaponConditionRows[3].caption.text,sizeof(repairWeaponConditionRows[3].caption.text),0,26},
        {"complex.caption.repair_weapon_5",&repairWeaponConditionRows[4].display,repairWeaponConditionRows[4].caption.text,sizeof(repairWeaponConditionRows[4].caption.text),0,26},
        {"complex.caption.repair_weapon_6",&repairWeaponConditionRows[5].display,repairWeaponConditionRows[5].caption.text,sizeof(repairWeaponConditionRows[5].caption.text),0,26},
        {"complex.caption.repair_weapon_7",&repairWeaponConditionRows[6].display,repairWeaponConditionRows[6].caption.text,sizeof(repairWeaponConditionRows[6].caption.text),0,26},
        {"complex.caption.repair_weapon_8",&repairWeaponConditionRows[7].display,repairWeaponConditionRows[7].caption.text,sizeof(repairWeaponConditionRows[7].caption.text),0,26},
        {"complex.caption.repair_weapon_9",&repairWeaponConditionRows[8].display,repairWeaponConditionRows[8].caption.text,sizeof(repairWeaponConditionRows[8].caption.text),0,26},
        {"complex.caption.repair_weapon_10",&repairWeaponConditionRows[9].display,repairWeaponConditionRows[9].caption.text,sizeof(repairWeaponConditionRows[9].caption.text),0,26},
        {"complex.caption.repair_weapon_11",&repairWeaponConditionRows[10].display,repairWeaponConditionRows[10].caption.text,sizeof(repairWeaponConditionRows[10].caption.text),0,26}};
    const size_t complexCount=sizeof(complexOwners)/sizeof(complexOwners[0]);
    if (!MwImportUiCaptions(complexOwners,complexCount)
        || !MwImportUiPanel("complex.panel.complex",&mechComplexPanel,5,complexOwners,complexCount)
        || !MwImportUiListPanel("complex.panel.owned",&ownedUnitListPanel,14,complexOwners,complexCount)
        || !MwImportUiPanel("complex.panel.mech_status",&mechStatusPanel,6,complexOwners,complexCount)
        || !MwImportUiPanel("complex.panel.dropship_status",&dropShipStatusPanel,3,complexOwners,complexCount)
        || !MwImportUiPanel("complex.panel.sale",&saleConfirmationPanel,3,complexOwners,complexCount)
        || !MwImportUiPanel("complex.panel.rearm",&rearmConfirmationPanel,3,complexOwners,complexCount)
        || !MwImportUiListPanel("complex.panel.repair",&componentRepairPanel,24,complexOwners,complexCount)
        || !MwImportUiMenu("complex.menu.complex",&mechComplexMenu,complexOwners,complexCount)
        || !MwImportUiMenu("complex.menu.owned",&ownedUnitListMenu,complexOwners,complexCount)
        || !MwImportUiMenu("complex.menu.mech_service",&mechServiceMenu,complexOwners,complexCount)
        || !MwImportUiMenu("complex.menu.dropship_service",&dropShipServiceMenu,complexOwners,complexCount)
        || !MwImportUiMenu("complex.menu.sale",&saleConfirmationMenu,complexOwners,complexCount)
        || !MwImportUiMenu("complex.menu.rearm",&rearmConfirmationMenu,complexOwners,complexCount)
        || !MwImportUiMenu("complex.menu.repair",&componentRepairMenu,complexOwners,complexCount)) return false;
    /* data/ui/dealer.json: the market list (28F1, 2A1B), the 'Mech and
     * DropShip panels (437E, 44A9; 443E, 44D3) and the component panel (44F1,
     * 480F). Reserve ammunition is ImportReserveAmmoUiData's. */
    UiCaptionOwner dealerOwners[]={
        {"dealer.caption.market_heading",&marketListHeading.display,marketListHeading.text,sizeof(marketListHeading.text),0,0},
        {"dealer.caption.market_1",&marketUnitListRows[0].display,marketUnitListRows[0].text,sizeof(marketUnitListRows[0].text),0,13},
        {"dealer.caption.market_2",&marketUnitListRows[1].display,marketUnitListRows[1].text,sizeof(marketUnitListRows[1].text),0,13},
        {"dealer.caption.market_3",&marketUnitListRows[2].display,marketUnitListRows[2].text,sizeof(marketUnitListRows[2].text),0,13},
        {"dealer.caption.market_4",&marketUnitListRows[3].display,marketUnitListRows[3].text,sizeof(marketUnitListRows[3].text),0,13},
        {"dealer.caption.market_5",&marketUnitListRows[4].display,marketUnitListRows[4].text,sizeof(marketUnitListRows[4].text),0,13},
        {"dealer.caption.market_6",&marketUnitListRows[5].display,marketUnitListRows[5].text,sizeof(marketUnitListRows[5].text),0,13},
        {"dealer.caption.market_7",&marketUnitListRows[6].display,marketUnitListRows[6].text,sizeof(marketUnitListRows[6].text),0,13},
        {"dealer.caption.market_8",&marketUnitListRows[7].display,marketUnitListRows[7].text,sizeof(marketUnitListRows[7].text),0,13},
        {"dealer.caption.market_9",&marketUnitListRows[8].display,marketUnitListRows[8].text,sizeof(marketUnitListRows[8].text),0,13},
        {"dealer.caption.market_10",&marketUnitListRows[9].display,marketUnitListRows[9].text,sizeof(marketUnitListRows[9].text),0,13},
        {"dealer.caption.market_11",&marketUnitListRows[10].display,marketUnitListRows[10].text,sizeof(marketUnitListRows[10].text),0,13},
        {"dealer.caption.market_12",&marketUnitListRows[11].display,marketUnitListRows[11].text,sizeof(marketUnitListRows[11].text),0,13},
        {"dealer.caption.market_13",&marketUnitListRows[12].display,marketUnitListRows[12].text,sizeof(marketUnitListRows[12].text),0,13},
        {"dealer.caption.mech_title",&marketMechStatusLabels[0].display,marketMechStatusLabels[0].text,sizeof(marketMechStatusLabels[0].text),0,0},
        {"dealer.caption.mech_buy",&marketMechStatusLabels[1].display,marketMechStatusLabels[1].text,sizeof(marketMechStatusLabels[1].text),0,0},
        {"dealer.caption.mech_inspect",&marketMechStatusLabels[2].display,marketMechStatusLabels[2].text,sizeof(marketMechStatusLabels[2].text),0,0},
        {"dealer.caption.mech_leave",&marketMechStatusLabels[3].display,marketMechStatusLabels[3].text,sizeof(marketMechStatusLabels[3].text),0,0},
        {"dealer.caption.dropship_title",&marketDropShipStatusLabels[0].display,marketDropShipStatusLabels[0].text,sizeof(marketDropShipStatusLabels[0].text),0,0},
        {"dealer.caption.dropship_buy",&marketDropShipStatusLabels[1].display,marketDropShipStatusLabels[1].text,sizeof(marketDropShipStatusLabels[1].text),0,0},
        {"dealer.caption.dropship_leave",&marketDropShipStatusLabels[2].display,marketDropShipStatusLabels[2].text,sizeof(marketDropShipStatusLabels[2].text),0,0},
        {"dealer.caption.component_title",&marketComponentTitle.display,marketComponentTitle.text,sizeof(marketComponentTitle.text),0,0},
        {"dealer.caption.component_system_1",&marketSystemConditionRows[0].display,marketSystemConditionRows[0].caption.text,sizeof(marketSystemConditionRows[0].caption.text),0,26},
        {"dealer.caption.component_system_2",&marketSystemConditionRows[1].display,marketSystemConditionRows[1].caption.text,sizeof(marketSystemConditionRows[1].caption.text),0,26},
        {"dealer.caption.component_system_3",&marketSystemConditionRows[2].display,marketSystemConditionRows[2].caption.text,sizeof(marketSystemConditionRows[2].caption.text),0,26},
        {"dealer.caption.component_system_4",&marketSystemConditionRows[3].display,marketSystemConditionRows[3].caption.text,sizeof(marketSystemConditionRows[3].caption.text),0,26},
        {"dealer.caption.component_system_5",&marketSystemConditionRows[4].display,marketSystemConditionRows[4].caption.text,sizeof(marketSystemConditionRows[4].caption.text),0,26},
        {"dealer.caption.component_system_6",&marketSystemConditionRows[5].display,marketSystemConditionRows[5].caption.text,sizeof(marketSystemConditionRows[5].caption.text),0,26},
        {"dealer.caption.component_system_7",&marketSystemConditionRows[6].display,marketSystemConditionRows[6].caption.text,sizeof(marketSystemConditionRows[6].caption.text),0,26},
        {"dealer.caption.component_system_8",&marketSystemConditionRows[7].display,marketSystemConditionRows[7].caption.text,sizeof(marketSystemConditionRows[7].caption.text),0,26},
        {"dealer.caption.component_system_9",&marketSystemConditionRows[8].display,marketSystemConditionRows[8].caption.text,sizeof(marketSystemConditionRows[8].caption.text),0,26},
        {"dealer.caption.component_system_10",&marketSystemConditionRows[9].display,marketSystemConditionRows[9].caption.text,sizeof(marketSystemConditionRows[9].caption.text),0,26},
        {"dealer.caption.component_body",&marketBodyCondition.display,marketBodyCondition.caption.text,sizeof(marketBodyCondition.caption.text),0,19},
        {"dealer.caption.component_weapon_heading",&marketWeaponHeading.display,marketWeaponHeading.text,sizeof(marketWeaponHeading.text),0,0},
        {"dealer.caption.component_weapon_1",&marketWeaponConditionRows[0].display,marketWeaponConditionRows[0].caption.text,sizeof(marketWeaponConditionRows[0].caption.text),0,26},
        {"dealer.caption.component_weapon_2",&marketWeaponConditionRows[1].display,marketWeaponConditionRows[1].caption.text,sizeof(marketWeaponConditionRows[1].caption.text),0,26},
        {"dealer.caption.component_weapon_3",&marketWeaponConditionRows[2].display,marketWeaponConditionRows[2].caption.text,sizeof(marketWeaponConditionRows[2].caption.text),0,26},
        {"dealer.caption.component_weapon_4",&marketWeaponConditionRows[3].display,marketWeaponConditionRows[3].caption.text,sizeof(marketWeaponConditionRows[3].caption.text),0,26},
        {"dealer.caption.component_weapon_5",&marketWeaponConditionRows[4].display,marketWeaponConditionRows[4].caption.text,sizeof(marketWeaponConditionRows[4].caption.text),0,26},
        {"dealer.caption.component_weapon_6",&marketWeaponConditionRows[5].display,marketWeaponConditionRows[5].caption.text,sizeof(marketWeaponConditionRows[5].caption.text),0,26},
        {"dealer.caption.component_weapon_7",&marketWeaponConditionRows[6].display,marketWeaponConditionRows[6].caption.text,sizeof(marketWeaponConditionRows[6].caption.text),0,26},
        {"dealer.caption.component_weapon_8",&marketWeaponConditionRows[7].display,marketWeaponConditionRows[7].caption.text,sizeof(marketWeaponConditionRows[7].caption.text),0,26},
        {"dealer.caption.component_weapon_9",&marketWeaponConditionRows[8].display,marketWeaponConditionRows[8].caption.text,sizeof(marketWeaponConditionRows[8].caption.text),0,26},
        {"dealer.caption.component_weapon_10",&marketWeaponConditionRows[9].display,marketWeaponConditionRows[9].caption.text,sizeof(marketWeaponConditionRows[9].caption.text),0,26},
        {"dealer.caption.component_done",&marketComponentDone.display,marketComponentDone.text,sizeof(marketComponentDone.text),0,0}};
    const size_t dealerCount=sizeof(dealerOwners)/sizeof(dealerOwners[0]);
    if (!MwImportUiCaptions(dealerOwners,dealerCount)
        || !MwImportUiListPanel("dealer.panel.market",&marketUnitListPanel,14,dealerOwners,dealerCount)
        || !MwImportUiPanel("dealer.panel.mech",&marketMechStatusPanel,4,dealerOwners,dealerCount)
        || !MwImportUiPanel("dealer.panel.dropship",&marketDropShipStatusPanel,3,dealerOwners,dealerCount)
        || !MwImportUiListPanel("dealer.panel.component",&marketComponentPanel,24,dealerOwners,dealerCount)
        || !MwImportUiMenu("dealer.menu.market",&marketUnitListMenu,dealerOwners,dealerCount)
        || !MwImportUiMenu("dealer.menu.mech",&marketMechMenu,dealerOwners,dealerCount)
        || !MwImportUiMenu("dealer.menu.dropship",&marketDropShipMenu,dealerOwners,dealerCount)
        || !MwImportUiMenu("dealer.menu.component",&marketComponentMenu,dealerOwners,dealerCount)) return false;
    if (!MwImportUiMenu("hub.menu.planet",&planetHubMenu,NULL,0)) return false; /*1083*/
    if (!MwImportUiMenu("hub.menu.contract",&contractHubMenu,NULL,0)) return false; /*10D7*/
    if (!MwImportUiMenu("hub.menu.remote_planet",&remotePlanetHubMenu,NULL,0)) return false; /*1101*/
    /* The body diagrams (3CEE): a background and nine regions each. */
    uint16_t diagrams[8*10*4];
    if (!MwUiCopyWords("complex.table.body_diagrams",diagrams,sizeof(diagrams)/sizeof(diagrams[0]),0)) return false;
    for (uint16_t chassis=0;chassis<8;++chassis)
        for (uint16_t region=0;region<10;++region)
        {
            MwMechDiagramRectangle *rectangle=region?&mechBodyDiagramLayouts[chassis].bodyRegions[region-1]:&mechBodyDiagramLayouts[chassis].background;
            const uint16_t *at=diagrams+(chassis*10+region)*4;
            rectangle->x=at[0];rectangle->y=at[1];rectangle->width=at[2];rectangle->height=at[3];
        }
    /* These initial near values have no live object until their callers set them. */
    repairQuoteDamage=NULL;componentRepairDamage=NULL;
    mechDiagramBodyDamage=NULL;mechDiagramRectangle=NULL;uiMenu=NULL;
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
    for (uint16_t i=0;i<4;++i) owners[count++]={companyIds[i],&companyMenuLabels[i].display,companyMenuLabels[i].text,sizeof(companyMenuLabels[i].text),0};
    for (uint16_t i=0;i<11;++i) owners[count++]={profileIds[i],&commanderProfileLabels[i].display,commanderProfileLabels[i].text,sizeof(commanderProfileLabels[i].text),0};
    for (uint16_t i=0;i<7;++i) owners[count++]={crewIds[i],&crewLabels[i].display,crewLabels[i].text,sizeof(crewLabels[i].text),0};
    for (uint16_t i=0;i<4;++i) owners[count++]={barIds[i],&barMenuLabels[i].display,barMenuLabels[i].text,sizeof(barMenuLabels[i].text),0};
    for (uint16_t i=0;i<3;++i) owners[count++]={recruitIds[i],&recruitInterviewLabels[i].display,recruitInterviewLabels[i].text,sizeof(recruitInterviewLabels[i].text),0};
    /* The crew menu's pilot rows (1..4) have no caption: 273E/28B2 assigns
     * them before any pilot record is selected. */
    if (!MwImportUiCaptions(owners,count)
        || !MwImportUiPanel("crew.panel.company",&companyMenuPanel,4,owners,count)
        || !MwImportUiPanel("crew.panel.commander_profile",&commanderProfilePanel,11,owners,count)
        || !MwImportUiPanel("crew.panel.crew",&crewLabelsPanel,7,owners,count)
        || !MwImportUiPanel("crew.panel.bar",&barMenuPanel,4,owners,count)
        || !MwImportUiPanel("crew.panel.recruit",&recruitInterviewPanel,3,owners,count)
        || !MwImportUiMenu("crew.menu.company",&companyMenu,owners,count)
        || !MwImportUiMenu("crew.menu.crew",&crewMenu,owners,count)
        || !MwImportUiMenu("crew.menu.bar",&barMenu,owners,count)
        || !MwImportUiMenu("crew.menu.recruit",&recruitInterviewMenu,owners,count)) return false;
    crewScreenRightEdge=0; /* 2438: Written before it's read; zero where the exe reloaded it. */
    pilotCardRosterSlot=0; /*243A*/
    pilotCardTextX=0; /* 243C: Written before it's read; zero where the exe reloaded it. */
    displayedPilotId=0; /*243E*/
    pilotCardGunneryTier=0; /* Written before it's read; zero where the exe reloaded it. */
    pilotCardPilotingTier=0; /* Written before it's read; zero where the exe reloaded it. */
    pilotCardPortraitId=0; /*2444*/
    pilotCardQuotedWageTier=0; /* Written before it's read; zero where the exe reloaded it. */
    pilotCardOwnedUnitSlot=0; /*2448*/
    assignmentSelectedPilotSlot=0; /*2581*/
    assignmentCandidateOwnedSlot=0; /*2583*/
    assignmentConflictingPilotSlot=0; /*2586*/
    assignmentConflictingOwnedSlot=0; /*2588*/
    recruitOfferSlot=0; /*911D*/
    selectedRecruitPilotId=0; /*911F*/
    assignmentConflictHighlighted=0; /*2585*/
    if (!MwUiCopyWords("crew.table.pilot_card_column_origins",pilotCardColumnOrigins,PILOT_CARD_COUNT,0)) return false; /*244A*/
    /* The crew's words (crew.word.*), through the pointer tables crew.json
     * names them by: reputation 23D0, skill 2452 and attitude 23FB. */
    uint16_t reputationWords[4],skillWords[4],attitudeWords[9];
    if (!MwUiCopyWords("crew.table.reputation_words",reputationWords,4,0)
        || !MwUiCopyWords("crew.table.skill_words",skillWords,4,0)
        || !MwUiCopyWords("crew.table.attitude_words",attitudeWords,9,0)) return false;
    for (uint16_t i=0;i<4;++i)
    {
        if (!ImportInitialString(mainWords,reputationWords[i],commanderReputationLabels[i],sizeof(commanderReputationLabels[i]))) return false;
        if (!ImportInitialString(mainWords,skillWords[i],pilotSkillLabels[i],sizeof(pilotSkillLabels[i]))) return false;
    }
    const uint16_t attitudeOffsets[5]={0x240D,0x2414,0x241D,0x2425,0x242E};
    for (uint16_t i=0;i<5;++i)
        if (!ImportInitialString(mainWords,attitudeOffsets[i],houseAttitudeLabels[i],sizeof(houseAttitudeLabels[i]))) return false;
    for (uint16_t i=0;i<9;++i)
    {
        uint16_t offset=attitudeWords[i],label=0;
        while (label<5 && attitudeOffsets[label]!=offset) ++label;
        if (label==5) return false;
        houseAttitudeText[i]=houseAttitudeLabels[label];
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
        if (caption==PILOT_ASSIGNMENT_CAPTION_COUNT || !MwUiCopyBytes(id,&pilotAssignmentCaptionColours[i],1,0)) return false;
        char textId[40];
        snprintf(textId,sizeof(textId),"%s.text",id);  /* the caption's text, story/words.json */
        if (!MwWordsCopyText(textId,pilotAssignmentCaptions[i],sizeof(pilotAssignmentCaptions[i]))) return false;
    }
    pilotDefinitions[0]=NULL;pilotDialogue[0]=NULL;
    for (uint16_t pilotId=1;pilotId<CAMPAIGN_PILOT_ID_COUNT;++pilotId)
    {
        uint16_t offset=pilotDefinitionOffsets[pilotId];
        if (offset<PILOT_DEFINITIONS_START || (uint32_t)offset+sizeof(MwPilotDefinitionPrefix)>=PILOT_DEFINITIONS_END) return false;
        const uint8_t *record=&pilotDefinitionStorage[offset-PILOT_DEFINITIONS_START];
        if (!memchr(record+3,0,0x0B) || !memchr(record+0x0E,0,PILOT_DEFINITIONS_END-offset-0x0E)) return false;
        pilotDefinitions[pilotId]=(const MwPilotDefinitionPrefix *)&pilotDefinitionStorage[offset-PILOT_DEFINITIONS_START];
        pilotDialogue[pilotId]=(const char *)(pilotDefinitions[pilotId]+1);
    }
    companyNavigationPadding=0; /*3020*/
    if (!ImportInitialString(mainWords,0x21C2,commanderProfileName,sizeof(commanderProfileName))) return false;
    if (!ImportInitialString(mainWords,0x9106,recruitStatLabels,sizeof(recruitStatLabels))) return false;
    if (!ImportInitialString(mainWords,0x9C39,noAvailableRecruitsText,sizeof(noAvailableRecruitsText))) return false;
    if (!ImportInitialString(mainWords,0x9C5A,fullFourPilotRosterText,sizeof(fullFourPilotRosterText))) return false;
    if (!ImportInitialString(mainWords,0x9C8D,fullThreePilotRosterText,sizeof(fullThreePilotRosterText))) return false;
    return ImportContractData();
}

MwPanelDescriptor contractOfficePanel,contractActionsPanel;
MwServiceCaption contractOfficeLabels[2],contractActionLabels[3];
static MwUiRecord contractOfficeRecords[2],contractActionRecords[6];
static uint8_t contractOfficeNavigationProbe,contractActionsNavigationProbe;
MwUiMenu contractOfficeMenu={contractOfficeRecords,2,&contractOfficeNavigationProbe};
MwUiMenu contractActionsMenu={contractActionRecords,6,&contractActionsNavigationProbe};
uint16_t negotiatingMissionOfferSlot,negotiatingMissionOfferByteOffset;
uint8_t contractRequestEditorInitialized,contractRequestTextColour;
uint16_t contractRequestTermByteOffset,contractNegotiationAttempts;
uint8_t negotiationRequestsSatisfied;
uint16_t contractDateYearXOffsets[CAMPAIGN_MONTH_COUNT];
uint16_t requestedTermX[3],requestedTermY[3],requestedTermDigits[3];
char contractNarrative[0x013C],houseContractNames[CAMPAIGN_HOUSE_COUNT][8];
char submitContractCaption[6],acceptContractCaption[6];
static uint8_t contractOfficePlanetGroupStorage[0x66];
const uint8_t *contractOfficePlanetGroups[CAMPAIGN_HOUSE_COUNT];
uint16_t houseExtraEnemyRollThresholds[CAMPAIGN_HOUSE_COUNT];
char noContractsText[0x2B],negotiationTerminatedText[0x47];
char contractAcceptedText[0x1B],houseContractRefusalText[0x5F];
uint16_t acceptedContractAdvanceCbillsLow,acceptedContractAdvanceCbillsHigh;
uint8_t deploymentRefusalCount,deploymentPenaltySuppressed;
char commanderNeedsMechText[0x37],assignedMechUnreadyText[0x3A],contractCancelledText[0x4F];

/* Actual EXE import boundary: no runtime segment registry or copied live DS. */
static bool ImportContractData(void)
{
    negotiatingMissionOfferSlot=0; /*488B*/
    negotiatingMissionOfferByteOffset=0; /*488E*/
    contractRequestEditorInitialized=0; /*488D*/
    contractRequestTextColour=0; /* 4B34: Written before it's read; zero where the exe reloaded it. */
    contractRequestTermByteOffset=0; /*4B32*/
    contractNegotiationAttempts=0; /*4B7E*/
    negotiationRequestsSatisfied=0; /*4B7D*/
    acceptedContractAdvanceCbillsLow=0; /*4B73*/
    acceptedContractAdvanceCbillsHigh=0; /*4B75*/
    deploymentRefusalCount=0; /*0FBF*/deploymentPenaltySuppressed=0; /*140B*/
    /* data/ui/contract_office.json: the year offsets (4B1A), and the requested
     * terms' x, then y, then digit counts (49CC). */
    if (!MwUiCopyWords("contract_office.table.date_year_x_offsets",contractDateYearXOffsets,CAMPAIGN_MONTH_COUNT,0)
        || !MwUiCopyWords("contract_office.table.requested_terms",requestedTermX,3,0)
        || !MwUiCopyWords("contract_office.table.requested_terms",requestedTermY,3,6)
        || !MwUiCopyWords("contract_office.table.requested_terms",requestedTermDigits,3,12)) return false;
    if (!ImportRuleWords("houses.extra_enemy_roll_thresholds",(uint16_t *)houseExtraEnemyRollThresholds,CAMPAIGN_HOUSE_COUNT)) return false;
    memcpy(submitContractCaption,mainWords+0x4B35,6);memcpy(acceptContractCaption,mainWords+0x4B3B,6);
    /* The planet groups (4B8A) and each House's pointer (4B80), from data/rules/houses.json. */
    uint16_t planetGroupOffsets[CAMPAIGN_HOUSE_COUNT];
    if (!ImportRuleBytes("houses.contract_planet_groups",contractOfficePlanetGroupStorage,sizeof(contractOfficePlanetGroupStorage))
        || !ImportRuleWords("houses.contract_planet_group_offsets",planetGroupOffsets,CAMPAIGN_HOUSE_COUNT)) return false;
    for (uint16_t house=0;house<CAMPAIGN_HOUSE_COUNT;++house)
    {
        char id[24];snprintf(id,sizeof(id),"house.%s.name",storyHouseKeys[house]);
        if (!MwStoryCopyText(id,houseContractNames[house],sizeof(houseContractNames[house]))) return false;
        uint16_t group=planetGroupOffsets[house];
        if (group<0x4B8A || group>=0x4BF0
            || !memchr(contractOfficePlanetGroupStorage+group-0x4B8A,CONTRACT_REGION_END,0x4BF0-group)) return false;
        contractOfficePlanetGroups[house]=contractOfficePlanetGroupStorage+group-0x4B8A;
    }
    /* The office (panel 4822, menu 486F) and the offer's actions (49DE, 4A15). */
    UiCaptionOwner owners[]={
        {"contract_office.caption.request_offer",&contractOfficeLabels[0].display,contractOfficeLabels[0].text,sizeof(contractOfficeLabels[0].text),0},
        {"contract_office.caption.leave_office",&contractOfficeLabels[1].display,contractOfficeLabels[1].text,sizeof(contractOfficeLabels[1].text),0},
        {"contract_office.caption.negotiate",&contractActionLabels[0].display,contractActionLabels[0].text,sizeof(contractActionLabels[0].text),0},
        {"contract_office.caption.next_offer",&contractActionLabels[1].display,contractActionLabels[1].text,sizeof(contractActionLabels[1].text),0},
        {"contract_office.caption.back_to_office",&contractActionLabels[2].display,contractActionLabels[2].text,sizeof(contractActionLabels[2].text),0}};
    const size_t count=sizeof(owners)/sizeof(owners[0]);
    if (!MwImportUiCaptions(owners,count)
        || !MwImportUiPanel("contract_office.panel.office",&contractOfficePanel,2,owners,count)
        || !MwImportUiPanel("contract_office.panel.actions",&contractActionsPanel,3,owners,count)
        || !MwImportUiMenu("contract_office.menu.office",&contractOfficeMenu,owners,count)
        || !MwImportUiMenu("contract_office.menu.actions",&contractActionsMenu,owners,count)) return false;
    if (!MwStoryCopyBytes("probe.contract_office",&contractOfficeNavigationProbe,1)
        || !MwStoryCopyBytes("probe.contract_actions",&contractActionsNavigationProbe,1)) return false;
    return MwStoryCopyText("contract.narrative",contractNarrative,sizeof(contractNarrative))
        && ImportInitialString(mainWords,0x4BFA,noContractsText,sizeof(noContractsText))
        && ImportInitialString(mainWords,0x4C25,negotiationTerminatedText,sizeof(negotiationTerminatedText))
        && ImportInitialString(mainWords,0x4C6C,contractAcceptedText,sizeof(contractAcceptedText))
        && MwStoryCopyText("contract.refusal.house",houseContractRefusalText,sizeof(houseContractRefusalText))
        && ImportInitialString(mainWords,0x0FC0,commanderNeedsMechText,sizeof(commanderNeedsMechText))
        && ImportInitialString(mainWords,0x0FF7,assignedMechUnreadyText,sizeof(assignedMechUnreadyText))
        && ImportInitialString(mainWords,0x1031,contractCancelledText,sizeof(contractCancelledText));
}

/* The after-action and settlement directories (1090:1491, 166A, 1786), whose
 * entries the C references above stand for, are checked at import: the
 * import table's check pieces (src/Import/pieces.txt) refuse an installation
 * whose directories differ. */

/* Native reserve-ammunition shop state and shared caption/menu owners. */
uint16_t reserveAmmoShopSelectedRow,reserveAmmoPreviewSelection;
uint8_t reserveAmmoSelling,reserveAmmoMechColours[11];
char reserveAmmoBuyLabel[5],reserveAmmoSellLabel[5];
MwServiceCaption reserveAmmoShopHeading,reserveAmmoFamilyLabels[6],reserveAmmoDoneLabel;
MwReserveAmmoQuantityRow reserveAmmoQuantityRows[6];
MwPanelDescriptor reserveAmmoShopPanel,reserveAmmoCompatibilityPanels[6];
MwServiceCaption reserveAmmoCompatibilityHeadings[6];
uint16_t reserveAmmoCompatibleMechs[6][6];
MwUiRecord reserveAmmoShopRecords[RESERVE_AMMO_MENU_ROWS];
/* Native FF-neighbour probe is 1090:505A, the existing travel table WORD. */
MwUiMenu reserveAmmoShopMenu={reserveAmmoShopRecords,RESERVE_AMMO_MENU_ROWS,NULL,&travelSquareRootValues[0x1BA],0};

static bool ImportReserveAmmoUiData(void)
{
    reserveAmmoShopSelectedRow=0; /*4137*/
    reserveAmmoPreviewSelection=0; /*3F6E*/
    reserveAmmoSelling=0; /*4139*/
    memset(reserveAmmoMechColours,0,sizeof(reserveAmmoMechColours)); /* 3F70: Written before it's read; zero where the exe reloaded it. */
    if (!ImportInitialString(mainWords,0x413A,reserveAmmoBuyLabel,sizeof(reserveAmmoBuyLabel))
        || !ImportInitialString(mainWords,0x413F,reserveAmmoSellLabel,sizeof(reserveAmmoSellLabel))) return false;
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
    owners[count++]={"dealer.caption.ammo_heading",&reserveAmmoShopHeading.display,reserveAmmoShopHeading.text,sizeof(reserveAmmoShopHeading.text),0,0};
    for (uint16_t i=0;i<CAMPAIGN_AMMO_FAMILY_COUNT;++i)
        owners[count++]={familyIds[i],&reserveAmmoFamilyLabels[i].display,reserveAmmoFamilyLabels[i].text,sizeof(reserveAmmoFamilyLabels[i].text),0,0};
    for (uint16_t i=0;i<CAMPAIGN_AMMO_FAMILY_COUNT;++i)
        owners[count++]={quantityIds[i],&reserveAmmoQuantityRows[i].display,reserveAmmoQuantityRows[i].text,sizeof(reserveAmmoQuantityRows[i].text),0,0};
    owners[count++]={"dealer.caption.ammo_done",&reserveAmmoDoneLabel.display,reserveAmmoDoneLabel.text,sizeof(reserveAmmoDoneLabel.text),0,0};
    for (uint16_t i=0;i<CAMPAIGN_AMMO_FAMILY_COUNT;++i)
        owners[count++]={compatibilityIds[i],&reserveAmmoCompatibilityHeadings[i].display,reserveAmmoCompatibilityHeadings[i].text,
            sizeof(reserveAmmoCompatibilityHeadings[i].text),0,0};
    if (!MwImportUiCaptions(owners,count)
        || !MwImportUiPanel("dealer.panel.ammo",&reserveAmmoShopPanel,14,owners,count)
        || !MwImportUiMenu("dealer.menu.ammo",&reserveAmmoShopMenu,owners,count)) return false;
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
            || !MwImportUiPanel(compatibilityPanelIds[panel],&reserveAmmoCompatibilityPanels[family],1,owners,count)) return false;
        uint16_t kinds=familyMechLists[family],index=0;
        if (kinds<AMMO_FAMILY_MECHS_START || (kinds-AMMO_FAMILY_MECHS_START)%2) return false;
        do
        {
            const uint16_t word=(uint16_t)((kinds-AMMO_FAMILY_MECHS_START)/2+index);
            if (index>=6 || word>=AMMO_FAMILY_MECH_WORDS) return false;
            uint16_t kind=familyMechs[word];
            reserveAmmoCompatibleMechs[family][index++]=kind;
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
        const size_t length=strlen((const char *)calendarMonthNames[month]);
        if (!length || calendarMonthNameWidths[month]!=(length-1)*CAMPAIGN_GLYPH_ADVANCE)
            return fprintf(stderr,"MechWarrior: hub.json's calendar_month_name_widths doesn't fit month %u's name\n",month+1),false;
        if (contractDateYearXOffsets[month]!=(length-1)*CONTRACT_GLYPH_ADVANCE)
            return fprintf(stderr,"MechWarrior: contract_office.json's date_year_x_offsets doesn't fit month %u's name\n",month+1),false;
    }
    static_assert(sizeof(repairTitleLeadingSpacesByKind)/sizeof(repairTitleLeadingSpacesByKind[0])==CAMPAIGN_UNIT_KIND_COUNT,"one padding per kind");
    for (uint16_t kind=0;kind<CAMPAIGN_UNIT_KIND_COUNT;++kind)
        if (repairTitleLeadingSpacesByKind[kind]!=(sizeof(unitDisplayNames[kind])-strlen(unitDisplayNames[kind]))/2)
            return fprintf(stderr,"MechWarrior: complex.json's repair_title_leading_spaces doesn't fit the %s's name\n",storyUnitKeys[kind]),false;
    return true;
}
