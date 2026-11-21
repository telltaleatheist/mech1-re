#include "Rules.h"

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
    r.only(file, {"price_tier_multipliers", "ammunition_prices", "stock_kind_ranges", "freight_rates"});
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

Rules load_rules(const fs::path& data_dir) { return {read_units(data_dir), read_market(data_dir)}; }

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
    return out;
}

}  // namespace mw1o::rules
