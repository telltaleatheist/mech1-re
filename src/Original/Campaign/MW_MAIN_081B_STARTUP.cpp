#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/campaign_continuations.h"
#include "mechwarrior/campaign_startup.h"
#include "mechwarrior/ega_campaign.h"
#include "mechwarrior/campaign_ui.h"
#include "mechwarrior/campaign_economy.h"
#include "mechwarrior/campaign_handoff.h"
#include "mechwarrior/campaign_pilots.h"
#include "mechwarrior/campaign_events.h"
#include "mechwarrior/video_mode.h"
#include "mechwarrior/campaign_input.h"
#include "mechwarrior/event_log.h"
#include "mechwarrior/game_files.h"
#include "Canvas.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* The unconverted Tandy branch uses its16-colour register codes. */

MwCampaignPlatformCalls mwCampaignPlatform;
void (*campaignStopMusic)(void);
void (*campaignStartMusic)(uint16_t selector,uint16_t offset);

/* Run the mercenary company's campaign, entering the planet hub on a new game
 * or settling the returned combat result before resuming campaign activity.
 * MW_MAIN081B:0002..015E/0179..0195 and failure tail02D0..02E5.
 * New-game entry enters actual166D,1909 and089B. Return144E imports the real
 * resident exchange then rejoins1CD4 inside this same native entry frame.
 * Physical DS/BP/SP saves are native CPU transport, not C objects/arguments;
 * the return value is the real campaign AL result. */
uint8_t MwRunCampaign(const MwCampaignLaunch *launch)
{
    uint16_t continuation;
    campaignStartup.launcherMemoryTier=launch->memoryTier;
    if (launch->identity[0]!='M'
        || launch->identity[4]!='W'
        || launch->identity[10]!='R')
    {
        campaignStartup.launcherExitCode=CAMPAIGN_EXIT_QUIT;
        goto finish;
    }
    MwInstallCampaignInterruptHandlers();
    if (!mwCampaignPlatform.allocateParagraphs || !mwCampaignPlatform.loadGraphicsOverlay) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mwCampaignPlatform.allocateParagraphs || !mwCampaignPlatform.loadGraphicsOverlay");
    uint16_t errorIndex,selectorOrError;
    if (mwCampaignPlatform.allocateParagraphs(0x0FFF,&selectorOrError))
    {errorIndex=4;goto allocationFailed;}
    campaignCore.activePictureBankSelector=selectorOrError;
    if (mwCampaignPlatform.allocateParagraphs(0x0909,&selectorOrError))
    {errorIndex=5;goto allocationFailed;}
    campaignCore.secondaryPictureBankSelector=selectorOrError;
    if (mwCampaignPlatform.allocateParagraphs(0x084D,&selectorOrError))
    {errorIndex=2;goto allocationFailed;}
    campaignCore.generalPictureBankSelector=selectorOrError;
    if (mwCampaignPlatform.allocateParagraphs(0x0D6D,&selectorOrError))
    {errorIndex=3;goto allocationFailed;}
    campaignCore.cockpitPictureBankSelector=selectorOrError;
    if (mwCampaignPlatform.allocateParagraphs(0x0800,&selectorOrError))
    {errorIndex=1;goto allocationFailed;}
    campaignCore.indexedPictureOutputSelector=selectorOrError;
    if (mwCampaignPlatform.allocateParagraphs(0x0800,&selectorOrError))
    {errorIndex=1;goto allocationFailed;}
    campaignCore.compressedPictureBufferSelector=selectorOrError;
    if (mwCampaignPlatform.allocateParagraphs(0x0338,&selectorOrError))
    {errorIndex=6;goto allocationFailed;}
    campaignStartup.campaignMusicBufferSelector=selectorOrError;
    if (mwCampaignPlatform.allocateParagraphs(0x0177,&selectorOrError))
    {errorIndex=0;goto allocationFailed;}
    campaignStartup.graphicsOverlaySelector=selectorOrError;
    if (mwCampaignPlatform.loadGraphicsOverlay("MW_EGA.EXE",campaignStartup.graphicsOverlaySelector)) /* Tandy dropped. */
    {errorIndex=1;goto allocationFailed;}
    EgaBuildScanlineOffsets();
    if (EgaLoadFonts()) {errorIndex=5;goto allocationFailed;}
    if (MwLoadGeneralPictureBank()) {errorIndex=6;goto allocationFailed;}
    if (MwLoadCockpitPictureBank()) {errorIndex=6;goto allocationFailed;}
    if (MwLoadSecondaryPictureBank()) {errorIndex=6;goto allocationFailed;}
    MwDetectAndCalibrateInputDevices();
    MwMixBiosTicksIntoRandomState();
    if (launch->phase!=CAMPAIGN_PHASE_NEW)
    {
        /*144E captures PSP transport before testing phase. The supported
         * resident protocol is a distinct0806 owner, not a live1090 alias. */
        MwDecodeCampaignExchange(campaignCombatExchange);
        campaignMission.acceptedContractActive=0;
        (void)MwLoadBarDeathAndLaunchMusic();
        (void)MwLoadPrimaryPictureBank();
        EgaInitializeGraphics();
        goto beginDebrief;
    }
    /* Native1494..15E9 remains part of this same campaign entry procedure. */
    MwSaveCurrentVideoMode();
    EgaInitializeGraphics();
    (void)MwLoadMechMusic();
    (void)MwLoadAlternatePictureBank();
    EgaClearDrawPage(Campaign_Pal_EGA_Black);
    MwDrawActivePicture(0x000C,0x002D,EGA_DRAW_PAGE,1);
    EgaFlipPages();
    if (MwWaitTicksOrInputInterruption(0x0048))
    {
        (void)MwLoadTPictureBank();
        goto initializeCampaign;
    }
    EgaClearDrawPage(Campaign_Pal_EGA_Black);
    EgaFlipPages();
    (void)MwLoadTPictureBank();
    EgaSelectFont(0);
    MwSetIntroTextPaletteColour(EGA_PALETTE_BLACK);
    MwDrawIntroNarrationLines(2);
    MwSetIntroTextPaletteColour(EGA_PALETTE_DARK_GREY);
    (void)MwWaitTicksOrInputInterruption(2);
    MwSetIntroTextPaletteColour(EGA_PALETTE_LIGHT_GREY);
    (void)MwWaitTicksOrInputInterruption(2);
    MwSetIntroTextPaletteColour(EGA_PALETTE_WHITE);
    if (MwWaitTicksOrInputInterruption(0x0024)) goto initializeCampaign;
    if (!campaignStartMusic) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !campaignStartMusic");
    campaignStartMusic(campaignStartup.campaignMusicBufferSelector,0);
    if (MwWaitTicksOrInputInterruption(0x019E)) goto initializeCampaign;
    MwDrawActivePicture(0,0,EGA_DRAW_PAGE,1);
    EgaFlipPages();
    EgaCopyDisplayToDrawPage(0,0,EGA_COLUMNS,EGA_ROWS);
    if (MwDrawTimedPictureAndTextTransition(0x0084,&campaignStartup.introFirstLeftText,&campaignStartup.introFirstRightText)) goto initializeCampaign;
    if (MwDrawTimedPictureAndTextTransition(0x0084,&campaignStartup.introSecondLeftText,&campaignStartup.introSecondRightText)) goto initializeCampaign;
    if (MwDrawTimedPictureAndTextTransition(0x0084,&campaignStartup.introThirdLeftText,&campaignStartup.introThirdRightText)) goto initializeCampaign;
    if (MwDrawTimedPictureAndTextTransition(0x0084,&campaignStartup.introFourthLeftText,&campaignStartup.introFourthRightText)) goto initializeCampaign;
    if (MwDrawTimedPictureAndTextTransition(0x0084,&campaignStartup.introFifthLeftText,&campaignStartup.introFifthRightText)) goto initializeCampaign;
    if (MwDrawTimedPictureAndTextTransition(0x0084,&campaignStartup.introSixthLeftText,&campaignStartup.introSixthRightText)) goto initializeCampaign;
    if (MwDrawTimedPictureAndTextTransition(0x00CC,&campaignStartup.introSeventhLeftText,&campaignStartup.introSeventhRightText)) goto initializeCampaign;
    (void)MwWaitTicksOrInputInterruption(0x006C);
initializeCampaign:
    continuation=MwStartNewCampaign(CAMPAIGN_CONTINUE_NEW_CAMPAIGN);
resumeCampaign:
    if (continuation==CAMPAIGN_CONTINUE_PLANET_HUB || continuation==CAMPAIGN_CONTINUE_HUB_MENU)
        continuation=MwEnterCampaignPlanetHub(continuation);
    if (continuation==CAMPAIGN_CONTINUE_UI)
        continuation=MwCampaignUiEventLoop();
    if (continuation==CAMPAIGN_CONTINUE_AFTER_ACTION) goto beginDebrief;
    if (continuation==CAMPAIGN_CONTINUE_VICTORY_EVENT_RETURN) goto finishVictory;
    if (continuation==CAMPAIGN_CONTINUE_CLEANUP) goto campaignExit;
    /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Unrepresented nonlocal destination, never a successful RET."); /* Unrepresented nonlocal destination, never a successful RET. */
beginDebrief:
    /*1CD4 is a shared continuation INSIDE0002, not a callable new method. */
    EgaClearDrawPage(AFTER_ACTION_BACKGROUND);
    EgaFlipPages();
    if (!mwCampaignInput.keyboardAvailable || !mwCampaignInput.readKeyboard) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mwCampaignInput.keyboardAvailable || !mwCampaignInput.readKeyboard");
    while (mwCampaignInput.keyboardAvailable()) (void)mwCampaignInput.readKeyboard();
    if (campaignMission.combatTransitionResult==0 && campaignMission.combatScenarioCode==COMBAT_SCENARIO_DARK_WING_BASE)
        goto darkWingVictory;
    MwDrawAlternatePicturePanelBorder(2,0x008D,0x013A,0x0039);
    {
        uint16_t picture=MwLoadIndexedPicture(campaignCore.currentPlanetHouseIndex+AFTER_ACTION_HOUSE_PICTURE_BASE);
        (void)EgaDrawPackedSprite(0x0100,0x008E,EGA_DISPLAY_PAGE,0,picture);
    }
    MwDrawAlternatePicturePanelBorder(2,2,0x013B,0x0088);
    EgaDrawLine(Campaign_Pal_EGA_White,7,0x0136,0x009C,7); /* BX/CX=(009C,0007), DX/SI=(0136,0007). */
    {
        uint16_t picture=AFTER_ACTION_SUCCESS_PICTURE;
        if (campaignMission.combatTransitionResult!=0)
        {
            picture=AFTER_ACTION_LOSS_PICTURE;
            uint8_t statusSum=0;
            uint16_t rosterSlot=0,remaining=pilotRosterCount;
            do
            {
                if (rosterSlot>=CAMPAIGN_ROSTER_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: rosterSlot>=CAMPAIGN_ROSTER_SLOTS"); /* Unmapped adjacent/corrupt roster. */
                statusSum+=campaignMechanics.rosterStatus[rosterSlot++];
            } while (--remaining!=0);
            if (statusSum==0) picture=AFTER_ACTION_RETREAT_PICTURE;
        }
        picture=MwLoadIndexedPicture(picture);
        (void)EgaDrawPackedSprite(8,7,EGA_DISPLAY_PAGE,0,picture);
    }
    EgaSelectFont(AFTER_ACTION_FONT);
    MwDrawCampaignTextColourOne(AFTER_ACTION_TEXT_X,0x000A,campaignHandoff.afterActionCrewHeading);
    MwDrawCampaignTextColourOne(AFTER_ACTION_TEXT_X,0x003C,campaignHandoff.afterActionMechHeading);
    MwDrawCampaignTextColourOne(AFTER_ACTION_TEXT_X,0x006E,campaignHandoff.afterActionSalvageHeading);
    MwDrawCampaignTextColourOne(AFTER_ACTION_TEXT_X,0x0078,campaignHandoff.afterActionPaymentHeading);
    {
        uint16_t rosterSlot=0;
        do
        {
            if (rosterSlot>=CAMPAIGN_ROSTER_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: rosterSlot>=CAMPAIGN_ROSTER_SLOTS"); /* Unmapped corrupt roster. */
            const char *pilotName=(const char *)campaignMission.commanderNameBytes;
            if (rosterSlot!=0)
            {
                uint16_t pilotId=rosterPilotIds[rosterSlot];
                if (pilotId>=CAMPAIGN_PILOT_ID_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: pilotId>=CAMPAIGN_PILOT_ID_COUNT");
                pilotName=(const char *)campaignPilots.pilotDefinitions[pilotId]->nameBytes;
            }
            campaignHandoff.afterActionRosterRowOffset=(uint8_t)rosterSlot*AFTER_ACTION_ROW_HEIGHT;
            uint16_t pilotY=campaignHandoff.afterActionRosterRowOffset+AFTER_ACTION_FIRST_PILOT_Y;
            MwDrawCampaignTextColourOne(AFTER_ACTION_TEXT_X,pilotY,pilotName);
            uint8_t status=campaignMechanics.rosterStatus[rosterSlot];
            if (status>=3) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: status>=3"); /* Unmapped status-text adjacency, not a clamp. */
            MwDrawCampaignTextColourOne(AFTER_ACTION_STATUS_X,pilotY,campaignHandoff.afterActionPilotStatusText[status]);
            uint16_t ownedUnitSlot=campaignMechanics.rosterMechAssignments[rosterSlot];
            if (ownedUnitSlot!=CAMPAIGN_NO_MECH_ASSIGNMENT)
            {
                if (ownedUnitSlot>=CAMPAIGN_OWNED_UNIT_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: ownedUnitSlot>=CAMPAIGN_OWNED_UNIT_SLOTS");
                uint16_t unitKind=campaignEconomy.ownedUnitKinds[ownedUnitSlot];
                if (unitKind>=CAMPAIGN_UNIT_KIND_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: unitKind>=CAMPAIGN_UNIT_KIND_COUNT");
                /* Native1E24/1E29 indexes these BY DOUBLED ROSTER SLOT, not
                 * chassis. Preserve that discrepancy instead of a tidy quote. */
                uint16_t weaponCount=campaignMechanics.unitWeaponCounts[rosterSlot*2];
                uint16_t systemCount=campaignService.unitConditionSystemFieldCounts[rosterSlot*2];
                const uint8_t *damage=(const uint8_t *)&campaignMechanics.ownedUnitDamage[(uint8_t)ownedUnitSlot];
                uint8_t damageSum=0;
                uint16_t field=0;
                do
                {
                    if (field>=sizeof(CampaignUnitDamage)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: field>=sizeof(CampaignUnitDamage)"); /* Unsupported adjacent-data spill after partial sum. */
                    damageSum+=damage[field++];
                } while (--systemCount!=0);
                field=offsetof(CampaignUnitDamage,weaponDamageTiers);
                do
                {
                    if (field>=sizeof(CampaignUnitDamage)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: field>=sizeof(CampaignUnitDamage)");
                    damageSum+=damage[field++];
                } while (--weaponCount!=0);
                for (uint16_t location=0;location<CAMPAIGN_BODY_LOCATION_COUNT;++location)
                    damageSum+=campaignMechanics.ownedUnitDamage[(uint8_t)ownedUnitSlot].bodyDamageTiers[location];
                uint16_t divisor=campaignService.unitConditionScoreDivisors[unitKind];
                uint16_t remainingScore=divisor-damageSum;
                uint32_t condition=(uint32_t)remainingScore*PERCENT_DIVISOR;
                if (!divisor || condition/divisor>UINT16_MAX) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !divisor || condition/divisor>UINT16_MAX"); /* Native DIV trap, no clamp. */
                condition/=divisor;
                MwDrawCampaignTextColourOne(AFTER_ACTION_TEXT_X,
                    campaignHandoff.afterActionRosterRowOffset+AFTER_ACTION_FIRST_MECH_Y,campaignService.unitDisplayNames[unitKind]);
                /* Name glyph LOOP leaves CH00; 1E88 writes only CL01. */
                MwDrawSignedDecimalField(condition,AFTER_ACTION_PERCENT_X,1,0,
                    AFTER_ACTION_FOREGROUND,AFTER_ACTION_BACKGROUND,campaignHandoff.afterActionRosterRowOffset+AFTER_ACTION_FIRST_MECH_Y);
                MwDrawCampaignTextColourOne(AFTER_ACTION_SUFFIX_X,
                    campaignHandoff.afterActionRosterRowOffset+AFTER_ACTION_FIRST_MECH_Y,campaignHandoff.afterActionPercentSuffix);
            }
            ++rosterSlot;
        } while (rosterSlot!=pilotRosterCount); /* LIVE count reload/equality. */
    }
    /*1EB6..1FB8. All arithmetic remains in the native entry continuation. */
    campaignHandoff.settlementPayoutPercent=PERCENT_DIVISOR;
    if (campaignMission.combatScenarioCode!=COMBAT_SCENARIO_DARK_WING_BASE)
    {
        uint16_t deductionRoll=MwRandomUpTo(PERCENT_DIVISOR);
        uint16_t house=campaignCore.currentPlanetHouseIndex;
        if (house>=CAMPAIGN_HOUSE_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: house>=CAMPAIGN_HOUSE_COUNT");
        if (deductionRoll<=campaignHandoff.houseDeductionRollThresholds[house])
            campaignHandoff.settlementPayoutPercent=campaignHandoff.houseReducedPayoutPercentages[house];
    }
    {
        uint32_t salvagePayment=0;
        if (campaignMission.combatScenarioCode!=COMBAT_SCENARIO_DARK_WING_BASE
            && campaignMission.combatTransitionResult==0 && campaignMission.acceptedContractSalvagePercent!=0)
        {
            uint16_t salvageThousands=0;
            for (uint16_t slot=0;slot<CAMPAIGN_SALVAGE_SLOTS;++slot)
            {
                const CampaignSalvageEntry *salvage=&campaignMission.salvageEntries[slot];
                uint16_t unitKind=salvage->unitKind|((uint16_t)salvage->leadingZeroByte<<8);
                if (unitKind==UINT16_MAX) continue;
                uint16_t condition=salvage->conditionPercent|((uint16_t)salvage->trailingZeroByte<<8);
                uint16_t valueThousands;
                if (condition<=SALVAGE_SCRAP_CONDITION_LIMIT)
                {
                    if (unitKind>=CAMPAIGN_MECH_KIND_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: unitKind>=CAMPAIGN_MECH_KIND_COUNT"); /* Unmapped adjoining pricing words. */
                    valueThousands=(uint8_t)campaignHandoff.campaignMechTonnages[unitKind]*SALVAGE_SCRAP_THOUSANDS_PER_TON;
                }
                else
                {
                    /*32EB overwrites DI, destroying the previous running total.
                     * Keep its market-tier result as the SAME accumulator. */
                    uint32_t value=MwCalculateUnitConditionPrice((uint8_t)condition,unitKind,&salvageThousands);
                    value/=CBILLS_PER_THOUSAND;
                    if (value>UINT16_MAX) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: value>UINT16_MAX");
                    valueThousands=(uint16_t)value;
                }
                salvageThousands+=valueThousands;
            }
            uint32_t share=(uint32_t)salvageThousands*campaignMission.acceptedContractSalvagePercent/PERCENT_DIVISOR;
            if (share>UINT16_MAX) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: share>UINT16_MAX"); /* First native WORD quotient before second MUL. */
            uint32_t paid=share*campaignHandoff.settlementPayoutPercent/PERCENT_DIVISOR;
            if (paid>UINT16_MAX) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: paid>UINT16_MAX");
            salvagePayment=paid*CBILLS_PER_THOUSAND;
        }
        uint32_t funds=((uint32_t)campaignEconomy.campaignFundsHigh<<NATIVE_WORD_BITS)|campaignEconomy.campaignFundsLow;
        funds+=salvagePayment;
        campaignEconomy.campaignFundsLow=(uint16_t)funds;campaignEconomy.campaignFundsHigh=(uint16_t)(funds>>NATIVE_WORD_BITS);
        campaignCore.numericGroupingEnabled=1;
        MwDrawSignedDecimalField(salvagePayment,AFTER_ACTION_PAYMENT_X,1,0,
            AFTER_ACTION_FOREGROUND,AFTER_ACTION_BACKGROUND,AFTER_ACTION_SALVAGE_Y);
        uint32_t contractPayment=0;
        if (campaignMission.combatTransitionResult==0)
        {
            uint32_t paymentThousands=(uint32_t)campaignMission.remainingContractPaymentThousands*campaignHandoff.settlementPayoutPercent/PERCENT_DIVISOR;
            if (paymentThousands>UINT16_MAX) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: paymentThousands>UINT16_MAX");
            contractPayment=paymentThousands*CBILLS_PER_THOUSAND;
        }
        funds=((uint32_t)campaignEconomy.campaignFundsHigh<<NATIVE_WORD_BITS)|campaignEconomy.campaignFundsLow;
        funds+=contractPayment;
        campaignEconomy.campaignFundsLow=(uint16_t)funds;campaignEconomy.campaignFundsHigh=(uint16_t)(funds>>NATIVE_WORD_BITS);
        campaignCore.numericGroupingEnabled=1;
        MwDrawSignedDecimalField(contractPayment,AFTER_ACTION_PAYMENT_X,1,0,
            AFTER_ACTION_FOREGROUND,AFTER_ACTION_BACKGROUND,AFTER_ACTION_PAYMENT_Y);
    }
    /*1FB9..2081. Player loss skips ALL roster removal/progression/time. */
    if (campaignMechanics.rosterStatus[0]==1) goto postCombatEvents;
    while (pilotRosterCount!=1 && campaignMechanics.rosterStatus[1]!=0)
    {
        MwStartDeathMusic();
        (void)MwRemovePilotAndCompactRoster(FIRST_HIRED_PILOT_SLOT);
    }
    if (pilotRosterCount!=1)
    {
        while (pilotRosterCount!=2 && campaignMechanics.rosterStatus[2]!=0)
        {
            MwStartDeathMusic();
            (void)MwRemovePilotAndCompactRoster(2);
        }
        if (pilotRosterCount!=2 && pilotRosterCount!=3 && campaignMechanics.rosterStatus[3]!=0)
        {
            MwStartDeathMusic();
            (void)MwRemovePilotAndCompactRoster(LAST_PILOT_CARD_SLOT);
        }
    }
    campaignEconomy.playerCombatCredit+=campaignMission.returnedDisabledEnemyCount;
    uint16_t reputation;reputation=0;
    if (campaignEconomy.playerCombatCredit>=COMMANDER_REPUTATION_SECOND_THRESHOLD)
    {
        ++reputation;
        if (campaignEconomy.playerCombatCredit>=COMMANDER_REPUTATION_THIRD_THRESHOLD)
        {
            ++reputation;
            if (campaignEconomy.playerCombatCredit>=COMMANDER_REPUTATION_FOURTH_THRESHOLD) ++reputation;
        }
    }
    campaignMechanics.rosterPayTiers[0]=reputation;
    ++campaignMission.playerSkillProgress;
    int32_t skillProgress;skillProgress=campaignMission.playerSkillProgress;
    if (skillProgress>=0x8000) skillProgress-=0x10000; /* Native signed JGE. */
    if ((campaignMechanics.rosterGunneryTiers[0]==PILOT_SKILL_POOR && skillProgress>=PILOT_POOR_ADVANCE_COUNT)
        || (campaignMechanics.rosterGunneryTiers[0]==PILOT_SKILL_AVERAGE && skillProgress>=PILOT_AVERAGE_ADVANCE_COUNT)
        || (campaignMechanics.rosterGunneryTiers[0]==PILOT_SKILL_GOOD && skillProgress>=PILOT_GOOD_ADVANCE_COUNT))
    {
        ++campaignMechanics.rosterGunneryTiers[0];
        ++campaignMechanics.rosterPilotingTiers[0]; /* Increment independently, NOT assignment to Gunnery. */
        campaignMission.playerSkillProgress=0;
    }
    for (uint16_t rosterSlot=1;rosterSlot!=pilotRosterCount;++rosterSlot)
        MwAdvancePilotSkillFromExperience(rosterSlot);
    {
        uint16_t elapsedUnits;
        if (campaignMission.combatScenarioCode==COMBAT_SCENARIO_DARK_WING_BASE)
            elapsedUnits=SPECIAL_ASSAULT_DEPLOYMENT_UNITS;
        else
        {
            uint16_t planetIndex=(uint16_t)campaignMission.combatPlanetSelector-1;
            uint16_t distance=MwCalculatePlanetTravelDistance(planetIndex);
            uint16_t travelUnits=(uint16_t)(distance*DEPLOYMENT_TRAVEL_MULTIPLIER)<<1;
            MwAdvanceCampaignTimeAndChargeWages(travelUnits);
            planetIndex=campaignCore.currentPlanetIndex;
            if (planetIndex>=CAMPAIGN_PLANET_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: planetIndex>=CAMPAIGN_PLANET_COUNT");
            uint16_t recordOffset=campaignCore.planetRecordOffsets[planetIndex];
            if (recordOffset<PLANET_RECORDS_START || (uint32_t)recordOffset+sizeof(MwPlanetHeader)>PLANET_RECORDS_END) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: recordOffset<PLANET_RECORDS_START || (uint32_t)recordOffset+sizeof(MwPlanetHeader)>PLANET_RECORDS_END");
            const MwPlanetHeader *planet=(const MwPlanetHeader *)&campaignCore.planetRecordStorage[recordOffset-PLANET_RECORDS_START];
            MwAdvanceCampaignTimeAndChargeWages((uint16_t)(planet->travelTimeWeight<<2));
            planetIndex=(uint16_t)((uint16_t)campaignMission.combatPlanetSelector-1); /* Re-read after the time call. */
            if (planetIndex>=CAMPAIGN_PLANET_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: planetIndex>=CAMPAIGN_PLANET_COUNT");
            recordOffset=campaignCore.planetRecordOffsets[planetIndex];
            if (recordOffset<PLANET_RECORDS_START || (uint32_t)recordOffset+sizeof(MwPlanetHeader)>PLANET_RECORDS_END) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: recordOffset<PLANET_RECORDS_START || (uint32_t)recordOffset+sizeof(MwPlanetHeader)>PLANET_RECORDS_END");
            planet=(const MwPlanetHeader *)&campaignCore.planetRecordStorage[recordOffset-PLANET_RECORDS_START];
            MwAdvanceCampaignTimeAndChargeWages((uint16_t)(planet->travelTimeWeight<<1));
            if (campaignMission.combatScenarioCode>=DEPLOYMENT_SCENARIO_TIME_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: combatScenarioCode>=DEPLOYMENT_SCENARIO_TIME_COUNT");
            elapsedUnits=campaignHandoff.missionDeploymentUnitsByScenario[campaignMission.combatScenarioCode];
        }
        MwAdvanceCampaignTimeAndChargeWages(elapsedUnits);
    }
    /* Explicit refreshes remain even if an earlier time charge refreshed too. */
    MwRefreshAvailablePilotRecruits();
    MwRefreshMarketUnitStock();
    MwRefreshMissionOffers();
    EgaSelectFont(0);
    {
        const char *outcome;
        if (campaignMission.combatScenarioCode==COMBAT_SCENARIO_DARK_WING_BASE)
            outcome=campaignHandoff.darkWingAttackFailedText;
        else
        {
            if (campaignMission.acceptedContractTargetHouseIndex<CAMPAIGN_HOUSE_COUNT)
                campaignEconomy.houseGrievancePoints[campaignMission.acceptedContractTargetHouseIndex]+=2;
            uint16_t house=campaignCore.currentPlanetHouseIndex;
            if (house>=CAMPAIGN_HOUSE_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: house>=CAMPAIGN_HOUSE_COUNT");
            if (campaignHandoff.deploymentResolvedWithoutCombat!=0)
            {
                campaignHandoff.deploymentResolvedWithoutCombat=0;
                campaignEconomy.houseFavorPoints[house]+=2;
                outcome=campaignHandoff.deploymentUneventfulText;
            }
            else if (campaignMission.combatTransitionResult!=0)
            {
                if (campaignMission.combatTransitionResult==2) ++campaignEconomy.houseGrievancePoints[house];
                outcome=campaignHandoff.afterActionFailureMessages[house];
            }
            else
            {
                if (!campaignStartMusic) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !campaignStartMusic");
                campaignStartMusic(campaignStartup.campaignMusicBufferSelector,LAUNCH_MUSIC_OFFSET);
                campaignEconomy.houseFavorPoints[house]+=2;
                outcome=campaignHandoff.afterActionSuccessMessages[house];
            }
        }
        MwDrawCampaignTextColourOne(AFTER_ACTION_OUTCOME_X,AFTER_ACTION_OUTCOME_Y,outcome);
        MwWaitForInputAcknowledgment();
    }
    if (campaignMission.combatScenarioCode!=COMBAT_SCENARIO_DARK_WING_BASE
        && campaignMission.combatTransitionResult==0 && campaignHandoff.settlementPayoutPercent!=PERCENT_DIVISOR)
    {
        EgaFillDisplayPageRectangle(9,0x0092,0x00F7,0x0030,AFTER_ACTION_BACKGROUND);
        const char *deduction=campaignHandoff.settlementNoPaymentText;
        if (campaignHandoff.settlementPayoutPercent!=0)
            deduction=campaignHandoff.settlementDeductionMessages[MwRandomUpTo(2)];
        MwDrawCampaignTextColourOne(AFTER_ACTION_OUTCOME_X,AFTER_ACTION_OUTCOME_Y,deduction);
        MwWaitForInputAcknowledgment();
    }
postCombatEvents:
    campaignEventDispatchReturnContinuation=CAMPAIGN_CONTINUE_PLANET_HUB;
    continuation=MwDispatchCampaignEvents(6,CAMPAIGN_CALL_DISPATCH_EVENTS);
    if (!continuation)
    {campaignEventDispatchReturnContinuation=0;continuation=CAMPAIGN_CONTINUE_PLANET_HUB;}
    goto resumeCampaign;
darkWingVictory:
    campaignEvents.darkWingBaseVictoryRecorded=1;
    campaignEvents.eventSeenFlags[EVENT_DARK_WING_BASE_ATTACK]=1;
    campaignEvents.eventSeenFlags[EVENT_PORT_ARREST]=1;
    MwDrawCampaignTextColourOne(0x000E,0x008C,campaignHandoff.darkWingVictoryText);
    EgaCopyDisplayToDrawPage(0,0,EGA_COLUMNS,EGA_ROWS);
    MwDrawIndexedPicture(0,0,VICTORY_TRANSITION_PICTURE);
    EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
    (void)MwLoadFPictureBank();
    (void)MwLoadVictoryMusic();
    if (!campaignStartMusic) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !campaignStartMusic");
    campaignStartMusic(campaignStartup.campaignMusicBufferSelector,0);
    MwDrawActivePicture(0,0x003B,EGA_DRAW_PAGE,9);
    MwDrawActivePicture(0x007C,0x002D,EGA_DRAW_PAGE,1);
    EgaFlipPages();
    MwDrawVictoryTransitionPictureAndWait();
    MwDrawActivePicture(0x006D,0x0026,EGA_DRAW_PAGE,2);
    EgaFlipPages();
    MwDrawVictoryTransitionPictureAndWait();
    MwDrawActivePicture(0,0x003B,EGA_DRAW_PAGE,9);
    MwDrawActivePicture(0x005E,0x001F,EGA_DRAW_PAGE,3);
    EgaFlipPages();
    MwDrawVictoryTransitionPictureAndWait();
    MwDrawActivePicture(0x0051,0x0019,EGA_DRAW_PAGE,4);
    EgaFlipPages();
    MwDrawVictoryTransitionPictureAndWait();
    MwDrawActivePicture(0,0x003B,EGA_DRAW_PAGE,9);
    MwDrawActivePicture(0x003A,0x000E,EGA_DRAW_PAGE,5);
    EgaFlipPages();
    MwDrawVictoryTransitionPictureAndWait();
    for (uint16_t cycle=0;cycle<VICTORY_ANIMATION_CYCLES;++cycle)
    {
        MwDrawActivePicture(0x0027,0x003E,EGA_DRAW_PAGE,6);
        MwDrawActivePicture(0x0091,8,EGA_DRAW_PAGE,7);
        EgaFlipPages();
        MwDrawVictoryTransitionPictureAndWait();
        MwDrawActivePicture(0,0x003B,EGA_DRAW_PAGE,9);
        MwDrawActivePicture(0x0027,0x003E,EGA_DRAW_PAGE,6);
        MwDrawActivePicture(0x0091,8,EGA_DRAW_PAGE,8);
        EgaFlipPages();
        MwDrawVictoryTransitionPictureAndWait();
    }
    MwWaitForInputAcknowledgment();
    continuation=MwDispatchFixedCampaignEvent();
    if (continuation) goto resumeCampaign;
finishVictory:
    campaignEventReturnContinuation=0;
    (void)MwLoadPrimaryPictureBank();
    (void)MwLoadBarDeathAndLaunchMusic();
    continuation=CAMPAIGN_CONTINUE_PLANET_HUB;
    goto resumeCampaign;
campaignExit:
    /*015F..0178 remains part of0002, not a manufactured exit procedure. */
    MwIndexedPictureCloseHook();
    if (campaignStartup.launcherExitCode==CAMPAIGN_EXIT_QUIT) MwRestoreSavedVideoMode();
    MwRestoreCampaignInterruptHandlers();
    if (!mwCampaignInput.keyboardAvailable || !mwCampaignInput.readKeyboard) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mwCampaignInput.keyboardAvailable || !mwCampaignInput.readKeyboard");
    if (mwCampaignInput.keyboardAvailable()) (void)mwCampaignInput.readKeyboard(); /* ONE pending BIOS key, not a drain. */
    goto finish;
allocationFailed:
    /* The original printed one of seven DOS messages (DS:017D) and waited for
     * a key. Here a startup failure stops the program with its reason. */
    MwRestoreSavedVideoMode();
    {
        char reason[64];
        snprintf(reason,sizeof(reason),"the campaign could not start (startup step %u)",(unsigned)errorIndex);
        MwAbortWithReason("startup",reason,__FILE__,__LINE__,__func__);
    }
finish:
    if (!campaignStopMusic) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !campaignStopMusic");
    campaignStopMusic();
    return campaignStartup.launcherExitCode;
}

/* MW_MAIN081B:019A..0209 /020A..0261. The original saves/restores four
 * interrupt vectors. Actual interrupt ownership belongs to the host boundary;
 * do not expose DOS handler addresses as game state or emulate an IVT. */
void MwInstallCampaignInterruptHandlers(void)
{
    if (!mwCampaignPlatform.installInterrupts) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mwCampaignPlatform.installInterrupts");
    mwCampaignPlatform.installInterrupts();
}
void MwRestoreCampaignInterruptHandlers(void)
{
    if (!mwCampaignPlatform.restoreInterrupts) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mwCampaignPlatform.restoreInterrupts");
    mwCampaignPlatform.restoreInterrupts();
}
/* MW_MAIN081B:02E6..02ED /02EE..02F5. */
void MwSaveCurrentVideoMode(void)
{
    campaignStartup.savedVideoMode=MwGetVideoMode();
}
void MwRestoreSavedVideoMode(void)
{
    MwSetVideoMode(campaignStartup.savedVideoMode);
}
/* MW_MAIN081B:245D..2478. The native saves no game arguments: selector and
 * DEATH.MUS offset are loaded here; DS is restored to1090 after FAR0186. */
void MwStartDeathMusic(void)
{
    if (!campaignStartMusic) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !campaignStartMusic");
    campaignStartMusic(campaignStartup.campaignMusicBufferSelector,DEATH_MUSIC_OFFSET);
}
/* MW_MAIN081B:0C0A..0C16. BIOS CX:DX ticks mix into BOTH native RNG words. */
void MwMixBiosTicksIntoRandomState(void)
{
    if (!mwCampaignPlatform.readBiosTicks) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mwCampaignPlatform.readBiosTicks");
    uint32_t ticks=mwCampaignPlatform.readBiosTicks();
    campaignCore.randomStateWord0^=(uint16_t)(ticks>>16);
    campaignCore.randomStateWord1^=(uint16_t)ticks;
}
/* MW_MAIN081B:05FE..0640. A mouse if the host has one; a joystick if one is
 * connected. The original reset INT 33h (FFFFh meaning a mouse) and counted
 * game-port reads until both of joystick A's axes discharged; H3 asks the
 * host instead, so those reads and their CLI/STI are gone. */
void MwDetectMouseAndJoystick(void)
{
    campaignInput.mouseInstalled=0;campaignInput.joystickInstalled=0;
    if (!mwCampaignPlatform.mousePresent || !mwCampaignInput.readJoystick) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mwCampaignPlatform.mousePresent || !mwCampaignInput.readJoystick");
    if (mwCampaignPlatform.mousePresent()) campaignInput.mouseInstalled=1;
    if (mwCampaignInput.readJoystick().connected) campaignInput.joystickInstalled=1;
}
/* MW_MAIN081B:03DD..03F6. The apparently empty mouse branch really is empty. */
void MwDetectAndCalibrateInputDevices(void)
{
    MwDetectMouseAndJoystick();
    campaignStartup.keyboardInterruptState=0;
    if (campaignInput.joystickInstalled) MwCalibrateJoystickA();
}

/* MW_MAIN081B:136C entry falls136F;1375 is its successful-open shared entry.
 * Carry is the logical result. Successful close's error is explicitly ignored;
 * a read failure leaves the handle open and partial bytes intact. */
bool MwLoadMechMusic(void)
{return MwOpenAndLoadMusicFile("MECH.MUS");}
/* Native139B JMP136F retains the same read/close/error behavior. */
bool MwLoadVictoryMusic(void)
{
    return MwOpenAndLoadMusicFile("VICTORY.MUS");
}
/* One file into the music buffer. A read asks for FFFF bytes; the music files
 * are short. A failed close is ignored. */
static bool LoadMusicFileAt(const char *filename,uint16_t bufferOffset)
{
    mw1o::files::GameFile file=mw1o::files::open_game_file(filename);
    if (!file || !file.read(MwResolveNativeSegment(campaignStartup.campaignMusicBufferSelector)+bufferOffset,UINT16_MAX)) return true;
    (void)file.close();
    return false;
}
bool MwOpenAndLoadMusicFile(const char *filename)
{
    return LoadMusicFileAt(filename,0);
}
/* MW_MAIN081B:15EA..160F. Ten strings, fixed X and thirteen-pixel row step.
 * Native renderer returns DI on each NUL; the following INC skips it. */
void MwDrawIntroNarrationLines(uint8_t colour)
{
    uint16_t y=0x002C;
    for (uint16_t line=0;line<10;++line)
    {
        MwText text=MwArrayText(campaignStartup.introNarrationLines[line]);
        (void)EgaDrawTransparentText(0x002B,y,colour,text);
        y+=0x000D;
    }
}
/* MW_MAIN081B:2442..245C. Deadline is sampled BEFORE drawing. Unsigned JA
 * against an absolute WORD tick is intentional: wrapping deadlines can finish
 * immediately. The host IRQ boundary delivers time, not input/RNG polling. */
void MwDrawVictoryTransitionPictureAndWait(void)
{
    uint16_t deadline=campaignInput.campaignTimerTicks+VICTORY_TRANSITION_TICKS;
    MwDrawIndexedPicture(0,0,VICTORY_TRANSITION_PICTURE);
    while (deadline>campaignInput.campaignTimerTicks)
    {
        if (!mwCampaignInput.waitForInterrupt) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mwCampaignInput.waitForInterrupt");
        mwCampaignInput.waitForInterrupt();
    }
}
/* MW_MAIN081B:1610..1624. Exact WORD deadline equality, not >= or elapsed time.
 * Even delay0 polls once and consumes the input/RNG side effects. */
bool MwWaitTicksOrInputInterruption(uint16_t delayTicks)
{
    uint16_t deadline=campaignInput.campaignTimerTicks+delayTicks;
    do
    {
        if (MwPollInputOnce()) return true;
    } while (campaignInput.campaignTimerTicks!=deadline);
    return false;
}
/* MW_MAIN081B:1625..162D. BIOS palette register2, BH supplies its code. */
void MwSetIntroTextPaletteColour(uint8_t paletteCode)
{
    mw1o::video::GameCanvas().setPaletteEntry(Campaign_Pal_EGA_White,mw1o::video::PaletteCodeColour(paletteCode));
}
/* MW_MAIN081B:162E..1643. Original record has X/Y then NUL text, no colour. */
void MwDrawPositionedIntroText(const MwIntroPositionedText *text)
{
    MwText view=MwArrayText(text->text);
    (void)EgaDrawTransparentText(text->x,text->y,Campaign_Pal_EGA_White,view);
}
/* MW_MAIN081B:1644..166C.166D result below transports the POP-IP/JMP path;
 * successful native return has no AX result contract. Do not seed this host
 * return value or the typed record pointers as an invented native ABI. */
uint16_t MwDrawTimedPictureAndTextTransition(uint16_t delayTicks,
    const MwIntroPositionedText *leftText,const MwIntroPositionedText *rightText)
{
    MwDrawPositionedIntroText(leftText);
    MwDrawPositionedIntroText(rightText);
    if (MwWaitTicksOrInputInterruption(delayTicks)) return CAMPAIGN_CONTINUE_NEW_CAMPAIGN;
    EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
    return 0;
}

/* MW_MAIN081B:13A0..1422. Each read requestsFFFF from its native offset;
 * actual original files are short. Do not replace these with three independent
 * allocations: DEATH/LAUNCH can overlap bytes loaded by an earlier read. */
bool MwLoadBarDeathAndLaunchMusic(void)
{
    return LoadMusicFileAt("MECHBAR.MUS",0) || LoadMusicFileAt("DEATH.MUS",0x2328) || LoadMusicFileAt("LAUNCH.MUS",0x2AF8);
}

/* MW_MAIN081B:166D..1908, with the shared company entry181A. From166D the
 * new campaign draws picture2, loads the primary picture bank and swaps to
 * the launch music. The original's copy protection (16A4..1819: a mech
 * picture, a callsign question and an accepted panel) is dropped; the company
 * is set up as from181A. */
uint16_t MwStartNewCampaign(uint16_t entryOffset)
{
    if (entryOffset==CAMPAIGN_CONTINUE_NEW_CAMPAIGN)
    {
        MwDrawActivePicture(0,0,EGA_DRAW_PAGE,2);
        EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
        (void)MwLoadPrimaryPictureBank();
        if (!campaignStopMusic) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !campaignStopMusic");
        campaignStopMusic();
        (void)MwLoadBarDeathAndLaunchMusic();
    }
    else if (entryOffset!=CAMPAIGN_CONTINUE_INITIALIZE_COMPANY) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: entryOffset is neither new-campaign entry"); /* Only the two proven shared entries. */
    /* The start-world check (event_log.h): the 18.2 Hz count and the random
     * words the start world is drawn from, kept before the draw. */
    const uint16_t startTicks=campaignInput.campaignTimerTicks,startWord0=campaignCore.randomStateWord0,startWord1=campaignCore.randomStateWord1;
    campaignStartup.campaignEventPlanetSelectors[0]=campaignStartup.eventPlanetChoicePool0[MwRandomUpTo(7)];
    campaignCore.currentPlanetIndex=campaignStartup.campaignEventPlanetSelectors[0]-1;
    MwEventLogCampaignStart(startTicks,startWord0,startWord1,campaignCore.currentPlanetIndex);
    MwApplyCurrentPlanetState();
    campaignStartup.campaignEventPlanetSelectors[1]=campaignStartup.eventPlanetChoicePool1[MwRandomUpTo(2)];
    campaignStartup.campaignEventPlanetSelectors[2]=campaignStartup.eventPlanetChoicePool2[MwRandomUpTo(3)];
    campaignStartup.campaignEventPlanetSelectors[3]=campaignStartup.eventPlanetChoicePool3[MwRandomUpTo(2)];
    campaignStartup.campaignEventPlanetSelectors[4]=campaignStartup.eventPlanetChoicePool4[MwRandomUpTo(3)];
    campaignStartup.campaignEventPlanetSelectors[5]=campaignStartup.eventPlanetChoicePool5[MwRandomUpTo(2)];
    MwInitializePilotRatingsFromDefinitions();
    MwRefreshMarketUnitStock();
    MwRefreshAvailablePilotRecruits();
    uint32_t startupCashBonus=(uint32_t)MwRandomUpTo(0x00FF)*0x03E8;
    uint32_t companyFunds=(((uint32_t)campaignEconomy.campaignFundsHigh<<NATIVE_WORD_BITS)|campaignEconomy.campaignFundsLow)+startupCashBonus;
    campaignEconomy.campaignFundsHigh=(uint16_t)(companyFunds>>NATIVE_WORD_BITS);
    campaignEconomy.campaignFundsLow=(uint16_t)companyFunds;
    campaignEconomy.generateStartingJenner=STOCK_FORCE_STARTER_MECH;
    MwGenerateUnitStock(0,0,0,4);
    campaignEconomy.ownedUnitCount=1;
    campaignEconomy.ownedUnitKinds[0]=campaignEconomy.generatedUnitKind;
    campaignMechanics.rosterMechAssignments[0]=0;
    /* Both29-byte records are disjoint ordinary objects with no callbacks or
     * writers between native byte copies. Other company slots are untouched. */
    campaignMechanics.ownedUnitDamage[0]=campaignMechanics.generatedMechDamage;
    for (uint16_t group=0;group<CAMPAIGN_AMMO_GROUPS;++group)
    {
        campaignEconomy.ownedAmmoKindsByGroup[group*CAMPAIGN_OWNED_UNIT_SLOTS]=campaignEconomy.generatedAmmoKinds[group];
        campaignEconomy.ownedAmmoQuantitiesByGroup[group*CAMPAIGN_OWNED_UNIT_SLOTS]=campaignEconomy.generatedAmmoQuantities[group];
    }
    MwRefreshMissionOffers();
    return CAMPAIGN_CONTINUE_PLANET_HUB; /*1905 PUSH AX/1906 JMP; not a native AX result. */
}
