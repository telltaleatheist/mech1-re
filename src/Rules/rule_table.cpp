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
    {"market.start.ammunition_transaction_prices", "MW_MAIN.EXE", 0x1090, 0x2F10, 24, "51c8e71f02806e76"},
    {"market.start.generated_ammo_kinds", "MW_MAIN.EXE", 0x1090, 0x3A5E, 6, "b0f66adc83641586"},
    {"market.start.generated_ammo_quantities", "MW_MAIN.EXE", 0x1090, 0x3A64, 6, "b0f66adc83641586"},
    {"market.start.generate_starting_jenner", "MW_MAIN.EXE", 0x1090, 0x3A6A, 1, "6e340b9cffb37a98"},
    {"repair.success_roll_limits", "MW_MAIN.EXE", 0x1090, 0x3A6B, 4, "16e074a6abb3de5f"},
    {"repair.system_prices", "MW_MAIN.EXE", 0x1090, 0x3B53, 200, "fffc01ef804a4b31"},
    {"houses.negotiation_attempt_base", "MW_MAIN.EXE", 0x1090, 0x11A3, 5, "51c4e0b8fdff7688"},
    {"houses.deduction_roll_thresholds", "MW_MAIN.EXE", 0x1090, 0x1241, 10, "d71c487105140770"},
    {"houses.reduced_payout_percent", "MW_MAIN.EXE", 0x1090, 0x124B, 10, "05bfb06d079f22ae"},
    {"houses.payment_roll_bias", "MW_MAIN.EXE", 0x1090, 0x4B41, 10, "84bd6b0143c99a5f"},
    {"houses.payment_adjustment_percent", "MW_MAIN.EXE", 0x1090, 0x4B4B, 10, "f15c7da923b98dff"},
    {"houses.salvage_base_percent", "MW_MAIN.EXE", 0x1090, 0x4B5F, 10, "4ab8005a113e1ed9"},
    {"houses.advance_base_percent", "MW_MAIN.EXE", 0x1090, 0x4B69, 10, "bd83680516653851"},
    {"houses.extra_enemy_roll_thresholds", "MW_MAIN.EXE", 0x1090, 0x4BF0, 10, "e819d8656ea53cb5"},
    {"houses.mission_type_limits", "MW_MAIN.EXE", 0x1090, 0x5CBA, 40, "b382aeff2141ff08"},
    {"houses.planet_range_starts", "MW_MAIN.EXE", 0x1090, 0x655C, 10, "a05a1cec153db22f"},
    {"houses.planet_range_counts", "MW_MAIN.EXE", 0x1090, 0x6566, 10, "1ac306ce2b30b457"},
    {"houses.mission_planet_policy_offsets", "MW_MAIN.EXE", 0x1090, 0x8E90, 10, "0e922fc2171ca124"},
    {"houses.mission_planet_policies", "MW_MAIN.EXE", 0x1090, 0x8E9A, 320, "3511276f6ff4132f"},
    {"contracts.start.settlement_payout_percent", "MW_MAIN.EXE", 0x1090, 0x123F, 2, "96a296d224f285c6"},
    {"contracts.start.pilot_card_gunnery", "MW_MAIN.EXE", 0x1090, 0x2440, 2, "96a296d224f285c6"},
    {"contracts.start.pilot_card_piloting", "MW_MAIN.EXE", 0x1090, 0x2442, 2, "96a296d224f285c6"},
    {"contracts.start.pilot_card_quoted_wage", "MW_MAIN.EXE", 0x1090, 0x2446, 2, "96a296d224f285c6"},
    {"contracts.pilot_wages", "MW_MAIN.EXE", 0x1090, 0x2476, 8, "619e7bd045f78bca"},
    {"contracts.start.mission_force_budget", "MW_MAIN.EXE", 0x1090, 0x4B79, 2, "96a296d224f285c6"},
    {"contracts.start.mission_payment_base_thousands", "MW_MAIN.EXE", 0x1090, 0x4B7B, 2, "96a296d224f285c6"},
    {"contracts.start.travel_destination_time_weight", "MW_MAIN.EXE", 0x1090, 0x54DA, 2, "96a296d224f285c6"},
    {"contracts.deployment_units_by_scenario", "MW_MAIN.EXE", 0x1090, 0x8E46, 74, "787a4b9fa40e8783"},
};
}  // namespace

std::span<const RulePiece> rule_table() { return kTable; }

}  // namespace mw1o::rules
