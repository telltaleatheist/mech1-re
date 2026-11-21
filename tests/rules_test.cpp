// src/Rules: data/rules rebuilds every table of the rule table byte for byte
// (its SHA-256 prefix, from the 1990-01-22 exes; no game files), and the
// strict reading refuses what doesn't fit, on made-up copies of the files.
//
//   rules_test <data dir>
// Prints ids and counts only.
#include "../src/Rules/NewCampaignFields.h"
#include "../src/Rules/RuleTable.h"
#include "../src/Rules/Rules.h"
#include "../src/Support/Json.h"
#include "../src/Support/Sha256.h"

#include <cstdio>
#include <fstream>
#include <functional>
#include <iterator>
#include <string>

namespace fs = std::filesystem;
using namespace mw1o::rules;
using mw1o::support::Json;

namespace {

int g_failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        ++g_failures;
        std::printf("FAIL %s\n", what.c_str());
    }
}

void refused(const std::function<void()>& run, const std::string& want, const std::string& what) {
    try {
        run();
        check(false, what + ": accepted");
    } catch (const RulesError& e) {
        check(std::string(e.what()).find(want) != std::string::npos,
              what + ": \"" + e.what() + "\" doesn't say \"" + want + "\"");
    }
}

std::string read(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

void write(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary) << text;
}

// A Json value written back (numbers are whole in these files).
std::string dump(const Json& v) {
    if (v.is_object()) {
        std::string out = "{";
        for (const auto& [key, member] : v.object()) out += (out.size() > 1 ? "," : "") + mw1o::support::json_quote(key) + ":" + dump(member);
        return out + "}";
    }
    if (v.is_array()) {
        std::string out = "[";
        for (const Json& e : v.array()) out += (out.size() > 1 ? "," : "") + dump(e);
        return out + "]";
    }
    if (v.is_string()) return mw1o::support::json_quote(v.string());
    if (v.is_number()) return std::to_string(static_cast<long long>(v.number()));
    return "null";
}

Json& at(Json& v, const std::string& key) { return std::get<Json::Object>(v.value).at(key); }

void the_rules_rebuild_the_exes_tables(const fs::path& data) {
    const auto pieces = native_pieces(load_rules(data));
    int exact = 0, tuned = 0;
    for (const RulePiece& piece : rule_table()) {
        const auto it = pieces.find(piece.id);
        if (it == pieces.end()) {
            check(false, std::string(piece.id) + ": the rules don't make it");
            continue;
        }
        check(it->second.size() == piece.length, std::string(piece.id) + ": " + std::to_string(it->second.size()) +
                                                     " bytes, the table's " + std::to_string(piece.length));
        if (piece.sha16.empty()) {
            ++tuned;
            continue;
        }
        const auto* bytes = reinterpret_cast<const std::uint8_t*>(it->second.data());
        const bool same = mw1o::support::sha256_hex({bytes, it->second.size()}).substr(0, 16) == piece.sha16;
        check(same, std::string(piece.id) + " differs from the exe's");
        exact += same;
    }
    for (const auto& [id, bytes] : pieces) check(find_rule_piece(id) != nullptr, id + ": not in the rule table");
    std::printf("%zu rule tables: %d as the exe's, %d tuned\n", rule_table().size(), exact, tuned);
}

// The save codec's fields, as new_campaign.json names them, cover the save
// block once each, end to end.
void the_new_campaign_fields_tile_the_save_block() {
    std::size_t at = 0;
    for (const NewCampaignField& field : new_campaign_fields()) {
        check(field.offset == at, std::string(field.name) + " is not where the field before it ends");
        at = std::size_t(field.offset) + field.length;
    }
    check(at == kSaveBlockBytes, "the fields end at " + std::to_string(at) + ", not the block's end");
}

void what_doesnt_fit_is_refused(const fs::path& data, const fs::path& work) {
    const auto restore = [&] {
        fs::create_directories(work / "rules");
        for (const auto& entry : fs::directory_iterator(data / "rules"))
            fs::copy_file(entry.path(), work / "rules" / entry.path().filename(), fs::copy_options::overwrite_existing);
    };
    const auto with_file = [&](const std::string& name, const std::function<void(Json&)>& change) {
        restore();
        Json copy = mw1o::support::parse_json(read(data / "rules" / name));
        change(copy);
        write(work / "rules" / name, dump(copy));
    };
    const auto with_units = [&](const std::function<void(Json&)>& change) { with_file("units.json", change); };
    const auto load = [&] { native_pieces(load_rules(work)); };

    with_units([](Json& u) { std::get<Json::Object>(at(u, "units").value).erase("wasp"); });
    refused(load, "no \"wasp\"", "a unit kind missing");
    with_units([](Json& u) { std::get<Json::Object>(at(u, "units").value)["jaguar"] = at(at(u, "units"), "wasp"); });
    refused(load, "unknown key \"jaguar\"", "an unknown unit kind");
    with_units([](Json& u) { std::get<Json::Object>(at(at(u, "units"), "locust").value)["armour"] = Json{1.0}; });
    refused(load, "unknown key \"armour\"", "an unknown field");
    with_units([](Json& u) { std::get<Json::Object>(at(at(u, "units"), "dropship").value)["tonnage"] = Json{1.0}; });
    refused(load, "unknown key \"tonnage\"", "a mech field on the DropShip");
    with_units([](Json& u) { at(at(at(u, "units"), "locust"), "heat_sinks") = Json{256.0}; });
    refused(load, "heat_sinks is not a whole number from 0 to 255", "a byte past 255");
    with_units([](Json& u) { at(at(at(u, "units"), "locust"), "weapons") = Json{4.0}; });
    refused(load, "\"weapon_repair_prices\" has 4 entries, not 5", "repair prices not one per repair row");
    with_units([](Json& u) {
        Json& locust = at(at(u, "units"), "locust");
        Json::Array profile(15, at(locust, "ammo_profile").array()[0]);
        at(locust, "ammo_profile") = Json{profile};
        at(locust, "ammo_groups") = Json{15.0};
    });
    refused(load, "more than 15 entries", "ammo profiles past the exe's storage");

    with_file("market.json", [](Json& m) {
        Json::Array rates = at(m, "freight_rates").array();
        rates.push_back(Json{1.0});
        at(m, "freight_rates") = Json{rates};
    });
    refused(load, "\"freight_rates\" has 7 entries, not 6", "a seventh freight rate");
    with_file("market.json", [](Json& m) { std::get<Json::Object>(m.value).erase("start"); });
    refused(load, "no \"start\"", "the market's start values missing");
    with_file("houses.json", [](Json& h) { std::get<Json::Object>(at(h, "houses").value).erase("liao"); });
    refused(load, "no \"liao\"", "a House missing");
    with_file("houses.json", [](Json& h) { at(at(at(h, "houses"), "marik"), "payment_adjustment_percent") = Json{-40000.0}; });
    refused(load, "from -32768 to 32767", "a signed word out of range");
    with_file("houses.json", [](Json& h) {
        Json& policy = at(at(at(h, "houses"), "davion"), "mission_planet_policy");
        Json::Array rows = policy.array();
        Json::Array row = rows[1].array();
        row[0] = Json{0.0};
        rows[1] = Json{row};
        policy = Json{rows};
    });
    refused(load, "not a planet id from 1 to 254", "a policy planet 0");
    with_file("houses.json", [](Json& h) { at(at(at(at(h, "houses"), "kurita"), "planets"), "first") = Json{0.0}; });
    refused(load, "first planet is not a planet id", "a House's first planet 0");
    with_file("contracts.json", [](Json& c) { std::get<Json::Object>(at(c, "pilot_wages").value)["elite"] = Json{1.0}; });
    refused(load, "unknown key \"elite\"", "an unknown skill tier");
    with_file("repair.json", [](Json& r) {
        Json::Array rows = at(r, "system_prices").array();
        rows.pop_back();
        at(r, "system_prices") = Json{rows};
    });
    refused(load, "\"system_prices\" has 9 entries, not 10", "nine systems");

    with_file("combat.json", [](Json& c) { std::get<Json::Object>(at(c, "body_of_armour").value)["tail"] = Json{std::string("head")}; });
    refused(load, "unknown key \"tail\"", "an unknown armour location");
    with_file("combat.json", [](Json& c) { at(at(c, "damage_transfer"), "head") = Json{std::string("cockpit")}; });
    refused(load, "is not one of its names", "a location that isn't one");
    with_file("combat.json", [](Json& c) { at(at(c, "critical_transfer"), "head") = Json{std::string("rear_center_torso")}; });
    refused(load, "is not one of its names", "an armour location where a body location goes");
    with_file("combat.json", [](Json& c) { at(at(c, "template_of_unit_kind"), "wasp") = Json{0.0}; });
    refused(load, "a template is not a number from 1 to 8", "template 0");
    with_file("combat.json", [](Json& c) {
        Json::Array arcs = at(c, "hit_locations").array();
        Json::Array arc = arcs[2].array();
        arc.pop_back();
        arcs[2] = Json{arc};
        at(c, "hit_locations") = Json{arcs};
    });
    refused(load, "a hit location row is not a list of 11", "a short hit location row");
    with_file("combat.json", [](Json& c) {
        Json::Array offsets = at(c, "avoidance_heading_offsets").array();
        offsets[1] = Json{40000.0};
        at(c, "avoidance_heading_offsets") = Json{offsets};
    });
    refused(load, "from -32768 to 32767", "a heading offset out of range");
    with_file("combat.json", [](Json& c) { std::get<Json::Object>(c.value)["start"] = Json{Json::Object{}}; });
    refused(load, "unknown key \"start\"", "a start value written before it's read");
    with_file("weapons.json", [](Json& w) { std::get<Json::Object>(at(w, "weapons").value).erase("ppc"); });
    refused(load, "no \"ppc\"", "a weapon kind missing");
    with_file("weapons.json", [](Json& w) { at(at(at(w, "weapons"), "ac5"), "ammo_family") = Json{6.0}; });
    refused(load, "ammo_family is not a whole number from 0 to 5", "an ammo family past the last");
    with_file("weapons.json", [](Json& w) { std::get<Json::Object>(at(at(at(w, "weapons"), "srm2"), "ranges").value)["extreme"] = Json{9.0}; });
    refused(load, "unknown key \"extreme\"", "an unknown range band");
    with_file("combat_assets.json", [](Json& a) {
        Json::Array counts = at(a, "animation_prefix_counts").array();
        counts.pop_back();
        at(a, "animation_prefix_counts") = Json{counts};
    });
    refused(load, "\"animation_prefix_counts\" has 8 entries, not 9", "eight animation counts");
    const auto with_mech = [&](std::size_t index, const std::function<void(Json&)>& change) {
        with_file("mechs.json", [&](Json& file) {
            Json::Array mechs = at(file, "mechs").array();
            change(mechs[index]);
            at(file, "mechs") = Json{mechs};
        });
    };
    with_mech(2, [](Json& m) { at(m, "name") = Json{std::string("combat.mech_template.4.name")}; });
    refused(load, "template 3's name is not", "a template named for another");
    with_mech(7, [](Json& m) {
        Json::Array mounts = at(m, "mounts").array();
        mounts.push_back(mounts[0]);
        at(m, "mounts") = Json{mounts};
    });
    refused(load, "more than 10 mounts", "an eleventh mount");
    with_mech(0, [](Json& m) {
        Json::Array mounts = at(m, "mounts").array();
        at(mounts[0], "weapon") = Json{std::string("flamer")};
        at(m, "mounts") = Json{mounts};
    });
    refused(load, "a weapon is not one of its names", "a weapon that isn't one");
    with_mech(1, [](Json& m) {
        Json::Array jets = at(m, "jump_jets").array();
        at(jets[0], "location") = Json{nullptr};
        at(m, "jump_jets") = Json{jets};
    });
    refused(load, "a location is not one of its names", "a jump jet group without a location");
    with_mech(4, [](Json& m) { std::get<Json::Object>(at(m, "internal").value).erase("head"); });
    refused(load, "no \"head\"", "an internal structure location missing");
    with_mech(5, [](Json& m) { std::get<Json::Object>(at(m, "armour").value)["rear_left_torso"] = Json{1.0}; });
    refused(load, "unknown key \"rear_left_torso\"", "an armour location that isn't one");
    with_file("new_campaign.json", [](Json& n) { at(n, "commanderNameBytes") = Json{std::string("commander.name")}; });
    refused(load, "\"commanderNameBytes\" is not \"commander.default_name\"", "the commander's name not its text id");
    with_file("new_campaign.json", [](Json& n) { std::get<Json::Object>(n.value).erase("calendarYear"); });
    refused(load, "no \"calendarYear\"", "a save block field missing");
    with_file("new_campaign.json", [](Json& n) { std::get<Json::Object>(n.value)["campaignFunds"] = Json{1.0}; });
    refused(load, "unknown key \"campaignFunds\"", "a field the codec doesn't have");
    with_file("new_campaign.json", [](Json& n) { at(n, "commanderAgeYears") = Json{70000.0}; });
    refused(load, "commanderAgeYears is not a whole number from 0 to 65535", "a word past 65535");
    with_file("new_campaign.json", [](Json& n) {
        Json::Array ids = at(n, "rosterPilotIds").array();
        ids.pop_back();
        at(n, "rosterPilotIds") = Json{ids};
    });
    refused(load, "\"rosterPilotIds\" has 3 entries, not 4", "a short word list");
    fs::remove(work / "rules" / "market.json");
    refused(load, "can't read", "a missing file");
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::printf("usage: rules_test <data dir>\n");
        return 2;
    }
    const fs::path work = fs::current_path() / "rules_test_work";
    fs::remove_all(work);
    try {
        the_rules_rebuild_the_exes_tables(argv[1]);
        the_new_campaign_fields_tile_the_save_block();
        what_doesnt_fit_is_refused(argv[1], work);
    } catch (const std::exception& e) {
        check(false, std::string("unexpected: ") + e.what());
    }
    fs::remove_all(work);
    std::printf("%s\n", g_failures ? "FAILED" : "ok");
    return g_failures ? 1 : 0;
}
