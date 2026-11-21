// The rules (docs/phase2/PLAN.md section 5): numbers we own, transcribed from
// the original and checked against it (RuleTable.h), read from
// <data dir>/rules/*.json. Never exe text: names are story text ids.
//
//   units.json   {"units": {"locust": {...}, ..., "dropship": {...}}}
//     Keyed by CampaignUnitKind (each kind once, nothing else). A mech:
//       {"tonnage", "base_price", "display": {"tonnage", "speed_kph",
//        "jump_metres"}, "heat_sinks", "weapons", "ammo_groups",
//        "jump_jet_groups", "condition_fields", "condition_score_divisor",
//        "force_weight", "body_repair_price",
//        "ammo_profile": [{"family", "quantity_maximum"}, ...]  (ammo_groups),
//        "weapon_repair_prices": [...]}  (one per row of the unit's weapon
//        repair list: weapons + 1)
//     The DropShip: {"base_price", "condition_score_divisor"}.
//   market.json  {"price_tier_multipliers": [4],
//                 "ammunition_prices": {"buy": [6], "sell": [6]},
//                 "stock_kind_ranges": [{"start", "maximum"}, x3],
//                 "freight_rates": [6]}
//     Ammunition families have no names in the code yet, so they stay in
//     the original's order.
//   repair.json  {"system_prices": [{"locust": price, ...}, x10 systems],
//                 "success_roll_limits": [4, by market tier]}
//   houses.json  {"houses": {"kurita": {...}, "steiner", "marik", "liao",
//                 "davion"}}, keyed by HOUSE_*. A House:
//       {"advance_base_percent", "salvage_base_percent", "payment_roll_bias",
//        "payment_adjustment_percent" (signed), "deduction_roll_threshold",
//        "reduced_payout_percent", "extra_enemy_roll_threshold",
//        "negotiation_attempt_base",
//        "planets": {"first", "count"}  (story planet ids, from 1),
//        "mission_type_limits": [8, by target column],
//        "mission_planet_policy": [8 target columns of [8 planet ids or null]]}
//   contracts.json  {"deployment_units_by_scenario": [37],
//                    "pilot_wages": {"poor", "average", "good", "excellent"}}
//   "start" (market.json, contracts.json): runtime values play overwrites,
//   given as the exe starts them; applied again on every EXEC, as the exe's
//   reload does. market: "ammunition_transaction_prices" {"buy", "sell"},
//   "generate_starting_jenner", "generated_ammo_kinds" [3],
//   "generated_ammo_quantities" [3]. contracts: "mission_force_budget",
//   "mission_payment_base_thousands", "pilot_card": {"gunnery", "piloting",
//   "quoted_wage"}, "settlement_payout_percent",
//   "travel_destination_time_weight".
//   combat.json (BTECH): locations by name (armour: COMBAT_ARMOR_*; body: the
//   same less the rear centre torso, the order armorToBodyLocations gives),
//   mech templates by number from 1 (as the story's combat.mech_template.N),
//   unit kinds by name. See read_combat for the members; "start" holds
//   "side_objective_rules" [2]. Its "mission_side_zero_rules" and
//   "fallback_template_steps" were missing from the inventory.
//   weapons.json  {"weapons": {"small_laser": {...}, ...}}, keyed by
//     COMBAT_WEAPON_*: {"damage", "ammo_family" (null: none), "reload_ticks",
//     "heat", "range_class", "ranges": {"minimum", "short", "medium", "long"},
//     "critical_slots", "unknown_14", "missile_cluster_row" (null: none),
//     "projectile_profile" (null: none)}
//   combat_assets.json  frame counts and effect resource steps that describe
//     the install's files: {"animation_prefix_counts" [9], "player_pose_frame_counts" [8],
//     "body_damage_effect_steps" [4], "default_effect_steps" [7],
//     "machine_gun_effect_steps" [3]}
//   mechs.json  {"mechs": [8 templates, in order]}, each {"name" (its story
//     text id, combat.mech_template.N.name), "tonnage", "display_scale",
//     "firing_movement_threshold", "maximum_speed_scale", "jump_acceleration",
//     "jump_speed_limit", "unknown_0e", "heat_sinks", "reference_points"
//     {armour: [3 signed]}, "armour" {armour: n}, "internal" {body: n},
//     "mounts" [up to 10 {"weapon", "location" (body), "unknown_4",
//     "unknown_6"}], "ammo" [6, by ammo family: {"location" (body or null),
//     "salvos"}], "jump_jets" [up to 3 {"location" (body), "critical_slots"}]}.
//     The mount and jet counts are the lists' lengths.
//   new_campaign.json  MW_MAIN's save block at the start of a new campaign,
//     keyed by the save codec's field names (NewCampaignFields.h): a byte or
//     word field is a number, a word list a list of numbers, any other field
//     a list of its bytes. "commanderNameBytes" is its story text id,
//     "commander.default_name", which the importer fills.
#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace mw1o::rules {

struct RulesError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// CampaignUnitKind, in order; the first ten are the mechs.
inline constexpr std::array<std::string_view, 11> kUnitKinds = {
    "locust", "wasp", "jenner", "phoenix_hawk", "shadow_hawk", "wolverine",
    "rifleman", "warhammer", "marauder", "battlemaster", "dropship"};
inline constexpr std::size_t kMechKinds = 10;
inline constexpr std::size_t kAmmoFamilies = 6;
// The exe's storage for the per-unit lists (AMMO_PROFILE_ENTRY_COUNT,
// REPAIR_WEAPON_PRICE_WORD_COUNT): the lists, in unit order, must fit.
inline constexpr std::size_t kAmmoProfileEntries = 15;
inline constexpr std::size_t kWeaponRepairPrices = 62;

struct AmmoProfileEntry {
    std::uint16_t family = 0, quantity_maximum = 0;
};

struct Mech {
    std::uint16_t tonnage = 0;
    std::uint8_t display_tonnage = 0, display_speed_kph = 0, display_jump_metres = 0;
    std::uint8_t heat_sinks = 0, weapons = 0, ammo_groups = 0, jump_jet_groups = 0;
    std::uint8_t condition_fields = 0, force_weight = 0, body_repair_price = 0;
    std::vector<AmmoProfileEntry> ammo_profile;
    std::vector<std::uint16_t> weapon_repair_prices;
};

struct Units {
    std::array<Mech, kMechKinds> mechs;
    std::array<std::uint16_t, kUnitKinds.size()> base_prices{};
    std::array<std::uint8_t, kUnitKinds.size()> condition_score_divisors{};
};

struct StockKindRange {
    std::uint8_t start = 0, maximum = 0;
};

struct MarketStart {
    std::array<std::uint16_t, kAmmoFamilies> ammunition_buy{}, ammunition_sell{};
    std::uint8_t generate_starting_jenner = 0;
    std::array<std::uint16_t, 3> generated_ammo_kinds{}, generated_ammo_quantities{};
};

struct Market {
    std::array<std::uint8_t, 4> price_tier_multipliers{};
    std::array<std::uint16_t, kAmmoFamilies> ammunition_buy{}, ammunition_sell{};
    std::array<StockKindRange, 3> stock_kind_ranges{};
    std::array<std::uint16_t, kAmmoFamilies> freight_rates{};
    MarketStart start;
};

inline constexpr std::size_t kSystems = 10;

struct Repair {
    std::array<std::array<std::uint16_t, kMechKinds>, kSystems> system_prices{};
    std::array<std::uint8_t, 4> success_roll_limits{};
};

// HOUSE_*, in order.
inline constexpr std::array<std::string_view, 5> kHouses = {"kurita", "steiner", "marik", "liao", "davion"};
inline constexpr std::size_t kTargetColumns = 8, kPlanetChoices = 8;
inline constexpr std::uint8_t kNoPlanet = 0xFF;  // MISSION_PLANET_SELECTOR_NONE

struct House {
    std::uint16_t advance_base_percent = 0, salvage_base_percent = 0, payment_roll_bias = 0;
    std::int16_t payment_adjustment_percent = 0;
    std::uint16_t deduction_roll_threshold = 0, reduced_payout_percent = 0, extra_enemy_roll_threshold = 0;
    std::uint8_t negotiation_attempt_base = 0;
    std::uint16_t first_planet = 1, planet_count = 0;  // story planet ids, from 1
    std::array<std::uint8_t, kTargetColumns> mission_type_limits{};
    // Planet ids from 1, or kNoPlanet.
    std::array<std::array<std::uint8_t, kPlanetChoices>, kTargetColumns> mission_planet_policy{};
};

// CampaignPilotSkillTier, in order.
inline constexpr std::array<std::string_view, 4> kSkillTiers = {"poor", "average", "good", "excellent"};
inline constexpr std::size_t kDeploymentScenarios = 37;

struct ContractsStart {
    std::uint16_t mission_force_budget = 0, mission_payment_base_thousands = 0;
    std::uint16_t pilot_card_gunnery = 0, pilot_card_piloting = 0, pilot_card_quoted_wage = 0;
    std::uint16_t settlement_payout_percent = 0, travel_destination_time_weight = 0;
};

struct Contracts {
    std::array<std::uint16_t, kDeploymentScenarios> deployment_units_by_scenario{};
    std::array<std::uint16_t, kSkillTiers.size()> pilot_wages{};
    ContractsStart start;
};

// COMBAT_ARMOR_* and COMBAT_BODY_* (combat_damage.h, combat_mechs.h), in order.
inline constexpr std::array<std::string_view, 9> kArmourLocations = {
    "left_arm", "right_arm", "left_leg", "right_leg", "center_torso", "rear_center_torso",
    "left_torso", "right_torso", "head"};
inline constexpr std::array<std::string_view, 8> kBodyLocations = {
    "left_arm", "right_arm", "left_leg", "right_leg", "center_torso", "left_torso", "right_torso", "head"};
// COMBAT_WEAPON_*, in order.
inline constexpr std::array<std::string_view, 10> kWeaponKinds = {
    "small_laser", "medium_laser", "large_laser", "machine_gun", "ppc", "ac5", "lrm5", "srm2", "srm4", "srm6"};
inline constexpr std::size_t kTemplates = 8, kCombatMissions = 37, kStageProfiles = 9, kStagesPerProfile = 3;
inline constexpr std::size_t kAttackArcs = 4, kHitRolls = 11, kClusterRows = 4, kClusterRolls = 12;
inline constexpr std::size_t kProjectileProfiles = 5;

struct Combat {
    std::array<std::uint16_t, 9> body_of_armour{};               // body location index
    std::array<std::uint16_t, 9> campaign_body_tier_of_armour{};
    std::array<std::uint16_t, 8> critical_transfer{};            // body -> body
    std::array<std::uint16_t, 8> damage_transfer{};              // body -> armour
    std::array<std::uint16_t, 8> body_reference_point{};         // body -> armour
    std::array<std::uint16_t, 4> actuator_of_limb{};
    std::array<std::uint16_t, kMechKinds> template_of_unit_kind{};  // template index (0-based)
    std::array<std::uint16_t, kTemplates> unit_kind_of_template{};
    std::array<std::uint16_t, kTemplates> fallback_start_template{};
    std::array<std::int16_t, kTemplates> fallback_template_steps{};
    std::array<std::uint32_t, kTemplates> template_budget_costs{};
    std::array<std::uint16_t, 4> weapon_disabled_by_tier{}, system_hits_by_tier{}, actuator_loss_by_tier{};
    std::array<std::uint16_t, 5> actuator_integrity_by_condition{};
    std::array<std::uint16_t, 3> system_tier_by_hits{};
    std::array<std::uint16_t, 2> weapon_integrity_by_operational{};
    std::array<std::uint16_t, 5> opposing_objective{};
    std::array<std::uint16_t, kCombatMissions> mission_side_zero_rules{};
    std::array<std::array<std::uint16_t, kStagesPerProfile>, kStageProfiles> continuation_profiles{};
    std::array<std::uint16_t, 5> heat_aim_jitter_thresholds{}, range_hit_modifiers{};
    std::array<std::array<std::uint16_t, kHitRolls>, kAttackArcs> hit_locations{};  // armour
    std::array<std::array<std::uint16_t, kClusterRolls>, kClusterRows> missile_cluster_rows{};
    std::array<std::uint16_t, 2> opposing_side_first_unit_slots{};
    std::array<std::int16_t, 3> avoidance_heading_offsets{};
    std::array<std::uint16_t, 2> start_side_objective_rules{};
};

inline constexpr std::uint16_t kNone = 0xFFFF;  // ammo family, cluster row, projectile profile: none

struct Weapon {
    std::uint16_t damage = 0, ammo_family = kNone, reload_ticks = 0, heat = 0, range_class = 0;
    std::uint16_t minimum_range = 0, short_range = 0, medium_range = 0, long_range = 0;
    std::uint16_t critical_slots = 0, unknown_14 = 0;
    std::uint16_t missile_cluster_row = kNone, projectile_profile = kNone;
};

struct CombatAssets {
    std::array<std::uint16_t, 9> animation_prefix_counts{};
    std::array<std::uint16_t, 8> player_pose_frame_counts{};
    std::array<std::uint16_t, 4> body_damage_effect_steps{};
    std::array<std::uint16_t, 7> default_effect_steps{};
    std::array<std::uint16_t, 3> machine_gun_effect_steps{};
};

inline constexpr std::size_t kMounts = 10, kJetGroups = 3;

struct MechTemplate {
    std::uint16_t tonnage = 0, display_scale = 0, firing_movement_threshold = 0, maximum_speed_scale = 0;
    std::uint16_t jump_acceleration = 0, jump_speed_limit = 0, unknown_0e = 0, heat_sinks = 0;
    std::array<std::array<std::int16_t, 3>, 9> reference_points{};  // by armour location
    std::array<std::uint16_t, 9> armour{};
    std::array<std::uint16_t, 8> internal{};
    struct Mount { std::uint16_t weapon = 0, location = 0, unknown_4 = 0, unknown_6 = 0; };
    std::vector<Mount> mounts;                                 // at most kMounts
    std::array<std::uint16_t, kAmmoFamilies> ammo_location{};  // body location, or kNone
    std::array<std::uint16_t, kAmmoFamilies> ammo_salvos{};
    struct JumpJets { std::uint16_t location = 0, critical_slots = 0; };
    std::vector<JumpJets> jump_jets;                           // at most kJetGroups
};

inline constexpr std::size_t kSaveBlockBytes = 0x718;
inline constexpr std::string_view kCommanderNameField = "commanderNameBytes";
inline constexpr std::string_view kCommanderNameText = "commander.default_name";

struct NewCampaign {
    std::array<std::uint8_t, kSaveBlockBytes> block{};  // the commander's name left zero
};

struct Rules {
    Units units;
    Market market;
    Repair repair;
    std::array<House, kHouses.size()> houses;
    Contracts contracts;
    Combat combat;
    std::array<Weapon, kWeaponKinds.size()> weapons;
    CombatAssets assets;
    std::array<MechTemplate, kTemplates> mechs;
    NewCampaign new_campaign;
};

// <data_dir>/rules/*.json, read strictly. Throws RulesError naming the file.
Rules load_rules(const std::filesystem::path& data_dir);

// Every rule piece's bytes as the exe lays them out (RuleTable.h), by id:
// what the game fills its owners from, and what rules_test hashes. Throws
// RulesError if a per-unit list doesn't fit the exe's storage.
std::map<std::string, std::string, std::less<>> native_pieces(const Rules& rules);

}  // namespace mw1o::rules
