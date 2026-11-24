#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_scene.h"
#include "mechwarrior/combat_world.h"
#include "mechwarrior/combat_platform.h"
#include "mechwarrior/combat_runtime.h"
#include <stdlib.h>
#include <string.h>

/* BTECH0810:2C40..34A7. One native procedure, including shared3448 world join.
 * The host-only seed projects the preceding sound loader's OBSERVABLE local
 * alias; it is not a native source argument or a new304E field. */
void BtechPrepareCampaignCombatStage(uint16_t inheritedSoundStreamIdentity)
{
    BtechCampaignImport company;
    BtechDecodeCampaignImport(campaignCombatExchange,&company);
    if (combatImport.combatContinuationStage&0x8000)
    {
        for (uint16_t character=0;character<8;++character)
            if (company.commanderName[character]!=combatImport.combatDefaultCommanderNameBytes[character])
                BtechTerminateToLauncher(1);
        memset(campaignCombatExchange+0x000D,0xFF,0x10);
        if ((uint8_t)company.gunneryTiers[0]>1) BtechIncreaseTemplateLegIntegrity();
        uint8_t command=' ';
        if (!(combatImport.savedArgumentCount&0x8000) && combatImport.savedArgumentCount>2)
        {
            if (!combatImport.savedArguments || !combatImport.savedArguments[2]) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !savedArguments || !savedArguments[2]");
            command=BtechLowercaseCharacterByClassTable((int8_t)combatImport.savedArguments[2][0]);
        }
        recordCombatInputRequested=command=='r';combatImport.playbackCombatInputRequested=command=='p';
        BtechWaitScaledRuntimeDelay(COMBAT_PREPARE_DELAY);
        BtechCalibrateJoystickAxisPairs();
        uint16_t checksum=inheritedSoundStreamIdentity;
        for (uint16_t offset=0;offset<COMBAT_CHECKSUM_BYTES;++offset) checksum+=campaignCombatExchange[offset];
        BtechMixRandomStateWords(checksum);
        combatImport.reducedMemoryMode=company.memoryTier==COMBAT_MEMORY_TIER_MINIMUM;
        combatImport.combatTerrainDetailLevel=company.terrainDetailLevel;
        combatImport.combatMissionDefinitionIndex=(uint16_t)(company.scenarioCode-1);
        if (company.scenarioCode==COMBAT_SCENARIO_DARK_WING_BASE)
        {combatMechs.specialAssaultMode=1;combatContinuationProfileIndex=8;combatImport.combatMissionDefinitionIndex=COMBAT_PLANETARY_ASSAULT;}
    }
    combatImport.combatSoundEnabled=company.soundDisabled==0;
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot) combatMechs.mechStates[slot]=BtechMechState{};
    /*29CA is the genuine XOR AX,AX/RET. Its short-circuited replay predicate
     * can never select this join; no invented successful service is called. */
    if (combatImport.combatMissionDefinitionIndex==COMBAT_NO_ASSIGNMENT) goto loadWorld;
    if (combatImport.savedArgumentCount>3 && (combatImport.combatContinuationStage&0x8000))
    {
        if (recordCombatInputRequested || combatImport.playbackCombatInputRequested)
        {
            if (!combatImport.savedArguments || !combatImport.savedArguments[3]) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !savedArguments || !savedArguments[3]");
            BtechConfigureCombatInputReplay(recordCombatInputRequested?COMBAT_INPUT_RECORD:COMBAT_INPUT_PLAYBACK,combatImport.savedArguments[3]);
        }
    }
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {combatImport.gunneryTargetNumbers[slot]=COMBAT_DEFAULT_SKILL_PARAMETER;combatImport.pilotingRecoveryBounds[slot]=COMBAT_DEFAULT_SKILL_PARAMETER;}
    if (combatImport.combatMissionDefinitionIndex==COMBAT_EXTENDED_OFFENSIVE || combatImport.combatMissionDefinitionIndex==COMBAT_EXTENDED_DEFENSIVE
        || combatImport.combatMissionDefinitionIndex==COMBAT_EXTENDED_SIEGE || !(combatImport.combatContinuationStage&0x8000) || combatMechs.specialAssaultMode)
    {
        ++combatImport.combatContinuationStage;
        if (!combatImport.combatContinuationStage && !combatMechs.specialAssaultMode) combatContinuationProfileIndex=BtechRandomBelowBound(8);
        if (combatContinuationProfileIndex>=COMBAT_STAGE_PROFILES || combatImport.combatContinuationStage>=COMBAT_STAGES_PER_PROFILE) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: combatContinuationProfileIndex>=COMBAT_STAGE_PROFILES || combatContinuationStage>=COMBAT_STAGES_PER_PROFILE");
        combatImport.combatMissionDefinitionIndex=combatImport.combatContinuationProfiles[combatContinuationProfileIndex].missionCodes[combatImport.combatContinuationStage]-1;
    }
    combatImport.specialTargetInteractionBlocked=(!(combatImport.combatMissionDefinitionIndex&0x8000)
        && ((combatImport.combatMissionDefinitionIndex>=0x000A && combatImport.combatMissionDefinitionIndex<=0x000D) || combatImport.combatMissionDefinitionIndex>0x0022));
    if (combatImport.combatMissionDefinitionIndex>=COMBAT_MISSION_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: combatMissionDefinitionIndex>=COMBAT_MISSION_COUNT");
    combatImport.sideObjectiveRules[0]=combatMissionSideZeroRules[combatImport.combatMissionDefinitionIndex];
    if (combatImport.sideObjectiveRules[0]>=5) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: sideObjectiveRules[0]>=5");
    combatImport.sideObjectiveRules[1]=combatImport.opposingObjectiveRules[combatImport.sideObjectiveRules[0]];
    combatImport.combatLandscapeIndex=company.landscapeIndex;
    if (combatImport.combatLandscapeIndex>2) combatImport.combatLandscapeIndex=2;
    if (!(combatImport.combatContinuationStage&0x8000) && combatImport.combatContinuationStage>=1
        && combatContinuationProfileIndex!=4 && combatContinuationProfileIndex!=5)
    {
        uint16_t nextScenario=combatImport.combatScenarioIndex+1;
        int32_t signedScenario=nextScenario<0x8000?nextScenario:(int32_t)nextScenario-0x10000;
        combatImport.combatScenarioIndex=(uint16_t)(signedScenario%COMBAT_SCENARIO_COUNT);
    }
    else combatImport.combatScenarioIndex=company.planetSelector%COMBAT_SCENARIO_COUNT;
    uint16_t deployedFriendlyCount;deployedFriendlyCount=0;
    for (uint16_t rosterSlot=0;rosterSlot<COMBAT_LANCE_SLOTS;++rosterSlot)
    {
        uint16_t ownedUnitSlot=company.ownedMechSlots[rosterSlot];
        if (ownedUnitSlot==COMBAT_NO_ASSIGNMENT) continue;
        if (ownedUnitSlot>=12) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: ownedUnitSlot>=12");
        combatImport.combatToPilotRosterSlots[deployedFriendlyCount]=rosterSlot;
        combatImport.combatToOwnedMechSlots[deployedFriendlyCount]=ownedUnitSlot;
        uint16_t unitKind=company.ownedUnitKinds[ownedUnitSlot];
        if (unitKind>=10) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: unitKind>=10");
        uint16_t templateIndex=combatImport.combatTemplateByCampaignUnitKind[unitKind];
        if (templateIndex>=COMBAT_MECH_TEMPLATES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: templateIndex>=COMBAT_MECH_TEMPLATES");
        BtechMechState *mech=BtechClaimFreeMechStateSlot(1);
        if (!mech) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mech"); /* Native would write unrelated304E:0000; not a successful claim. */
        const BtechMechTemplate *chassis=&combatMechs.mechTemplates[templateIndex];
        const CampaignUnitDamage *damage=&company.ownedDamage[ownedUnitSlot];
        combatImport.gunneryTargetNumbers[deployedFriendlyCount]=CAMPAIGN_SKILL_TO_COMBAT_BASE-(uint8_t)company.gunneryTiers[rosterSlot];
        combatImport.pilotingRecoveryBounds[deployedFriendlyCount]=CAMPAIGN_SKILL_TO_COMBAT_BASE-(uint8_t)company.pilotingTiers[rosterSlot];
        if (rosterSlot) combatImport.deployedPilotIds[deployedFriendlyCount]=(uint8_t)company.pilotIds[rosterSlot];
        mech->templateIndex=templateIndex;mech->engineHits=damage->engineHits;
        if (damage->sensorDamageTier>=4 || damage->gyroDamageTier>=4 || damage->lifeSupportDamageTier>=4) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: damage->sensorDamageTier>=4 || damage->gyroDamageTier>=4 || damage->lifeSupportDamageTier>=4");
        mech->sensorHits=combatImport.systemHitsByDamageTier[damage->sensorDamageTier];
        mech->gyroHits=combatImport.systemHitsByDamageTier[damage->gyroDamageTier];
        mech->lifeSupportHits=combatImport.systemHitsByDamageTier[damage->lifeSupportDamageTier];
        mech->destroyedHeatSinks=damage->destroyedHeatSinks;
        for (uint16_t actuator=0;actuator<COMBAT_ACTUATORS;++actuator)
        {
            uint8_t tier=damage->actuatorDamageTiers[actuator];if (tier>=4) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: tier>=4");
            mech->actuatorCondition[actuator]=COMBAT_ACTUATOR_CONDITION_FULL-combatImport.actuatorLossByDamageTier[tier];
        }
        mech->destroyedJumpJetGroups=damage->destroyedJumpJetGroups;
        if (chassis->jumpJetGroupCount) mech->jumpCharge=COMBAT_JUMP_CHARGE_CAPACITY;
        for (uint16_t mount=0;chassis->weaponMountCount<0x8000 && mount<chassis->weaponMountCount;++mount)
        {
            if (mount>=COMBAT_WEAPON_MOUNTS || damage->weaponDamageTiers[mount]>=4) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mount>=COMBAT_WEAPON_MOUNTS || damage->weaponDamageTiers[mount]>=4");
            mech->weapons[mount].operational=(uint8_t)(1-(uint8_t)combatImport.weaponDisabledByDamageTier[damage->weaponDamageTiers[mount]]);
        }
        for (uint16_t armor=0;armor<COMBAT_ARMOR_LOCATIONS;++armor)
        {
            uint16_t body=combatImport.armorToBodyLocations[armor],tier=combatImport.campaignBodyTierByArmorLocation[armor];
            if (body>=COMBAT_BODY_LOCATIONS || tier>=9) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: body>=COMBAT_BODY_LOCATIONS || tier>=9");
            uint16_t integrity=CAMPAIGN_FULL_BODY_INTEGRITY-damage->bodyDamageTiers[tier];
            uint16_t maximumStructure=chassis->maximumInternalStructure[body];
            uint16_t maximumSum=chassis->maximumArmor[armor]+maximumStructure;
            uint16_t product=(uint32_t)maximumSum*integrity;
            int32_t remaining=(product<0x8000?product:(int32_t)product-0x10000)/CAMPAIGN_FULL_BODY_INTEGRITY;
            int32_t structure=maximumStructure<0x8000?maximumStructure:(int32_t)maximumStructure-0x10000;
            if (remaining>structure)
            {mech->currentInternalStructure[body]=maximumStructure;mech->currentArmor[armor]=(uint16_t)(remaining-structure);}
            else {mech->currentInternalStructure[body]=(uint16_t)remaining;mech->currentArmor[armor]=0;}
        }
        if (!mech->currentInternalStructure[COMBAT_BODY_LEFT_LEG]) mech->currentInternalStructure[COMBAT_BODY_LEFT_LEG]=1;
        if (!mech->currentInternalStructure[COMBAT_BODY_RIGHT_LEG]) mech->currentInternalStructure[COMBAT_BODY_RIGHT_LEG]=1;
        /* Native passes the ROSTER index, NOT the compact claim slot, to4163. */
        if (!BtechIsMechOperational(mech) && rosterSlot) BtechClearUnitStateBinding(rosterSlot);
        else ++deployedFriendlyCount;
        uint16_t packedGroup=0;
        for (uint16_t family=0;family<COMBAT_AMMO_FAMILIES;++family)
            if (chassis->ammoBodyLocations[family]!=-1)
            {
                if (packedGroup>=6) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: packedGroup>=6");
                mech->ammoSalvosRemaining[family]=(uint8_t)company.ammoQuantities[packedGroup++][ownedUnitSlot];
            }
        uint16_t operationalGroups=chassis->jumpJetGroupCount-mech->destroyedJumpJetGroups;
        for (uint16_t group=0;operationalGroups<0x8000 && group<operationalGroups;++group)
        {if (group>=COMBAT_JET_GROUPS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: group>=COMBAT_JET_GROUPS");mech->jumpJetGroupOperational[group]=1;}
    }
    combatImport.deployedPilotIds[0]=0;reservedMechTemplateCount=0;enemyMechPreparationCount=0;
    uint32_t availableBytes;availableBytes=BtechAllocateDosMemoryBlock(UINT32_MAX).availableBytes;
    availableBytes-=combatImport.hudColourSchemeIndex?0x00014050:0x00019640;
    mechTemplateBudgetBytesLow=(uint16_t)availableBytes;
    mechTemplateBudgetBytesHigh=(uint16_t)(availableBytes>>16);
    for (uint16_t slot=1;slot<COMBAT_LANCE_SLOTS;++slot)
        if (combatMechs.unitBindings[slot].mech) (void)BtechTryReserveMechTemplate(combatMechs.unitBindings[slot].mech->templateIndex);
    if (combatMechs.specialAssaultMode)
    {
        (void)BtechPrepareEnemyMech(COMBAT_TEMPLATE_BATTLEMASTER);
        (void)BtechPrepareEnemyMech(COMBAT_TEMPLATE_BATTLEMASTER);
        (void)BtechPrepareEnemyMech(COMBAT_TEMPLATE_BATTLEMASTER);
        (void)BtechPrepareEnemyMech(COMBAT_TEMPLATE_WARHAMMER);
        for (uint16_t slot=4;slot<8;++slot) {combatImport.gunneryTargetNumbers[slot]=0;combatImport.pilotingRecoveryBounds[slot]=0;}
        combatScene.distanceSortObjects[0]=NULL;combatImport.activeCockpitLayoutIndex=0; /* Fifth native pair aliases these owners. */
    }
    else
    {
        uint16_t heavy=company.enemyHeavyCount,medium=company.enemyMediumCount,light=company.enemyLightCount;
        if (!(combatImport.combatContinuationStage&0x8000))
        {
            uint16_t count=heavy-combatImport.combatContinuationStage+2;heavy=(uint16_t)((count<0x8000?count:(int32_t)count-0x10000)/3);
            count=medium-combatImport.combatContinuationStage+2;medium=(uint16_t)((count<0x8000?count:(int32_t)count-0x10000)/3);
            count=light-combatImport.combatContinuationStage+2;light=(uint16_t)((count<0x8000?count:(int32_t)count-0x10000)/3);
        }
        uint16_t failed=0;
        while (!failed && (uint16_t)(heavy+medium+light)!=0)
        {
            if (heavy) {--heavy;if (!BtechPrepareEnemyMech(((uint8_t)BtechRandomBelowBound(0x80)&0x0C)/4+4)) failed=1;}
            if (medium && !failed) {--medium;if (!BtechPrepareEnemyMech(((uint8_t)BtechRandomBelowBound(0x80)&4)/4+2)) failed=1;}
            if (light && !failed) {--light;if (!BtechPrepareEnemyMech(((uint8_t)BtechRandomBelowBound(0x80)&4)/4)) failed=1;}
        }
    }
loadWorld:
    BtechLoadScenarioAndWorld(combatImport.combatScenarioIndex);
    for (uint16_t slot=1;slot<COMBAT_LANCE_SLOTS;++slot)
    {
        const BtechMechState *mech=combatMechs.unitBindings[slot].mech;
        if (!mech) continue;
        if (!mech->currentInternalStructure[COMBAT_BODY_LEFT_ARM]) combatMechs.unitBindings[slot].leftArm->object.prefix.renderFlags|=COMBAT_WORLD_OBJECT_HIDDEN;
        if (!mech->currentInternalStructure[COMBAT_BODY_RIGHT_ARM]) combatMechs.unitBindings[slot].rightArm->object.prefix.renderFlags|=COMBAT_WORLD_OBJECT_HIDDEN;
    }
}

/* BTECH0810:4112..4162. The fallen actuator gate reads CURRENT control,
 * deliberately not a control record inferred from the supplied Mech. */
uint16_t BtechIsMechOperational(const BtechMechState *mech)
{
    if ((mech->engineHits<0x8000 && mech->engineHits>=COMBAT_ENGINE_HIT_LIMIT)
        || (mech->lifeSupportHits<0x8000 && mech->lifeSupportHits>=COMBAT_LIFE_SUPPORT_HIT_LIMIT)
        || mech->cockpitDisabled || !mech->currentInternalStructure[COMBAT_BODY_LEFT_LEG]
        || !mech->currentInternalStructure[COMBAT_BODY_RIGHT_LEG]) return 0;
    /* Initial1E84 calls2C40 BEFORE setting7752=D334. Near0000 is NOT a host
     * null dereference: its+0051 reads an authored304E WORD. Retain that owner
     * until actual control records are selected; never invent a zeroed Mech. */
    uint16_t movementMode=combatImport.activeUnitControl?combatImport.activeUnitControl->movementMode:combatImport.startupControlMovementAlias;
    if (movementMode==COMBAT_MOVEMENT_FALLEN
        && (!mech->actuatorCondition[COMBAT_BODY_LEFT_LEG] || !mech->actuatorCondition[COMBAT_BODY_RIGHT_LEG])) return 0;
    return 1;
}

/* BTECH0810:3761..37F6. Exact equality is insufficient budget; signed HIGH
 * comparison precedes unsigned LOW, not a modern unsigned DWORD comparison. */
uint16_t BtechTryReserveMechTemplate(uint16_t templateIndex)
{
    uint16_t index=0;
    if (reservedMechTemplateCount && !(reservedMechTemplateCount&0x8000))
        while (reservedMechTemplateIds[index]!=templateIndex)
        {if (++index>=reservedMechTemplateCount) break;if (index>=10) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: index>=10");}
    if (index!=reservedMechTemplateCount) return 1;
    if (templateIndex>=8) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: templateIndex>=8");
    uint32_t cost=combatImport.mechTemplateBudgetCostsBytes[templateIndex];
    uint32_t budget=(uint32_t)mechTemplateBudgetBytesLow|((uint32_t)mechTemplateBudgetBytesHigh<<16);
    int32_t costHigh=cost>>16,budgetHigh=budget>>16;
    if (costHigh>=0x8000) costHigh-=0x10000;if (budgetHigh>=0x8000) budgetHigh-=0x10000;
    if (!(costHigh<budgetHigh || (costHigh==budgetHigh && (uint16_t)cost<(uint16_t)budget))) return 0;
    index=reservedMechTemplateCount++;
    if (index>=10) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: index>=10"); /* Corrupt spill beyond the represented native neighbours. */
    reservedMechTemplateIds[index]=templateIndex;
    budget=(uint32_t)mechTemplateBudgetBytesLow|((uint32_t)mechTemplateBudgetBytesHigh<<16);
    budget-=cost; /* Native reload AFTER append: append itself may alias budget. */
    mechTemplateBudgetBytesLow=(uint16_t)budget;
    mechTemplateBudgetBytesHigh=(uint16_t)(budget>>16);
    return 1;
}

/* BTECH0810:350E..3582. Fallback ring is intentionally unbounded. */
uint16_t BtechPrepareEnemyMech(uint16_t preferredTemplate)
{
    if (!(enemyMechPreparationCount&0x8000) && enemyMechPreparationCount>3) return 0;
    ++enemyMechPreparationCount;
    if (!BtechTryReserveMechTemplate(preferredTemplate))
    {
        if (preferredTemplate>=8) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: preferredTemplate>=8");
        int16_t step=combatImport.mechFallbackTemplateSteps[preferredTemplate];
        preferredTemplate=combatImport.mechFallbackStartTemplates[preferredTemplate];
        while (!BtechTryReserveMechTemplate(preferredTemplate)) preferredTemplate=(uint16_t)(preferredTemplate+step)&7;
    }
    BtechInitializeMechConditionFromTemplate(BtechClaimFreeMechStateSlot(0),preferredTemplate);
    return 1;
}
