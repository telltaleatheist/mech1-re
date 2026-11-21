// Written by tools/import/make_import_table.py --rules from rule_pieces.txt; don't edit.
#include "RuleTable.h"

namespace mw1o::rules {

namespace {
constexpr RulePiece kTable[] = {
    {"units.tonnages", "MW_MAIN.EXE", 0x1090, 0x122B, 20, "e8f0299862d1f9fe"},
    {"units.weapons", "MW_MAIN.EXE", 0x1090, 0x2AED, 10, "3083ec30c3988fed"},
    {"units.condition_fields", "MW_MAIN.EXE", 0x1090, 0x2AE3, 10, "6cf7047b9014bcad"},
    {"units.condition_score_divisors", "MW_MAIN.EXE", 0x1090, 0x2B18, 11, "4fba0f2bf22d6aa9"},
    {"units.display_tonnages", "MW_MAIN.EXE", 0x1090, 0x2B65, 10, "a603b2d4fd0671a1"},
    {"units.display_speeds", "MW_MAIN.EXE", 0x1090, 0x2B6F, 10, "ba1e703018a16e77"},
    {"units.display_jumps", "MW_MAIN.EXE", 0x1090, 0x2B79, 10, "69faeb392ab91acd"},
    {"units.ammo_groups", "MW_MAIN.EXE", 0x1090, 0x2EEE, 10, "9a46cd38eca33fdc"},
    {"units.ammo_profiles", "MW_MAIN.EXE", 0x1090, 0x2F28, 20, "319c71d801c02007"},
    {"units.ammo_profile_entries", "MW_MAIN.EXE", 0x1090, 0x2F3C, 60, "8f76acf2958ab37b"},
    {"units.base_prices", "MW_MAIN.EXE", 0x1090, 0x305F, 22, "3d0627ad66d42b14"},
    {"units.heat_sinks", "MW_MAIN.EXE", 0x1090, 0x368C, 10, "dc0da4f3f822dc33"},
    {"units.jump_jet_groups", "MW_MAIN.EXE", 0x1090, 0x3696, 10, "5d47d05ec9806762"},
    {"units.weapon_repair_price_lists", "MW_MAIN.EXE", 0x1090, 0x3C1B, 20, "fed8a7b2bf0bcb60"},
    {"units.weapon_repair_prices", "MW_MAIN.EXE", 0x1090, 0x3C2F, 124, "360960cc8d5ccbca"},
    {"units.body_repair_prices", "MW_MAIN.EXE", 0x1090, 0x3CAB, 10, "96b7c6adbdafbb2e"},
    {"units.force_weights", "MW_MAIN.EXE", 0x1090, 0x4B55, 10, "4bcab9d750596f5e"},
    {"market.ammunition_prices", "MW_MAIN.EXE", 0x1090, 0x2EF8, 24, "51c8e71f02806e76"},
    {"market.price_tier_multipliers", "MW_MAIN.EXE", 0x1090, 0x3057, 4, "a3cb794e52a64e65"},
    {"market.stock_kind_range_maxima", "MW_MAIN.EXE", 0x1090, 0x3A37, 3, "f63893d26c31b9b1"},
    {"market.stock_kind_range_starts", "MW_MAIN.EXE", 0x1090, 0x3A3A, 3, "66f4f714063c1f06"},
    {"market.freight_rates", "MW_MAIN.EXE", 0x1090, 0x5C94, 12, "edbc19d9ad92ba4b"},
};
}  // namespace

std::span<const RulePiece> rule_table() { return kTable; }

}  // namespace mw1o::rules
