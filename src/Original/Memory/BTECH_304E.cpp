#include "mechwarrior/story.h"
#include "mechwarrior/rules.h"
#include "mechwarrior/combat_mechs.h"
#include "mechwarrior/native_address.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_installation.h"
#include "mechwarrior/combat_damage.h"
#include <string.h>

CombatImportState combatImport;
static_assert(offsetof(BtechMissionRules,fields.continuationProfileIndex)==0x48,"Mission36 aliases profile0974");
static_assert(sizeof(BtechTemplateReservations)==0x14,"Native reservation WORD aliases");
static_assert(offsetof(BtechTemplateReservations,fields.reservedCount)==0x0E,"Native reservation count alias");
CombatDamageState combatDamage;

bool BtechImportCombatStageInitialData(const uint8_t *data,size_t byteCount)
{
    if (!BtechImportMechInitialData(data,byteCount)) return false;
    combatImport.startupControlMovementAlias=MwReadNativeWord(data,0x0051);
    combatDamage.playerDamageSuppressed=data[0x0E1A];
    /* The rule tables come from data/rules (src/Rules), laid out as the exe's. */
    if (!MwRulesCopyWords("combat.damage_transfer",combatDamage.damageTransferArmorLocations,COMBAT_BODY_LOCATIONS,0)
        || !MwRulesCopyWords("combat.critical_transfer",combatDamage.criticalTransferBodyLocations,COMBAT_BODY_LOCATIONS,0)
        || !MwRulesCopyWords("combat.actuator_of_limb",combatDamage.actuatorIndicesByBodyLocation,4,0)) return false;
    if (!MwStoryCopyText("combat.commander.default_name",(char *)combatImport.combatDefaultCommanderNameBytes,sizeof(combatImport.combatDefaultCommanderNameBytes))) return false;
    combatImport.salvageEntryCount=0; /*0976*/
    uint16_t steps[8],costs[16];
    if (!MwRulesCopyWords("combat.unit_kind_of_template",combatImport.campaignUnitKindByCombatTemplate,COMBAT_MECH_TEMPLATES,0)
        || !MwRulesCopyWords("combat.actuator_integrity_by_condition",combatImport.actuatorIntegrityTierByCondition,5,0)
        || !MwRulesCopyWords("combat.system_tier_by_hits",combatImport.systemDamageTierByHits,3,0)
        || !MwRulesCopyWords("combat.weapon_integrity_by_operational",combatImport.weaponIntegrityTierByOperational,2,0)
        || !MwRulesCopyWords("combat.body_of_armour",combatImport.armorToBodyLocations,9,0)
        || !MwRulesCopyWords("combat.campaign_body_tier_of_armour",combatImport.campaignBodyTierByArmorLocation,9,0)
        || !MwRulesCopyWords("combat.mission_side_zero_rules",combatMissionSideZeroRules,COMBAT_MISSION_COUNT,0)
        || !MwRulesCopyWords("combat.opposing_objective",combatImport.opposingObjectiveRules,5,0)
        || !MwRulesCopyWords("combat.template_of_unit_kind",combatImport.combatTemplateByCampaignUnitKind,10,0)
        || !MwRulesCopyWords("combat.weapon_disabled_by_tier",combatImport.weaponDisabledByDamageTier,4,0)
        || !MwRulesCopyWords("combat.system_hits_by_tier",combatImport.systemHitsByDamageTier,4,0)
        || !MwRulesCopyWords("combat.actuator_loss_by_tier",combatImport.actuatorLossByDamageTier,4,0)
        || !MwRulesCopyWords("combat.fallback_start_template",combatImport.mechFallbackStartTemplates,8,0)
        || !MwRulesCopyWords("combat.fallback_template_steps",steps,8,0)
        || !MwRulesCopyWords("combat.template_budget_costs",costs,16,0)) return false;
    for (uint16_t i=0;i<9;++i)
        if (!MwRulesCopyWords("combat.continuation_profiles",combatImport.combatContinuationProfiles[i].missionCodes,3,i*6)) return false;
    for (uint16_t i=0;i<8;++i)
    {
        combatImport.mechFallbackTemplateSteps[i]=(int16_t)(steps[i]<0x8000?steps[i]:(int32_t)steps[i]-0x10000);
        combatImport.mechTemplateBudgetCostsBytes[i]=(uint32_t)costs[i*2]|((uint32_t)costs[i*2+1]<<16);
    }
    combatImport.combatContinuationStage=0; /*C35A*/
    combatContinuationProfileIndex=MwReadNativeWord(data,0x0974);
    combatImport.combatMissionDefinitionIndex=0; /*C96E*/
    return true;
}

CombatMechState combatMechs;

/* File decoder only. Runtime records need not inherit native unaligned WORDs
 * or DOS near references. Mech/binding/control state is initialized by native
 *206D/2C40, NOT cleared as a side effect of loading authored templates. */
bool BtechImportMechInitialData(const uint8_t *data,size_t byteCount)
{
    if (!data || byteCount<NATIVE_SEGMENT_BYTE_COUNT) return false;
    /* The rule tables come from data/rules (src/Rules), laid out as the exe's. */
    if (!MwRulesCopyWords("combat.heat_aim_jitter_thresholds",combatMechs.heatAimJitterThresholds,COMBAT_HEAT_THRESHOLDS,0)) return false;
    for (uint16_t kind=0;kind<COMBAT_WEAPON_KINDS;++kind)
    {
        uint16_t row[11];
        if (!MwRulesCopyWords("weapons.definitions",row,11,kind*0x16)) return false;
        BtechWeaponDefinition *weapon=&combatMechs.weaponDefinitions[kind];
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
        BtechMechTemplate *mech=&combatMechs.mechTemplates[index];
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
        combatMechs.randomStateWords[index]=MwReadNativeWord(data,0x281C+index*2);
    combatMechs.randomWriteIndex=0; /*285C*/
    combatMechs.randomAddendIndex=MwReadNativeWord(data,0x285E);
    combatMechs.specialAssaultMode=0; /*C7BE*/
    return true;
}

/* EXEC/DFCE data projection only, not206D or a new game procedure. Clear
 * converted owners in the native4EDC..D5DF BSS interval; reload authored
 * scalars separately. Host bindings and campaign owners are untouched. */
void BtechInitializeFreshMechOwners(const uint8_t *data)
{
    combatImport.combatContinuationStage=0;
    combatImport.reducedMemoryMode=0;
    combatImport.combatTerrainDetailLevel=0;
    combatImport.combatMissionDefinitionIndex=0;
    combatImport.combatSoundEnabled=0;
    combatImport.playbackCombatInputRequested=0;
    combatImport.combatScenarioIndex=0;
    for (uint16_t slot=0;slot<8;++slot)
        combatImport.gunneryTargetNumbers[slot]=0;
    for (uint16_t slot=0;slot<8;++slot)
        combatImport.pilotingRecoveryBounds[slot]=0;
    combatImport.activeCockpitLayoutIndex=0;
    for (uint16_t slot=0;slot<4;++slot)
        combatImport.combatToPilotRosterSlots[slot]=0;
    for (uint16_t slot=0;slot<4;++slot)
        combatImport.combatToOwnedMechSlots[slot]=0;
    for (uint16_t slot=0;slot<4;++slot)
        combatImport.deployedPilotIds[slot]=0;
    combatImport.mechTemplateReservations=BtechTemplateReservations{};
    combatImport.hudColourSchemeIndex=0;
    combatImport.savedArgumentCount=0;
    combatImport.activeUnitControl=NULL;
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
        combatMechs.mechStates[slot]=BtechMechState{};
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
        combatMechs.unitBindings[slot]=BtechUnitBinding{};
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
        combatMechs.unitControls[slot]=BtechUnitControlState{};
    combatMechs.specialAssaultMode=0;
    combatImport.specialTargetInteractionBlocked=0; /*0EEA*/
    /* Written before it's read (BtechPrepareCampaignCombatStage); zero where the exe reloaded it. */
    for (uint16_t slot=0;slot<sizeof(combatImport.sideObjectiveRules)/sizeof(combatImport.sideObjectiveRules[0]);++slot) combatImport.sideObjectiveRules[slot]=0;
    combatImport.combatLandscapeIndex=0; /*1E86*/
    /* From the story file, as the importer reads it; a missing text fails that import. */
    (void)MwStoryCopyText("combat.commander.default_name",(char *)combatImport.combatDefaultCommanderNameBytes,sizeof(combatImport.combatDefaultCommanderNameBytes));
    combatImport.savedArguments=NULL;
}
