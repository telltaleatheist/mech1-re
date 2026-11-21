#include "Rules.h"

#include <cmath>
#include <span>
#include <variant>

#include "../Support/StrictReader.h"
#include "RuleTable.h"

namespace mw1o::rules {

namespace fs = std::filesystem;
using support::Json;
using Reader = support::StrictReader<RulesError>;

namespace {

constexpr std::string_view kMechKeys[] = {
    "tonnage",          "base_price",     "display",          "heat_sinks",        "weapons",
    "ammo_groups",      "jump_jet_groups", "condition_fields", "condition_score_divisor",
    "force_weight",     "body_repair_price", "ammo_profile",   "weapon_repair_prices"};
constexpr std::string_view kDropShipKeys[] = {"base_price", "condition_score_divisor"};

template <std::size_t N>
void words(const Reader& r, const Json::Array& from, std::array<std::uint16_t, N>& to, std::string_view what) {
    for (std::size_t i = 0; i < N; ++i) to[i] = r.word(from[i], what);
}

Mech read_mech(const Reader& r, const Json& m) {
    r.only(m, kMechKeys);
    Mech out;
    out.tonnage = r.u16(m, "tonnage");
    const Json& display = r.member(m, "display");
    r.only(display, {"tonnage", "speed_kph", "jump_metres"});
    out.display_tonnage = r.u8(display, "tonnage");
    out.display_speed_kph = r.u8(display, "speed_kph");
    out.display_jump_metres = r.u8(display, "jump_metres");
    out.heat_sinks = r.u8(m, "heat_sinks");
    out.weapons = r.u8(m, "weapons");
    out.ammo_groups = r.u8(m, "ammo_groups");
    out.jump_jet_groups = r.u8(m, "jump_jet_groups");
    out.condition_fields = r.u8(m, "condition_fields");
    out.force_weight = r.u8(m, "force_weight");
    out.body_repair_price = r.u8(m, "body_repair_price");
    for (const Json& e : r.array(m, "ammo_profile", out.ammo_groups)) {
        r.only(e, {"family", "quantity_maximum"});
        out.ammo_profile.push_back({r.u16(e, "family"), r.u16(e, "quantity_maximum")});
    }
    for (const Json& p : r.array(m, "weapon_repair_prices", std::size_t(out.weapons) + 1))
        out.weapon_repair_prices.push_back(r.word(p, "a weapon repair price"));
    return out;
}

Units read_units(const fs::path& data_dir) {
    Reader r(data_dir / "rules" / "units.json");
    const Json file = r.parse();
    r.only(file, {"units"});
    const auto units = r.keyed(file, "units", kUnitKinds);
    Units out;
    for (std::size_t kind = 0; kind < kUnitKinds.size(); ++kind) {
        const Json& u = *units[kind];
        if (kind < kMechKinds) out.mechs[kind] = read_mech(r, u);
        else r.only(u, kDropShipKeys);
        out.base_prices[kind] = r.u16(u, "base_price");
        out.condition_score_divisors[kind] = r.u8(u, "condition_score_divisor");
    }
    return out;
}

Market read_market(const fs::path& data_dir) {
    Reader r(data_dir / "rules" / "market.json");
    const Json file = r.parse();
    r.only(file, {"price_tier_multipliers", "ammunition_prices", "stock_kind_ranges", "freight_rates", "start"});
    Market out;
    const auto& tiers = r.array(file, "price_tier_multipliers", out.price_tier_multipliers.size());
    for (std::size_t i = 0; i < tiers.size(); ++i) out.price_tier_multipliers[i] = r.byte(tiers[i], "a price tier multiplier");
    const Json& ammo = r.member(file, "ammunition_prices");
    r.only(ammo, {"buy", "sell"});
    const auto& buy = r.array(ammo, "buy", kAmmoFamilies);
    const auto& sell = r.array(ammo, "sell", kAmmoFamilies);
    const auto& freight = r.array(file, "freight_rates", kAmmoFamilies);
    for (std::size_t f = 0; f < kAmmoFamilies; ++f) {
        out.ammunition_buy[f] = r.word(buy[f], "an ammunition price");
        out.ammunition_sell[f] = r.word(sell[f], "an ammunition price");
        out.freight_rates[f] = r.word(freight[f], "a freight rate");
    }
    const auto& ranges = r.array(file, "stock_kind_ranges", out.stock_kind_ranges.size());
    for (std::size_t i = 0; i < ranges.size(); ++i) {
        r.only(ranges[i], {"start", "maximum"});
        out.stock_kind_ranges[i] = {r.u8(ranges[i], "start"), r.u8(ranges[i], "maximum")};
    }
    const Json& start = r.member(file, "start");
    r.only(start, {"ammunition_transaction_prices", "generate_starting_jenner", "generated_ammo_kinds",
                   "generated_ammo_quantities"});
    const Json& transaction = r.member(start, "ammunition_transaction_prices");
    r.only(transaction, {"buy", "sell"});
    words(r, r.array(transaction, "buy", kAmmoFamilies), out.start.ammunition_buy, "an ammunition price");
    words(r, r.array(transaction, "sell", kAmmoFamilies), out.start.ammunition_sell, "an ammunition price");
    out.start.generate_starting_jenner = r.u8(start, "generate_starting_jenner");
    words(r, r.array(start, "generated_ammo_kinds", 3), out.start.generated_ammo_kinds, "an ammunition kind");
    words(r, r.array(start, "generated_ammo_quantities", 3), out.start.generated_ammo_quantities,
          "an ammunition quantity");
    return out;
}

Repair read_repair(const fs::path& data_dir) {
    Reader r(data_dir / "rules" / "repair.json");
    const Json file = r.parse();
    r.only(file, {"system_prices", "success_roll_limits"});
    Repair out;
    const auto& systems = r.array(file, "system_prices", kSystems);
    for (std::size_t system = 0; system < kSystems; ++system) {
        if (!systems[system].is_object()) r.fail("a system's prices are not an object");
        r.only(systems[system], std::span(kUnitKinds).first(kMechKinds));
        for (std::size_t kind = 0; kind < kMechKinds; ++kind)
            out.system_prices[system][kind] = r.u16(systems[system], kUnitKinds[kind]);
    }
    const auto& limits = r.array(file, "success_roll_limits", out.success_roll_limits.size());
    for (std::size_t i = 0; i < limits.size(); ++i) out.success_roll_limits[i] = r.byte(limits[i], "a roll limit");
    return out;
}

constexpr std::string_view kHouseKeys[] = {
    "advance_base_percent", "salvage_base_percent", "payment_roll_bias", "payment_adjustment_percent",
    "deduction_roll_threshold", "reduced_payout_percent", "extra_enemy_roll_threshold",
    "negotiation_attempt_base", "planets", "mission_type_limits", "mission_planet_policy"};

std::array<House, kHouses.size()> read_houses(const fs::path& data_dir) {
    Reader r(data_dir / "rules" / "houses.json");
    const Json file = r.parse();
    r.only(file, {"houses"});
    const auto houses = r.keyed(file, "houses", kHouses);
    std::array<House, kHouses.size()> out;
    for (std::size_t i = 0; i < kHouses.size(); ++i) {
        const Json& h = *houses[i];
        r.only(h, kHouseKeys);
        House& house = out[i];
        house.advance_base_percent = r.u16(h, "advance_base_percent");
        house.salvage_base_percent = r.u16(h, "salvage_base_percent");
        house.payment_roll_bias = r.u16(h, "payment_roll_bias");
        const Json& adjustment = r.member(h, "payment_adjustment_percent");
        if (!adjustment.is_number() || adjustment.number() < -32768 || adjustment.number() > 32767 ||
            std::floor(adjustment.number()) != adjustment.number())
            r.fail("payment_adjustment_percent is not a whole number from -32768 to 32767");
        house.payment_adjustment_percent = static_cast<std::int16_t>(adjustment.number());
        house.deduction_roll_threshold = r.u16(h, "deduction_roll_threshold");
        house.reduced_payout_percent = r.u16(h, "reduced_payout_percent");
        house.extra_enemy_roll_threshold = r.u16(h, "extra_enemy_roll_threshold");
        house.negotiation_attempt_base = r.u8(h, "negotiation_attempt_base");
        const Json& planets = r.member(h, "planets");
        r.only(planets, {"first", "count"});
        house.first_planet = r.u16(planets, "first");
        house.planet_count = r.u16(planets, "count");
        if (house.first_planet < 1) r.fail("a House's first planet is not a planet id (from 1)");
        const auto& limits = r.array(h, "mission_type_limits", kTargetColumns);
        for (std::size_t c = 0; c < kTargetColumns; ++c) house.mission_type_limits[c] = r.byte(limits[c], "a mission type limit");
        const auto& policy = r.array(h, "mission_planet_policy", kTargetColumns);
        for (std::size_t c = 0; c < kTargetColumns; ++c) {
            if (!policy[c].is_array() || policy[c].array().size() != kPlanetChoices)
                r.fail("a mission planet policy row is not " + std::to_string(kPlanetChoices) + " planets");
            for (std::size_t choice = 0; choice < kPlanetChoices; ++choice) {
                const Json& planet = policy[c].array()[choice];
                std::uint8_t id = kNoPlanet;
                if (!std::holds_alternative<std::nullptr_t>(planet.value)) {
                    id = r.byte(planet, "a policy planet");
                    if (id < 1 || id == kNoPlanet) r.fail("a policy planet is not a planet id from 1 to 254");
                }
                house.mission_planet_policy[c][choice] = id;
            }
        }
    }
    return out;
}

Contracts read_contracts(const fs::path& data_dir) {
    Reader r(data_dir / "rules" / "contracts.json");
    const Json file = r.parse();
    r.only(file, {"deployment_units_by_scenario", "pilot_wages", "start"});
    Contracts out;
    words(r, r.array(file, "deployment_units_by_scenario", kDeploymentScenarios), out.deployment_units_by_scenario,
          "a deployment time");
    const auto wages = r.keyed(file, "pilot_wages", kSkillTiers);
    for (std::size_t tier = 0; tier < kSkillTiers.size(); ++tier) out.pilot_wages[tier] = r.word(*wages[tier], "a wage");
    const Json& start = r.member(file, "start");
    r.only(start, {"mission_force_budget", "mission_payment_base_thousands", "pilot_card",
                   "settlement_payout_percent", "travel_destination_time_weight"});
    out.start.mission_force_budget = r.u16(start, "mission_force_budget");
    out.start.mission_payment_base_thousands = r.u16(start, "mission_payment_base_thousands");
    const Json& card = r.member(start, "pilot_card");
    r.only(card, {"gunnery", "piloting", "quoted_wage"});
    out.start.pilot_card_gunnery = r.u16(card, "gunnery");
    out.start.pilot_card_piloting = r.u16(card, "piloting");
    out.start.pilot_card_quoted_wage = r.u16(card, "quoted_wage");
    out.start.settlement_payout_percent = r.u16(start, "settlement_payout_percent");
    out.start.travel_destination_time_weight = r.u16(start, "travel_destination_time_weight");
    return out;
}

template <std::size_t N>
void signed_words(const Reader& r, const Json::Array& from, std::array<std::int16_t, N>& to, std::string_view what) {
    for (std::size_t i = 0; i < N; ++i) to[i] = r.signed_word(from[i], what);
}

// A list inside a list, of exactly `count` entries.
const Json::Array& row(const Reader& r, const Json& v, std::size_t count, std::string_view what) {
    if (!v.is_array() || v.array().size() != count)
        r.fail(std::string(what) + " is not a list of " + std::to_string(count));
    return v.array();
}

// Each entry one of `names`, as its index.
template <std::size_t N>
void name_list(const Reader& r, const Json::Array& from, std::span<const std::string_view> names,
               std::array<std::uint16_t, N>& to, std::string_view what) {
    for (std::size_t i = 0; i < N; ++i) to[i] = static_cast<std::uint16_t>(r.name(from[i], names, what));
}

// An object keyed by exactly `keys`; each value one of `names` as its index,
// or a word when `names` is empty.
template <std::size_t N>
void keyed_words(const Reader& r, const Json& file, std::string_view key, std::span<const std::string_view> keys,
                 std::span<const std::string_view> names, std::array<std::uint16_t, N>& to) {
    const auto members = r.keyed(file, key, keys);
    for (std::size_t i = 0; i < N; ++i)
        to[i] = names.empty() ? r.word(*members[i], key) : static_cast<std::uint16_t>(r.name(*members[i], names, key));
}

// A mech template by number from 1, as its index.
std::uint16_t template_index(const Reader& r, const Json& v) {
    if (!v.is_number() || v.number() < 1 || v.number() > kTemplates || std::floor(v.number()) != v.number())
        r.fail("a template is not a number from 1 to " + std::to_string(kTemplates));
    return static_cast<std::uint16_t>(v.number() - 1);
}

Combat read_combat(const fs::path& data_dir) {
    Reader r(data_dir / "rules" / "combat.json");
    const Json file = r.parse();
    r.only(file, {"body_of_armour", "campaign_body_tier_of_armour", "critical_transfer", "damage_transfer",
                  "body_reference_point", "actuator_of_limb", "template_of_unit_kind", "unit_kind_of_template",
                  "fallback_start_template", "fallback_template_steps", "template_budget_costs", "damage_tiers",
                  "integrity_tiers", "opposing_objective", "mission_side_zero_rules", "continuation_profiles",
                  "heat_aim_jitter_thresholds", "range_hit_modifiers", "hit_locations", "missile_cluster_rows",
                  "opposing_side_first_unit_slots", "avoidance_heading_offsets", "start"});
    const std::span<const std::string_view> armour(kArmourLocations), body(kBodyLocations);
    const std::span<const std::string_view> mech_kinds = std::span(kUnitKinds).first(kMechKinds);
    Combat c;
    keyed_words(r, file, "body_of_armour", armour, body, c.body_of_armour);
    keyed_words(r, file, "campaign_body_tier_of_armour", armour, {}, c.campaign_body_tier_of_armour);
    keyed_words(r, file, "critical_transfer", body, body, c.critical_transfer);
    keyed_words(r, file, "damage_transfer", body, armour, c.damage_transfer);
    keyed_words(r, file, "body_reference_point", body, armour, c.body_reference_point);
    keyed_words(r, file, "actuator_of_limb", body.first(c.actuator_of_limb.size()), {}, c.actuator_of_limb);
    const auto templates = r.keyed(file, "template_of_unit_kind", mech_kinds);
    for (std::size_t i = 0; i < kMechKinds; ++i) c.template_of_unit_kind[i] = template_index(r, *templates[i]);
    name_list(r, r.array(file, "unit_kind_of_template", kTemplates), mech_kinds, c.unit_kind_of_template, "a unit kind");
    const Json::Array& starts = r.array(file, "fallback_start_template", kTemplates);
    for (std::size_t i = 0; i < kTemplates; ++i) c.fallback_start_template[i] = template_index(r, starts[i]);
    signed_words(r, r.array(file, "fallback_template_steps", kTemplates), c.fallback_template_steps, "a template step");
    const Json::Array& costs = r.array(file, "template_budget_costs", kTemplates);
    for (std::size_t i = 0; i < kTemplates; ++i)
        c.template_budget_costs[i] = static_cast<std::uint32_t>(r.whole(costs[i], "a budget cost", 4294967295.0));
    const Json& damage = r.member(file, "damage_tiers");
    r.only(damage, {"weapon_disabled", "system_hits", "actuator_loss"});
    words(r, r.array(damage, "weapon_disabled", 4), c.weapon_disabled_by_tier, "a damage tier entry");
    words(r, r.array(damage, "system_hits", 4), c.system_hits_by_tier, "a damage tier entry");
    words(r, r.array(damage, "actuator_loss", 4), c.actuator_loss_by_tier, "a damage tier entry");
    const Json& integrity = r.member(file, "integrity_tiers");
    r.only(integrity, {"actuator_by_condition", "system_by_hits", "weapon_by_operational"});
    words(r, r.array(integrity, "actuator_by_condition", 5), c.actuator_integrity_by_condition, "an integrity tier");
    words(r, r.array(integrity, "system_by_hits", 3), c.system_tier_by_hits, "an integrity tier");
    words(r, r.array(integrity, "weapon_by_operational", 2), c.weapon_integrity_by_operational, "an integrity tier");
    words(r, r.array(file, "opposing_objective", 5), c.opposing_objective, "an objective");
    words(r, r.array(file, "mission_side_zero_rules", kCombatMissions), c.mission_side_zero_rules, "a mission rule");
    const Json::Array& profiles = r.array(file, "continuation_profiles", kStageProfiles);
    for (std::size_t i = 0; i < kStageProfiles; ++i)
        words(r, row(r, profiles[i], kStagesPerProfile, "a continuation profile"), c.continuation_profiles[i],
              "a mission code");
    words(r, r.array(file, "heat_aim_jitter_thresholds", 5), c.heat_aim_jitter_thresholds, "a heat threshold");
    words(r, r.array(file, "range_hit_modifiers", 5), c.range_hit_modifiers, "a range modifier");
    const Json::Array& arcs = r.array(file, "hit_locations", kAttackArcs);
    for (std::size_t i = 0; i < kAttackArcs; ++i)
        name_list(r, row(r, arcs[i], kHitRolls, "a hit location row"), armour, c.hit_locations[i], "a hit location");
    const Json::Array& clusters = r.array(file, "missile_cluster_rows", kClusterRows);
    for (std::size_t i = 0; i < kClusterRows; ++i)
        words(r, row(r, clusters[i], kClusterRolls, "a missile cluster row"), c.missile_cluster_rows[i], "a missile count");
    words(r, r.array(file, "opposing_side_first_unit_slots", 2), c.opposing_side_first_unit_slots, "a unit slot");
    signed_words(r, r.array(file, "avoidance_heading_offsets", 3), c.avoidance_heading_offsets, "a heading offset");
    const Json& start = r.member(file, "start");
    r.only(start, {"side_objective_rules"});
    words(r, r.array(start, "side_objective_rules", 2), c.start_side_objective_rules, "a side objective rule");
    return c;
}

std::array<Weapon, kWeaponKinds.size()> read_weapons(const fs::path& data_dir) {
    Reader r(data_dir / "rules" / "weapons.json");
    const Json file = r.parse();
    r.only(file, {"weapons"});
    const auto rows = r.keyed(file, "weapons", kWeaponKinds);
    // null for none, else a whole number below `count`.
    const auto optional = [&](const Json& object, std::string_view key, std::size_t count) {
        const Json& v = r.member(object, key);
        if (std::holds_alternative<std::nullptr_t>(v.value)) return kNone;
        return static_cast<std::uint16_t>(r.whole(v, key, double(count - 1)));
    };
    std::array<Weapon, kWeaponKinds.size()> out;
    for (std::size_t i = 0; i < out.size(); ++i) {
        const Json& e = *rows[i];
        r.only(e, {"damage", "ammo_family", "reload_ticks", "heat", "range_class", "ranges", "critical_slots",
                   "unknown_14", "missile_cluster_row", "projectile_profile"});
        Weapon& w = out[i];
        w.damage = r.u16(e, "damage");
        w.ammo_family = optional(e, "ammo_family", kAmmoFamilies);
        w.reload_ticks = r.u16(e, "reload_ticks");
        w.heat = r.u16(e, "heat");
        w.range_class = r.u16(e, "range_class");
        const Json& ranges = r.member(e, "ranges");
        r.only(ranges, {"minimum", "short", "medium", "long"});
        w.minimum_range = r.u16(ranges, "minimum");
        w.short_range = r.u16(ranges, "short");
        w.medium_range = r.u16(ranges, "medium");
        w.long_range = r.u16(ranges, "long");
        w.critical_slots = r.u16(e, "critical_slots");
        w.unknown_14 = r.u16(e, "unknown_14");
        w.missile_cluster_row = optional(e, "missile_cluster_row", kClusterRows);
        w.projectile_profile = optional(e, "projectile_profile", kProjectileProfiles);
    }
    return out;
}

CombatAssets read_combat_assets(const fs::path& data_dir) {
    Reader r(data_dir / "rules" / "combat_assets.json");
    const Json file = r.parse();
    r.only(file, {"animation_prefix_counts", "player_pose_frame_counts", "body_damage_effect_steps",
                  "default_effect_steps", "machine_gun_effect_steps"});
    CombatAssets a;
    words(r, r.array(file, "animation_prefix_counts", 9), a.animation_prefix_counts, "a frame count");
    words(r, r.array(file, "player_pose_frame_counts", 8), a.player_pose_frame_counts, "a frame count");
    words(r, r.array(file, "body_damage_effect_steps", 4), a.body_damage_effect_steps, "an effect step");
    words(r, r.array(file, "default_effect_steps", 7), a.default_effect_steps, "an effect step");
    words(r, r.array(file, "machine_gun_effect_steps", 3), a.machine_gun_effect_steps, "an effect step");
    return a;
}

std::array<MechTemplate, kTemplates> read_mechs(const fs::path& data_dir) {
    Reader r(data_dir / "rules" / "mechs.json");
    const Json file = r.parse();
    r.only(file, {"mechs"});
    const Json::Array& mechs = r.array(file, "mechs", kTemplates);
    const std::span<const std::string_view> armour(kArmourLocations), body(kBodyLocations), weapons(kWeaponKinds);
    // A body location by name, or null for none (when `none` is allowed).
    const auto location = [&](const Json& object, bool none) {
        const Json& v = r.member(object, "location");
        if (none && std::holds_alternative<std::nullptr_t>(v.value)) return kNone;
        return static_cast<std::uint16_t>(r.name(v, body, "a location"));
    };
    std::array<MechTemplate, kTemplates> out;
    for (std::size_t i = 0; i < kTemplates; ++i) {
        const Json& m = mechs[i];
        r.only(m, {"name", "tonnage", "display_scale", "firing_movement_threshold", "maximum_speed_scale",
                   "jump_acceleration", "jump_speed_limit", "unknown_0e", "heat_sinks", "reference_points", "armour",
                   "internal", "mounts", "ammo", "jump_jets"});
        // The name is the story's; the cockpit finds it by template number.
        const std::string name = "combat.mech_template." + std::to_string(i + 1) + ".name";
        if (r.text(m, "name") != name) r.fail("template " + std::to_string(i + 1) + "'s name is not \"" + name + "\"");
        MechTemplate& t = out[i];
        t.tonnage = r.u16(m, "tonnage");
        t.display_scale = r.u16(m, "display_scale");
        t.firing_movement_threshold = r.u16(m, "firing_movement_threshold");
        t.maximum_speed_scale = r.u16(m, "maximum_speed_scale");
        t.jump_acceleration = r.u16(m, "jump_acceleration");
        t.jump_speed_limit = r.u16(m, "jump_speed_limit");
        t.unknown_0e = r.u16(m, "unknown_0e");
        t.heat_sinks = r.u16(m, "heat_sinks");
        const auto points = r.keyed(m, "reference_points", armour);
        for (std::size_t l = 0; l < armour.size(); ++l)
            signed_words(r, row(r, *points[l], 3, "a reference point"), t.reference_points[l], "a reference component");
        keyed_words(r, m, "armour", armour, {}, t.armour);
        keyed_words(r, m, "internal", body, {}, t.internal);
        const Json::Array& mounts = r.array(m, "mounts");
        if (mounts.size() > kMounts) r.fail("a template has more than " + std::to_string(kMounts) + " mounts");
        for (const Json& e : mounts) {
            r.only(e, {"weapon", "location", "unknown_4", "unknown_6"});
            t.mounts.push_back({static_cast<std::uint16_t>(r.name(r.member(e, "weapon"), weapons, "a weapon")),
                                location(e, false), r.u16(e, "unknown_4"), r.u16(e, "unknown_6")});
        }
        const Json::Array& ammo = r.array(m, "ammo", kAmmoFamilies);
        for (std::size_t f = 0; f < kAmmoFamilies; ++f) {
            r.only(ammo[f], {"location", "salvos"});
            t.ammo_location[f] = location(ammo[f], true);
            t.ammo_salvos[f] = r.u16(ammo[f], "salvos");
        }
        const Json::Array& jets = r.array(m, "jump_jets");
        if (jets.size() > kJetGroups) r.fail("a template has more than " + std::to_string(kJetGroups) + " jump jet groups");
        for (const Json& e : jets) {
            r.only(e, {"location", "critical_slots"});
            t.jump_jets.push_back({location(e, false), r.u16(e, "critical_slots")});
        }
    }
    return out;
}

void put_word(std::string& out, std::uint16_t w) {
    out += char(w & 0xFF);
    out += char(w >> 8);
}

std::uint16_t piece_offset(std::string_view id) {
    const RulePiece* piece = find_rule_piece(id);
    if (!piece) throw RulesError("the rule table has no " + std::string(id));
    return piece->offset;
}

}  // namespace

Rules load_rules(const fs::path& data_dir) {
    return {read_units(data_dir), read_market(data_dir), read_repair(data_dir), read_houses(data_dir),
            read_contracts(data_dir), read_combat(data_dir), read_weapons(data_dir), read_combat_assets(data_dir),
            read_mechs(data_dir)};
}

std::map<std::string, std::string, std::less<>> native_pieces(const Rules& rules) {
    std::map<std::string, std::string, std::less<>> out;
    const auto& mechs = rules.units.mechs;
    const auto per_mech = [&](std::string_view id, auto field) {
        std::string& bytes = out[std::string(id)];
        for (const Mech& m : mechs) bytes += char(field(m));
    };
    std::string& tonnages = out["units.tonnages"];
    for (const Mech& m : mechs) put_word(tonnages, m.tonnage);
    per_mech("units.weapons", [](const Mech& m) { return m.weapons; });
    per_mech("units.condition_fields", [](const Mech& m) { return m.condition_fields; });
    per_mech("units.display_tonnages", [](const Mech& m) { return m.display_tonnage; });
    per_mech("units.display_speeds", [](const Mech& m) { return m.display_speed_kph; });
    per_mech("units.display_jumps", [](const Mech& m) { return m.display_jump_metres; });
    per_mech("units.ammo_groups", [](const Mech& m) { return m.ammo_groups; });
    per_mech("units.heat_sinks", [](const Mech& m) { return m.heat_sinks; });
    per_mech("units.jump_jet_groups", [](const Mech& m) { return m.jump_jet_groups; });
    per_mech("units.body_repair_prices", [](const Mech& m) { return m.body_repair_price; });
    per_mech("units.force_weights", [](const Mech& m) { return m.force_weight; });
    std::string& divisors = out["units.condition_score_divisors"];
    for (std::uint8_t d : rules.units.condition_score_divisors) divisors += char(d);
    std::string& prices = out["units.base_prices"];
    for (std::uint16_t p : rules.units.base_prices) put_word(prices, p);

    // The per-unit lists, laid end to end in unit order as the exe stores
    // them, each unit's pointer the native offset of its first entry.
    const std::uint16_t entries_at = piece_offset("units.ammo_profile_entries");
    const std::uint16_t repair_at = piece_offset("units.weapon_repair_prices");
    std::string& entries = out["units.ammo_profile_entries"];
    std::string& profiles = out["units.ammo_profiles"];
    std::string& repair = out["units.weapon_repair_prices"];
    std::string& repair_lists = out["units.weapon_repair_price_lists"];
    for (const Mech& m : mechs) {
        put_word(profiles, std::uint16_t(entries_at + entries.size()));
        for (const AmmoProfileEntry& e : m.ammo_profile) {
            put_word(entries, e.family);
            put_word(entries, e.quantity_maximum);
        }
        put_word(repair_lists, std::uint16_t(repair_at + repair.size()));
        for (std::uint16_t p : m.weapon_repair_prices) put_word(repair, p);
    }
    if (entries.size() > kAmmoProfileEntries * 4)
        throw RulesError("the units' ammo profiles have more than " + std::to_string(kAmmoProfileEntries) + " entries");
    if (repair.size() > kWeaponRepairPrices * 2)
        throw RulesError("the units' weapon repair prices are more than " + std::to_string(kWeaponRepairPrices));
    entries.resize(kAmmoProfileEntries * 4, '\0');
    repair.resize(kWeaponRepairPrices * 2, '\0');

    const Market& market = rules.market;
    std::string& ammo = out["market.ammunition_prices"];
    for (std::uint16_t p : market.ammunition_buy) put_word(ammo, p);
    for (std::uint16_t p : market.ammunition_sell) put_word(ammo, p);
    std::string& tiers = out["market.price_tier_multipliers"];
    for (std::uint8_t t : market.price_tier_multipliers) tiers += char(t);
    std::string& maxima = out["market.stock_kind_range_maxima"];
    std::string& starts = out["market.stock_kind_range_starts"];
    for (const StockKindRange& range : market.stock_kind_ranges) {
        maxima += char(range.maximum);
        starts += char(range.start);
    }
    std::string& freight = out["market.freight_rates"];
    for (std::uint16_t f : market.freight_rates) put_word(freight, f);
    std::string& transaction = out["market.start.ammunition_transaction_prices"];
    for (std::uint16_t p : market.start.ammunition_buy) put_word(transaction, p);
    for (std::uint16_t p : market.start.ammunition_sell) put_word(transaction, p);
    out["market.start.generate_starting_jenner"] = std::string(1, char(market.start.generate_starting_jenner));
    for (std::uint16_t k : market.start.generated_ammo_kinds) put_word(out["market.start.generated_ammo_kinds"], k);
    for (std::uint16_t q : market.start.generated_ammo_quantities)
        put_word(out["market.start.generated_ammo_quantities"], q);

    std::string& system_prices = out["repair.system_prices"];
    for (const auto& row : rules.repair.system_prices)
        for (std::uint16_t p : row) put_word(system_prices, p);
    std::string& limits = out["repair.success_roll_limits"];
    for (std::uint8_t l : rules.repair.success_roll_limits) limits += char(l);

    const auto per_house = [&](std::string_view id, auto field) {
        std::string& bytes = out[std::string(id)];
        for (const House& h : rules.houses) put_word(bytes, std::uint16_t(field(h)));
    };
    per_house("houses.advance_base_percent", [](const House& h) { return h.advance_base_percent; });
    per_house("houses.salvage_base_percent", [](const House& h) { return h.salvage_base_percent; });
    per_house("houses.payment_roll_bias", [](const House& h) { return h.payment_roll_bias; });
    per_house("houses.payment_adjustment_percent", [](const House& h) { return h.payment_adjustment_percent; });
    per_house("houses.deduction_roll_thresholds", [](const House& h) { return h.deduction_roll_threshold; });
    per_house("houses.reduced_payout_percent", [](const House& h) { return h.reduced_payout_percent; });
    per_house("houses.extra_enemy_roll_thresholds", [](const House& h) { return h.extra_enemy_roll_threshold; });
    per_house("houses.planet_range_starts", [](const House& h) { return h.first_planet - 1; });
    per_house("houses.planet_range_counts", [](const House& h) { return h.planet_count; });
    std::string& negotiation = out["houses.negotiation_attempt_base"];
    std::string& type_limits = out["houses.mission_type_limits"];
    std::string& policies = out["houses.mission_planet_policies"];
    std::string& policy_offsets = out["houses.mission_planet_policy_offsets"];
    const std::uint16_t policies_at = piece_offset("houses.mission_planet_policies");
    for (const House& h : rules.houses) {
        negotiation += char(h.negotiation_attempt_base);
        for (std::uint8_t l : h.mission_type_limits) type_limits += char(l);
        // Each House points at its own policy, in House order, as the exe's do.
        put_word(policy_offsets, std::uint16_t(policies_at + policies.size()));
        for (const auto& row : h.mission_planet_policy)
            for (std::uint8_t planet : row) policies += char(planet);
    }

    const Contracts& contracts = rules.contracts;
    for (std::uint16_t u : contracts.deployment_units_by_scenario) put_word(out["contracts.deployment_units_by_scenario"], u);
    for (std::uint16_t w : contracts.pilot_wages) put_word(out["contracts.pilot_wages"], w);
    const ContractsStart& start = contracts.start;
    put_word(out["contracts.start.mission_force_budget"], start.mission_force_budget);
    put_word(out["contracts.start.mission_payment_base_thousands"], start.mission_payment_base_thousands);
    put_word(out["contracts.start.pilot_card_gunnery"], start.pilot_card_gunnery);
    put_word(out["contracts.start.pilot_card_piloting"], start.pilot_card_piloting);
    put_word(out["contracts.start.pilot_card_quoted_wage"], start.pilot_card_quoted_wage);
    put_word(out["contracts.start.settlement_payout_percent"], start.settlement_payout_percent);
    put_word(out["contracts.start.travel_destination_time_weight"], start.travel_destination_time_weight);

    const auto put_words = [&](std::string_view id, const auto& values) {
        std::string& bytes = out[std::string(id)];
        for (auto v : values) put_word(bytes, static_cast<std::uint16_t>(v));
    };
    const Combat& combat = rules.combat;
    put_words("combat.body_of_armour", combat.body_of_armour);
    put_words("combat.campaign_body_tier_of_armour", combat.campaign_body_tier_of_armour);
    put_words("combat.critical_transfer", combat.critical_transfer);
    put_words("combat.damage_transfer", combat.damage_transfer);
    put_words("combat.body_reference_point", combat.body_reference_point);
    put_words("combat.actuator_of_limb", combat.actuator_of_limb);
    put_words("combat.template_of_unit_kind", combat.template_of_unit_kind);
    put_words("combat.unit_kind_of_template", combat.unit_kind_of_template);
    put_words("combat.fallback_start_template", combat.fallback_start_template);
    put_words("combat.fallback_template_steps", combat.fallback_template_steps);
    std::string& costs = out["combat.template_budget_costs"];
    for (std::uint32_t cost : combat.template_budget_costs) {
        put_word(costs, static_cast<std::uint16_t>(cost));
        put_word(costs, static_cast<std::uint16_t>(cost >> 16));
    }
    put_words("combat.weapon_disabled_by_tier", combat.weapon_disabled_by_tier);
    put_words("combat.system_hits_by_tier", combat.system_hits_by_tier);
    put_words("combat.actuator_loss_by_tier", combat.actuator_loss_by_tier);
    put_words("combat.actuator_integrity_by_condition", combat.actuator_integrity_by_condition);
    put_words("combat.system_tier_by_hits", combat.system_tier_by_hits);
    put_words("combat.weapon_integrity_by_operational", combat.weapon_integrity_by_operational);
    put_words("combat.opposing_objective", combat.opposing_objective);
    put_words("combat.mission_side_zero_rules", combat.mission_side_zero_rules);
    for (const auto& profile : combat.continuation_profiles) put_words("combat.continuation_profiles", profile);
    put_words("combat.heat_aim_jitter_thresholds", combat.heat_aim_jitter_thresholds);
    put_words("combat.range_hit_modifiers", combat.range_hit_modifiers);
    for (const auto& arc : combat.hit_locations) put_words("combat.hit_locations", arc);
    for (const auto& cluster : combat.missile_cluster_rows) put_words("combat.missile_cluster_rows", cluster);
    put_words("combat.opposing_side_first_unit_slots", combat.opposing_side_first_unit_slots);
    put_words("combat.avoidance_heading_offsets", combat.avoidance_heading_offsets);
    put_words("combat.start.side_objective_rules", combat.start_side_objective_rules);

    std::string& definitions = out["weapons.definitions"];
    for (const Weapon& w : rules.weapons)
        for (std::uint16_t v : {w.damage, w.ammo_family, w.reload_ticks, w.heat, w.range_class, w.minimum_range,
                                w.short_range, w.medium_range, w.long_range, w.critical_slots, w.unknown_14})
            put_word(definitions, v);
    for (const Weapon& w : rules.weapons) put_word(out["weapons.projectile_profiles"], w.projectile_profile);
    for (const Weapon& w : rules.weapons) put_word(out["weapons.missile_cluster_rows"], w.missile_cluster_row);

    const CombatAssets& assets = rules.assets;
    put_words("assets.animation_prefix_counts", assets.animation_prefix_counts);
    put_words("assets.player_pose_frame_counts", assets.player_pose_frame_counts);
    put_words("assets.body_damage_effect_steps", assets.body_damage_effect_steps);
    put_words("assets.default_effect_steps", assets.default_effect_steps);
    put_words("assets.machine_gun_effect_steps", assets.machine_gun_effect_steps);

    // Each template as the exe's 0xE0-byte row; unused mounts and jump jet
    // groups hold the exe's filler (no weapon, no location).
    std::string& templates = out["mechs.templates"];
    for (const MechTemplate& t : rules.mechs) {
        for (std::uint16_t v : {t.tonnage, t.display_scale, t.firing_movement_threshold, t.maximum_speed_scale,
                                static_cast<std::uint16_t>(t.jump_jets.size()), t.jump_acceleration,
                                t.jump_speed_limit, t.unknown_0e, t.heat_sinks,
                                static_cast<std::uint16_t>(t.mounts.size())})
            put_word(templates, v);
        for (const auto& point : t.reference_points)
            for (std::int16_t c : point) put_word(templates, static_cast<std::uint16_t>(c));
        for (std::size_t i = 0; i < kMounts; ++i) {
            const MechTemplate::Mount m = i < t.mounts.size() ? t.mounts[i] : MechTemplate::Mount{kNone, kNone, 0, 0};
            for (std::uint16_t v : {m.weapon, m.location, m.unknown_4, m.unknown_6}) put_word(templates, v);
        }
        for (std::uint16_t l : t.ammo_location) put_word(templates, l);
        for (std::size_t i = 0; i < kJetGroups; ++i) {
            const MechTemplate::JumpJets j = i < t.jump_jets.size() ? t.jump_jets[i] : MechTemplate::JumpJets{kNone, 0};
            put_word(templates, j.location);
            put_word(templates, j.critical_slots);
        }
        for (std::uint16_t a : t.armour) put_word(templates, a);
        for (std::uint16_t s : t.internal) put_word(templates, s);
        for (std::uint16_t s : t.ammo_salvos) put_word(templates, s);
    }
    return out;
}

}  // namespace mw1o::rules
