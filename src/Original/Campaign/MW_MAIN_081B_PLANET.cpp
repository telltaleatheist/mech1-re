#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/campaign_continuations.h"
#include "mechwarrior/campaign.h"
#include "mechwarrior/campaign_ui.h"
#include "mechwarrior/campaign_events.h"
#include "mechwarrior/campaign_state.h"
#include "mechwarrior/campaign_market.h"
#include "mechwarrior/ega_campaign.h"
#include <stdlib.h>

enum { HUB_LANDSCAPE_PICTURE_BASE=3,HUB_TITLE_PLATE_PICTURE=4,
    HUB_LEFT_UPPER_PICTURE=5,HUB_MECH_COMPLEX_PICTURE=6,HUB_DEPLOYMENT_PICTURE=0x0B,
    HUB_RIGHT_UPPER_PICTURE=8,HUB_RIGHT_MIDDLE_PICTURE=9,HUB_RIGHT_LOWER_PICTURE=0x0A,
    HUB_HOUSE_FACILITY_PICTURE_BASE=0x1A,HUB_LEFT_ICON_X=0x19,HUB_RIGHT_ICON_X=0xFF,
    HUB_UPPER_ICON_Y=0x37,HUB_MIDDLE_ICON_Y=0x69,HUB_LOWER_ICON_Y=0x9B,
    HUB_LEFT_TITLE_X=0x0A,HUB_RIGHT_TITLE_X=0xAA,HUB_TITLE_Y=0x0A,
    HUB_PLANET_NAME_X=0x20,HUB_CALENDAR_X=0xC0,HUB_HEADER_TEXT_Y=0x1C,HUB_TEXT_COLOUR=Campaign_Pal_EGA_White };

/* MW_MAIN081B:1909..1A35. POP1909 discards entry transport, not a C game
 * argument.1A0D resumes AFTER the suspended arrival-event CALL, never redraws
 * the hub or dispatches those events twice. No replacement scene loop. */
uint16_t MwEnterCampaignPlanetHub(uint16_t entryOffset)
{
    if (entryOffset==CAMPAIGN_CONTINUE_HUB_MENU) goto selectHubMenu;
    if (entryOffset!=CAMPAIGN_CONTINUE_PLANET_HUB) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: entryOffset!=CAMPAIGN_CONTINUE_PLANET_HUB");
    campaignUi.companyMenuSelectedRow=0;
    campaignService.unitInspectionContextCode=UINT16_MAX;
    campaignUi.barMenuSelectedRow=UINT16_MAX;
    campaignUi.optionsMenuSelection=0;
    if (!campaignStopMusic) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !campaignStopMusic");
    campaignStopMusic();
    EgaSelectFont(0);
    MwDrawIndexedPicture(0,0,campaignCore.currentPlanetLandscapeIndex+HUB_LANDSCAPE_PICTURE_BASE);
    MwDrawGeneralPicture(HUB_LEFT_ICON_X,HUB_UPPER_ICON_Y,EGA_DRAW_PAGE,HUB_LEFT_UPPER_PICTURE);
    MwDrawGeneralPicture(HUB_LEFT_ICON_X,HUB_MIDDLE_ICON_Y,EGA_DRAW_PAGE,HUB_MECH_COMPLEX_PICTURE);
    if (campaignMission.acceptedContractActive==1)
        MwDrawGeneralPicture(HUB_LEFT_ICON_X,HUB_LOWER_ICON_Y,EGA_DRAW_PAGE,HUB_DEPLOYMENT_PICTURE);
    else
    {
        if (campaignCore.currentPlanetFacilityDisplayFlag)
            MwDrawGeneralPicture(HUB_LEFT_ICON_X,HUB_LOWER_ICON_Y,EGA_DRAW_PAGE,campaignCore.currentPlanetHouseIndex+HUB_HOUSE_FACILITY_PICTURE_BASE);
        MwDrawGeneralPicture(HUB_RIGHT_ICON_X,HUB_UPPER_ICON_Y,EGA_DRAW_PAGE,HUB_RIGHT_UPPER_PICTURE);
        MwDrawGeneralPicture(HUB_RIGHT_ICON_X,HUB_MIDDLE_ICON_Y,EGA_DRAW_PAGE,HUB_RIGHT_MIDDLE_PICTURE);
        MwDrawGeneralPicture(HUB_RIGHT_ICON_X,HUB_LOWER_ICON_Y,EGA_DRAW_PAGE,HUB_RIGHT_LOWER_PICTURE);
    }
    MwDrawGeneralPicture(HUB_LEFT_TITLE_X,HUB_TITLE_Y,EGA_DRAW_PAGE,HUB_TITLE_PLATE_PICTURE);
    MwDrawGeneralPicture(HUB_RIGHT_TITLE_X,HUB_TITLE_Y,EGA_DRAW_PAGE,HUB_TITLE_PLATE_PICTURE);
    EgaFlipPages();
    MwDrawPlanetNameCentered(HUB_PLANET_NAME_X,HUB_HEADER_TEXT_Y,HUB_TEXT_COLOUR,campaignCore.currentPlanetIndex);
    /* Native font0's final nonempty name glyph leaves CX0 at0AAE LOOP. All
     * installed planet names take it; CH is not a separate game argument. */
    MwDrawCampaignCalendar(HUB_CALENDAR_X,0,1,HUB_TEXT_COLOUR,HUB_HEADER_TEXT_Y);
    MwResetRepairAttemptFailures();
    MwShowPendingNotifications();
    campaignEventDispatchReturnContinuation=CAMPAIGN_CONTINUE_HUB_MENU;
    uint16_t transfer;transfer=MwDispatchCampaignEvents(0,CAMPAIGN_CALL_DISPATCH_EVENTS);
    if (transfer) return transfer;
    campaignEventDispatchReturnContinuation=0;
selectHubMenu:
    campaignEvents.planetArrivalEventsPending=0; /* AFTER dispatch/choice return. */
    MwUiMenu *menu;
    if (campaignMission.acceptedContractActive) menu=&campaignUi.contractHubMenu;
    else if (campaignCore.currentPlanetFacilityDisplayFlag) menu=&campaignUi.planetHubMenu;
    else menu=&campaignUi.remotePlanetHubMenu;
    MwSelectUiRecordTable(campaignUi.hubSelectedRecordIndex,menu);
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN 081B:1ACE..1B54. Startup/arrival applies the selected record before
 * further offers/recruits are generated. Two RNG draws, in the native order. */
void MwApplyCurrentPlanetState(void)
{
    uint16_t directoryByteOffset = campaignCore.currentPlanetIndex * sizeof(uint16_t);
    if (directoryByteOffset >= sizeof(campaignCore.planetRecordOffsets)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: directoryByteOffset >= sizeof(planetRecordOffsets)"); /* Unmapped adjacent DS, not a clamp. */
    uint16_t planetOffset = campaignCore.planetRecordOffsets[directoryByteOffset / sizeof(uint16_t)];
    if (planetOffset < PLANET_RECORDS_START
        || (uint32_t)planetOffset + sizeof(MwPlanetHeader) > PLANET_RECORDS_END) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: planetOffset < PLANET_RECORDS_START || (uint32_t)planetOffset + sizeof(MwPlanetHeader) > PLANET_RECORDS_END");
    const MwPlanetHeader *planet = (const MwPlanetHeader *)&campaignCore.planetRecordStorage[planetOffset - PLANET_RECORDS_START];
    uint8_t classification = planet->landscapeAndMarketCode;
    classification--; /* DEC AL wraps before the unsigned BYTE divide. */
    campaignCore.currentPlanetMarketTier = classification / PLANET_LANDSCAPE_COUNT;
    campaignCore.currentPlanetLandscapeIndex = classification % PLANET_LANDSCAPE_COUNT;
    campaignCore.currentPlanetHouseIndex = planet->houseIndex;
    campaignCore.currentPlanetFacilityDisplayFlag = planet->facilityDisplayFlag;
    campaignCore.currentPlanetMapPositionX = planet->mapPositionX;
    campaignCore.currentPlanetMapPositionY = planet->mapPositionY;
    uint16_t basePremium = (uint8_t)(campaignCore.currentPlanetMarketTier + 1) * PLANET_MARKET_PREMIUM_STEP;
    campaignCore.planetMarketPremiumPercent = MwRandomUpTo(PLANET_PREMIUM_RANDOM_MAX) + basePremium;
    for (uint16_t priceIndex = 0; priceIndex < AMMUNITION_PRICE_COUNT; ++priceIndex)
    {
        uint32_t pricePremium = (uint32_t)campaignCore.ammunitionBasePrices[priceIndex] * campaignCore.planetMarketPremiumPercent;
        pricePremium /= PERCENT_DIVISOR;
        if (pricePremium > UINT16_MAX) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: pricePremium > UINT16_MAX"); /* Unbound native INT0; earlier prices remain written. */
        /* The succeeding ADD/store wraps as a WORD, unlike DIV overflow. */
        campaignCore.ammunitionTransactionPrices[priceIndex] = pricePremium + campaignCore.ammunitionBasePrices[priceIndex];
    }
    uint16_t attemptLimit = MwRandomUpTo(PLANET_NEGOTIATION_RANDOM_MAX);
    if (campaignCore.currentPlanetHouseIndex >= CAMPAIGN_HOUSE_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: currentPlanetHouseIndex >= CAMPAIGN_HOUSE_COUNT"); /* Unmapped House byte, at its native use. */
    /* ADD AL only: wrap the low byte without carrying into AH. */
    campaignCore.contractNegotiationAttemptLimit = (attemptLimit & NATIVE_WORD_HIGH_BYTE_MASK)
        | (uint8_t)((uint8_t)attemptLimit + campaignCore.houseNegotiationAttemptBase[campaignCore.currentPlanetHouseIndex]);
}

/* MW_MAIN 081B:3FFD..400B. Hub 1A01 calls this once per hub entry, not repair.
 * Nested row/slot loops visit the same 220 BYTEs in increasing native order. */
void MwResetRepairAttemptFailures(void)
{
    for (uint16_t mechKind = 0; mechKind < REPAIR_ATTEMPT_KIND_COUNT; ++mechKind)
        for (uint16_t damageIndex = 0; damageIndex < REPAIR_ATTEMPT_SLOTS_PER_KIND; ++damageIndex)
            campaignCore.repairAttemptFailuresByKind[mechKind].failedByDamageIndex[damageIndex] = 0;
}
