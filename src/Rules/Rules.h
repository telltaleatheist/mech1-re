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

struct Market {
    std::array<std::uint8_t, 4> price_tier_multipliers{};
    std::array<std::uint16_t, kAmmoFamilies> ammunition_buy{}, ammunition_sell{};
    std::array<StockKindRange, 3> stock_kind_ranges{};
    std::array<std::uint16_t, kAmmoFamilies> freight_rates{};
};

struct Rules {
    Units units;
    Market market;
};

// <data_dir>/rules/*.json, read strictly. Throws RulesError naming the file.
Rules load_rules(const std::filesystem::path& data_dir);

// Every rule piece's bytes as the exe lays them out (RuleTable.h), by id:
// what the game fills its owners from, and what rules_test hashes. Throws
// RulesError if a per-unit list doesn't fit the exe's storage.
std::map<std::string, std::string, std::less<>> native_pieces(const Rules& rules);

}  // namespace mw1o::rules
