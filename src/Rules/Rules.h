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
//   market.json "start": runtime values play overwrites but reads first on
//   some path, given as the exe starts them and applied again on every EXEC,
//   as the exe's reload does: "ammunition_transaction_prices" {"buy",
//   "sell"}, "generate_starting_jenner", "generated_ammo_kinds" [3],
//   "generated_ammo_quantities" [3]. (The other start values are always
//   written before they're read; they are zeroed where the exe reloaded
//   them. docs/phase2/PLAN.md 5.6, 3f.)
//   combat.json (BTECH): locations by name (armour: COMBAT_ARMOR_*; body: the
//   same less the rear centre torso, the order armorToBodyLocations gives),
//   mech templates by number from 1 (as the story's combat.mech_template.N),
//   unit kinds by name. See read_combat for the members. Its
//   "mission_side_zero_rules" and
//   "fallback_template_steps" were missing from the inventory.
//   weapons.json  {"weapons": {"small_laser": {...}, ...}}, keyed by
//     COMBAT_WEAPON_*: {"damage", "ammo_family" (null: none), "reload_ticks",
//     "heat", "range_class", "ranges": {"minimum", "short", "medium", "long"},
//     "critical_slots", "unknown_14", "missile_cluster_row" (null: none),
//     "projectile_profile" (null: none), "beam_half_width"},
//     "projectiles": [5 {"speed_scale", "weapon", "lifetime_ticks",
//     "resource_delta"}], "beams": {"triangles": {"vertical", "horizontal":
//     [vertex indices]}, "ppc", "laser": {"scale_limit", "polygons":
//     {"vertical", "horizontal": {"flags", "fill", "line", "alternate_fill",
//     "alternate_line", "normal"}}}}
//   combat_assets.json  frame counts and effect resource steps that describe
//     the install's files: {"animation_prefix_counts" [9],
//     "body_damage_effect_steps" [4], "default_effect_steps" [7],
//     "machine_gun_effect_steps" [3]}
//   animation.json  {"unit_sequences": [9 {"frame_map": [logical frame ->
//     part frame], "part_frames"}], "player_poses": [8 sequences of
//     [{"displacement": [3 signed], "orientation_step"}]]}. A sequence's
//     logical frame count and a pose's frame count are its list's length.
//   mechs.json  {"mechs": [8 templates, in order]}, each {"name" (its story
//     text id, combat.mech_template.N.name), "tonnage", "display_scale",
//     "firing_movement_threshold", "maximum_speed_scale", "jump_acceleration",
//     "jump_speed_limit", "unknown_0e", "heat_sinks", "reference_points"
//     {armour: [3 signed]}, "armour" {armour: n}, "internal" {body: n},
//     "mounts" [up to 10 {"weapon", "location" (body), "unknown_4",
//     "unknown_6"}], "ammo" [6, by ammo family: {"location" (body or null),
//     "salvos"}], "jump_jets" [up to 3 {"location" (body), "critical_slots"}]}.
//     The mount and jet counts are the lists' lengths. Each template also
//     has "fallen_reference_points" {armour: [3 signed]} and "animations"
//     [9 true/false, by unit animation sequence]; the file also has
//     "muzzle_lateral_offsets" {body: signed}.
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
// The exe's storage for the mechs each ammo family fits (1090:3F9F, each list
// ended by 0xFFFF) and the Houses' contract planet groups (1090:4B8A).
inline constexpr std::size_t kAmmoFamilyMechBytes = 36;
inline constexpr std::size_t kContractPlanetGroupBytes = 0x66;
inline constexpr std::uint8_t kPlanetGroupEnd = 0xFF, kHouseGroupsEnd = 0xFE;

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
    // Per ammo family, the mech kinds (kUnitKinds indices) its ammunition fits.
    std::array<std::vector<std::uint8_t>, kAmmoFamilies> ammo_family_mechs;
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
    // The contract office's planet groups: planet ids from 1 (not 0xFE, 0xFF).
    std::vector<std::vector<std::uint8_t>> contract_planet_groups;
};

// CampaignPilotSkillTier, in order.
inline constexpr std::array<std::string_view, 4> kSkillTiers = {"poor", "average", "good", "excellent"};
inline constexpr std::size_t kDeploymentScenarios = 37;

struct Contracts {
    std::array<std::uint16_t, kDeploymentScenarios> deployment_units_by_scenario{};
    std::array<std::uint16_t, kSkillTiers.size()> pilot_wages{};
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
inline constexpr std::size_t kProjectileProfiles = 5, kAvoidanceProbes = 4, kRandomStateWords = 32;
// combat.json "start": runtime values play overwrites but reads first, as a
// fresh EXEC starts them.
struct CombatStart {
    std::uint16_t continuation_profile = 0xFFFF;  // none
    std::uint8_t heading_radar_selected = 0;
    std::uint16_t radar_range_shift = 0, view_scale_shift = 0;
    std::uint8_t mount_group_mode = 0;
    std::array<std::uint8_t, 2> firing_indicators_pending{};  // bank 0, bank 1
    std::array<std::uint16_t, kRandomStateWords> random_state{};
    std::uint16_t random_addend_index = 0;
};

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
    // Each probe's x and y: a sign, and the avoidance step it scales (0 x, 1 y).
    struct ProbeAxis { std::int16_t sign = 0; std::uint16_t step = 0; };
    std::array<std::array<ProbeAxis, 2>, kAvoidanceProbes> avoidance_probes{};
    std::array<std::array<std::int32_t, 3>, 2> world_boundary_corners{};
    bool player_damage_suppressed = false, ai_updates_suppressed = false;
    std::uint8_t unknown_0ebb = 0;
    CombatStart start;
};

inline constexpr std::uint16_t kNone = 0xFFFF;  // ammo family, cluster row, projectile profile: none

struct Weapon {
    std::uint16_t damage = 0, ammo_family = kNone, reload_ticks = 0, heat = 0, range_class = 0;
    std::uint16_t minimum_range = 0, short_range = 0, medium_range = 0, long_range = 0;
    std::uint16_t critical_slots = 0, unknown_14 = 0;
    std::uint16_t missile_cluster_row = kNone, projectile_profile = kNone;
    std::uint16_t beam_half_width = 0;
};
struct Projectile {
    std::uint16_t speed_scale = 0, weapon = 0, lifetime_ticks = 0, resource_delta = 0;
};
struct BeamPolygon {
    std::uint8_t flags = 0, fill = 0, line = 0, alternate_fill = 0, alternate_line = 0, normal = 0;
};
struct Beam {
    std::uint8_t scale_limit = 0;
    std::array<BeamPolygon, 2> polygons{};  // vertical, horizontal
};
struct Weapons {
    std::array<Weapon, kWeaponKinds.size()> kinds;
    std::array<Projectile, kProjectileProfiles> projectiles{};
    std::array<std::vector<std::uint8_t>, 2> beam_triangles;  // vertical, horizontal
    std::array<Beam, 2> beams{};                               // ppc, laser
};
inline constexpr std::size_t kUnitAnimationSequences = 9, kPlayerPoseSequences = 8;
struct PoseKeyframe {
    std::array<std::int16_t, 3> displacement{};
    std::uint16_t orientation_step = 0;
};
struct Animation {
    struct UnitSequence { std::vector<std::uint8_t> frame_map; std::uint8_t part_frames = 0; };
    std::array<UnitSequence, kUnitAnimationSequences> unit_sequences;
    std::array<std::vector<PoseKeyframe>, kPlayerPoseSequences> player_poses;
};

struct CombatAssets {
    std::array<std::uint16_t, 9> animation_prefix_counts{};
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
    std::array<std::array<std::int16_t, 3>, 9> fallen_reference_points{};  // by armour location
    std::array<bool, kUnitAnimationSequences> animations{};
};
struct Mechs {
    std::array<MechTemplate, kTemplates> templates;
    std::array<std::int16_t, 8> muzzle_lateral_offsets{};  // by body location
};

inline constexpr std::size_t kSaveBlockBytes = 0x718;
inline constexpr std::string_view kCommanderNameField = "commanderNameBytes";
inline constexpr std::string_view kCommanderNameText = "commander.default_name";

struct NewCampaign {
    std::array<std::uint8_t, kSaveBlockBytes> block{};  // the commander's name left zero
    // "start": the random generator's two words as a fresh EXEC starts them.
    std::array<std::uint16_t, 2> random_seed{};
};

struct Rules {
    Units units;
    Market market;
    Repair repair;
    std::array<House, kHouses.size()> houses;
    Contracts contracts;
    Combat combat;
    Weapons weapons;
    CombatAssets assets;
    Animation animation;
    Mechs mechs;
    NewCampaign new_campaign;
};

// <data_dir>/rules/*.json, read strictly. Throws RulesError naming the file.
Rules load_rules(const std::filesystem::path& data_dir);

// Every rule piece's bytes as the exe lays them out (RuleTable.h), by id:
// what the game fills its owners from, and what rules_test hashes. Throws
// RulesError if a per-unit list doesn't fit the exe's storage.
std::map<std::string, std::string, std::less<>> native_pieces(const Rules& rules);

}  // namespace mw1o::rules
