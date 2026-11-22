#include "mechwarrior/story.h"
#include "mechwarrior/rules.h"
#include "mechwarrior/combat_mechs.h"
#include "mechwarrior/native_address.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_installation.h"
#include "mechwarrior/combat_damage.h"
#include <string.h>

uint16_t combatContinuationStage;
uint16_t salvageEntryCount;
uint16_t campaignUnitKindByCombatTemplate[COMBAT_MECH_TEMPLATES];
uint16_t actuatorIntegrityTierByCondition[5],systemDamageTierByHits[3],weaponIntegrityTierByOperational[2];
BtechMissionRules missionRules;
static_assert(offsetof(BtechMissionRules,fields.continuationProfileIndex)==0x48,"Mission36 aliases profile0974");
uint16_t reducedMemoryMode,combatTerrainDetailLevel,combatMissionDefinitionIndex,combatSoundEnabled;
uint16_t playbackCombatInputRequested,combatScenarioIndex;
uint16_t gunneryTargetNumbers[8],pilotingRecoveryBounds[8],activeCockpitLayoutIndex;
uint16_t combatToPilotRosterSlots[4],combatToOwnedMechSlots[4],deployedPilotIds[4];
BtechTemplateReservations mechTemplateReservations;
static_assert(sizeof(BtechTemplateReservations)==0x14,"Native reservation WORD aliases");
static_assert(offsetof(BtechTemplateReservations,fields.reservedCount)==0x0E,"Native reservation count alias");
uint32_t mechTemplateBudgetCostsBytes[8];
uint16_t hudColourSchemeIndex,specialTargetInteractionBlocked,sideObjectiveRules[2],combatLandscapeIndex;
BtechCombatStageProfile combatContinuationProfiles[9];
uint16_t opposingObjectiveRules[5];
uint16_t combatTemplateByCampaignUnitKind[10],weaponDisabledByDamageTier[4],systemHitsByDamageTier[4],actuatorLossByDamageTier[4];
uint16_t armorToBodyLocations[9],campaignBodyTierByArmorLocation[9];
uint16_t mechFallbackStartTemplates[8];
int16_t mechFallbackTemplateSteps[8];
uint8_t combatDefaultCommanderNameBytes[10];
uint16_t savedArgumentCount;
const char *const *savedArguments;
BtechUnitControlState *activeUnitControl;
uint16_t startupControlMovementAlias;
uint8_t playerDamageSuppressed;
uint16_t damageTransferArmorLocations[COMBAT_BODY_LOCATIONS];
uint16_t criticalTransferBodyLocations[COMBAT_BODY_LOCATIONS],actuatorIndicesByBodyLocation[4];

bool BtechImportCombatStageInitialData(const uint8_t *data,size_t byteCount)
{
    if (!BtechImportMechInitialData(data,byteCount)) return false;
    startupControlMovementAlias=MwReadNativeWord(data,0x0051);
    playerDamageSuppressed=data[0x0E1A];
    /* The rule tables come from data/rules (src/Rules), laid out as the exe's. */
    if (!MwRulesCopyWords("combat.damage_transfer",damageTransferArmorLocations,COMBAT_BODY_LOCATIONS,0)
        || !MwRulesCopyWords("combat.critical_transfer",criticalTransferBodyLocations,COMBAT_BODY_LOCATIONS,0)
        || !MwRulesCopyWords("combat.actuator_of_limb",actuatorIndicesByBodyLocation,4,0)) return false;
    if (!MwStoryCopyText("combat.commander.default_name",(char *)combatDefaultCommanderNameBytes,sizeof(combatDefaultCommanderNameBytes))) return false;
    salvageEntryCount=0; /*0976*/
    uint16_t steps[8],costs[16];
    if (!MwRulesCopyWords("combat.unit_kind_of_template",campaignUnitKindByCombatTemplate,COMBAT_MECH_TEMPLATES,0)
        || !MwRulesCopyWords("combat.actuator_integrity_by_condition",actuatorIntegrityTierByCondition,5,0)
        || !MwRulesCopyWords("combat.system_tier_by_hits",systemDamageTierByHits,3,0)
        || !MwRulesCopyWords("combat.weapon_integrity_by_operational",weaponIntegrityTierByOperational,2,0)
        || !MwRulesCopyWords("combat.body_of_armour",armorToBodyLocations,9,0)
        || !MwRulesCopyWords("combat.campaign_body_tier_of_armour",campaignBodyTierByArmorLocation,9,0)
        || !MwRulesCopyWords("combat.mission_side_zero_rules",combatMissionSideZeroRules,COMBAT_MISSION_COUNT,0)
        || !MwRulesCopyWords("combat.opposing_objective",opposingObjectiveRules,5,0)
        || !MwRulesCopyWords("combat.template_of_unit_kind",combatTemplateByCampaignUnitKind,10,0)
        || !MwRulesCopyWords("combat.weapon_disabled_by_tier",weaponDisabledByDamageTier,4,0)
        || !MwRulesCopyWords("combat.system_hits_by_tier",systemHitsByDamageTier,4,0)
        || !MwRulesCopyWords("combat.actuator_loss_by_tier",actuatorLossByDamageTier,4,0)
        || !MwRulesCopyWords("combat.fallback_start_template",mechFallbackStartTemplates,8,0)
        || !MwRulesCopyWords("combat.fallback_template_steps",steps,8,0)
        || !MwRulesCopyWords("combat.template_budget_costs",costs,16,0)) return false;
    for (uint16_t i=0;i<9;++i)
        if (!MwRulesCopyWords("combat.continuation_profiles",combatContinuationProfiles[i].missionCodes,3,i*6)) return false;
    for (uint16_t i=0;i<8;++i)
    {
        mechFallbackTemplateSteps[i]=(int16_t)(steps[i]<0x8000?steps[i]:(int32_t)steps[i]-0x10000);
        mechTemplateBudgetCostsBytes[i]=(uint32_t)costs[i*2]|((uint32_t)costs[i*2+1]<<16);
    }
    combatContinuationStage=0; /*C35A*/
    combatContinuationProfileIndex=MwReadNativeWord(data,0x0974);
    combatMissionDefinitionIndex=0; /*C96E*/
    return true;
}

BtechMechTemplate mechTemplates[COMBAT_MECH_TEMPLATES];
BtechWeaponDefinition weaponDefinitions[COMBAT_WEAPON_KINDS];
uint16_t templateMaximumCombatScores[COMBAT_MECH_TEMPLATES];
uint8_t unitVisibility[COMBAT_MECH_SLOTS][COMBAT_MECH_SLOTS];
BtechMechState mechStates[COMBAT_MECH_SLOTS];
BtechUnitBinding unitBindings[COMBAT_MECH_SLOTS];
BtechUnitControlState unitControls[COMBAT_MECH_SLOTS];
uint16_t heatAimJitterThresholds[COMBAT_HEAT_THRESHOLDS];
uint16_t heatSpeedPenalties[COMBAT_MECH_SLOTS],heatInhibitFlags[COMBAT_MECH_SLOTS];
uint16_t specialAssaultMode;
uint16_t randomStateWords[COMBAT_RANDOM_WORDS],randomWriteIndex,randomAddendIndex;

/* File decoder only. Runtime records need not inherit native unaligned WORDs
 * or DOS near references. Mech/binding/control state is initialized by native
 *206D/2C40, NOT cleared as a side effect of loading authored templates. */
bool BtechImportMechInitialData(const uint8_t *data,size_t byteCount)
{
    if (!data || byteCount<NATIVE_SEGMENT_BYTE_COUNT) return false;
    /* The rule tables come from data/rules (src/Rules), laid out as the exe's. */
    if (!MwRulesCopyWords("combat.heat_aim_jitter_thresholds",heatAimJitterThresholds,COMBAT_HEAT_THRESHOLDS,0)) return false;
    for (uint16_t kind=0;kind<COMBAT_WEAPON_KINDS;++kind)
    {
        uint16_t row[11];
        if (!MwRulesCopyWords("weapons.definitions",row,11,kind*0x16)) return false;
        BtechWeaponDefinition *weapon=&weaponDefinitions[kind];
        weapon->damageOrMissileCount=row[0];
        weapon->ammoFamily=(int16_t)row[1];
        weapon->reloadTicks=row[2];weapon->heatUnits=row[3];
        weapon->rangeClass=row[4];weapon->minimumRange=row[5];
        weapon->shortRangeBoundary=row[6];
        weapon->mediumRangeBoundary=row[7];
        weapon->longRangeBoundary=row[8];
        weapon->criticalSlots=row[9];weapon->unidentifiedField10=row[10];
    }
    for (uint16_t index=0;index<COMBAT_MECH_TEMPLATES;++index)
    {
        /* Each template is a 0xE0-byte row of the rule table (mechs.json),
         * laid out as the exe's, read here at the exe's offsets. */
        uint8_t bytes[0xE0];
        if (!MwRulesCopyBytes("mechs.templates",bytes,sizeof(bytes),index*0x00E0)) return false;
        BtechMechTemplate *mech=&mechTemplates[index];
        mech->tonnage=MwReadNativeWord(bytes,0);
        mech->displayScale=MwReadNativeWord(bytes,2);
        mech->firingMovementThreshold=MwReadNativeWord(bytes,4);
        mech->maximumSpeedScale=MwReadNativeWord(bytes,6);
        mech->jumpJetGroupCount=MwReadNativeWord(bytes,8);
        mech->jumpAcceleration=MwReadNativeWord(bytes,0x000A);
        mech->jumpSpeedLimit=MwReadNativeWord(bytes,0x000C);
        mech->unidentifiedField8=MwReadNativeWord(bytes,0x000E);
        mech->heatSinkCount=MwReadNativeWord(bytes,0x0010);
        mech->weaponMountCount=MwReadNativeWord(bytes,0x0012);
        for (uint16_t location=0;location<COMBAT_ARMOR_LOCATIONS;++location)
        {
            BtechBodyReferencePoint *point=&mech->bodyReferencePoints[location];
            point->componentA=(int16_t)MwReadNativeWord(bytes,0x0014+location*6);
            point->componentB=(int16_t)MwReadNativeWord(bytes,0x0016+location*6);
            point->componentC=(int16_t)MwReadNativeWord(bytes,0x0018+location*6);
            mech->maximumArmor[location]=MwReadNativeWord(bytes,0x00B2+location*2);
        }
        for (uint16_t mount=0;mount<COMBAT_WEAPON_MOUNTS;++mount)
        {
            BtechWeaponMount *weapon=&mech->weaponMounts[mount];
            weapon->weaponKind=(int16_t)MwReadNativeWord(bytes,0x004A+mount*8);
            weapon->bodyLocation=MwReadNativeWord(bytes,0x004C+mount*8);
            weapon->unidentifiedField4=MwReadNativeWord(bytes,0x004E +mount*8);
            weapon->unidentifiedField6=MwReadNativeWord(bytes,0x0050+mount*8);
        }
        for (uint16_t family=0;family<COMBAT_AMMO_FAMILIES;++family)
        {
            mech->ammoBodyLocations[family]=(int16_t)MwReadNativeWord(bytes,0x009A+family*2);
            mech->initialAmmoSalvos[family]=MwReadNativeWord(bytes,0x00D4+family*2);
        }
        for (uint16_t group=0;group<COMBAT_JET_GROUPS;++group)
        {
            mech->jumpJetGroups[group].bodyLocation=(int16_t)MwReadNativeWord(bytes,0x00A6+group*4);
            mech->jumpJetGroups[group].criticalSlots=MwReadNativeWord(bytes,0x00A8+group*4);
        }
        for (uint16_t location=0;location<COMBAT_BODY_LOCATIONS;++location)
            mech->maximumInternalStructure[location]=MwReadNativeWord(bytes,0x00C4+location*2);
    }
    for (uint16_t index=0;index<COMBAT_RANDOM_WORDS;++index)
        randomStateWords[index]=MwReadNativeWord(data,0x281C+index*2);
    randomWriteIndex=0; /*285C*/
    randomAddendIndex=MwReadNativeWord(data,0x285E);
    specialAssaultMode=0; /*C7BE*/
    return true;
}

/* EXEC/DFCE data projection only, not206D or a new game procedure. Clear
 * converted owners in the native4EDC..D5DF BSS interval; reload authored
 * scalars separately. Host bindings and campaign owners are untouched. */
void BtechInitializeFreshMechOwners(const uint8_t *data)
{
    combatContinuationStage=0;
    reducedMemoryMode=0;
    combatTerrainDetailLevel=0;
    combatMissionDefinitionIndex=0;
    combatSoundEnabled=0;
    playbackCombatInputRequested=0;
    combatScenarioIndex=0;
    for (uint16_t slot=0;slot<8;++slot)
        gunneryTargetNumbers[slot]=0;
    for (uint16_t slot=0;slot<8;++slot)
        pilotingRecoveryBounds[slot]=0;
    activeCockpitLayoutIndex=0;
    for (uint16_t slot=0;slot<4;++slot)
        combatToPilotRosterSlots[slot]=0;
    for (uint16_t slot=0;slot<4;++slot)
        combatToOwnedMechSlots[slot]=0;
    for (uint16_t slot=0;slot<4;++slot)
        deployedPilotIds[slot]=0;
    mechTemplateReservations=BtechTemplateReservations{};
    hudColourSchemeIndex=0;
    savedArgumentCount=0;
    activeUnitControl=NULL;
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
        mechStates[slot]=BtechMechState{};
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
        unitBindings[slot]=BtechUnitBinding{};
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
        unitControls[slot]=BtechUnitControlState{};
    specialAssaultMode=0;
    specialTargetInteractionBlocked=0; /*0EEA*/
    /* Written before it's read (BtechPrepareCampaignCombatStage); zero where the exe reloaded it. */
    for (uint16_t slot=0;slot<sizeof(sideObjectiveRules)/sizeof(sideObjectiveRules[0]);++slot) sideObjectiveRules[slot]=0;
    combatLandscapeIndex=0; /*1E86*/
    /* From the story file, as the importer reads it; a missing text fails that import. */
    (void)MwStoryCopyText("combat.commander.default_name",(char *)combatDefaultCommanderNameBytes,sizeof(combatDefaultCommanderNameBytes));
    savedArguments=NULL;
}
