// src/Ui: data/ui rebuilds every piece of the layout table byte for byte
// (its SHA-256 prefix, from the 1990-01-22 exes; no game files), and the
// strict reading refuses what doesn't fit, on made-up copies of the files.
//
//   ui_test <data dir>
// Prints ids and counts only.
#include "../src/Support/Json.h"
#include "../src/Support/Sha256.h"
#include "../src/Ui/Ui.h"
#include "../src/Ui/UiTable.h"

#include <cstdio>
#include <fstream>
#include <functional>
#include <iterator>
#include <string>

namespace fs = std::filesystem;
using namespace mw1o::ui;
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
    } catch (const UiError& e) {
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
    if (std::holds_alternative<bool>(v.value)) return std::get<bool>(v.value) ? "true" : "false";
    return "null";
}

Json& at(Json& v, const std::string& key) { return std::get<Json::Object>(v.value).at(key); }
Json& at(Json& v, std::size_t index) { return std::get<Json::Array>(v.value).at(index); }
Json::Object& object(Json& v) { return std::get<Json::Object>(v.value); }
Json::Array& array(Json& v) { return std::get<Json::Array>(v.value); }

void the_layout_rebuilds_the_exes_tables(const fs::path& data) {
    const auto pieces = load_layout(data);
    int exact = 0, tuned = 0;
    for (const UiPiece& piece : ui_table()) {
        const auto it = pieces.find(piece.id);
        if (it == pieces.end()) {
            check(false, std::string(piece.id) + ": the layout doesn't make it");
            continue;
        }
        if (piece.sha16.empty()) {
            ++tuned;
            continue;
        }
        const auto* bytes = reinterpret_cast<const std::uint8_t*>(it->second.data());
        const bool same = mw1o::support::sha256_hex({bytes, it->second.size()}).substr(0, 16) == piece.sha16;
        check(same, std::string(piece.id) + " differs from the exe's");
        exact += same;
    }
    for (const auto& [id, bytes] : pieces) check(find_ui_piece(id) != nullptr, id + ": not in the layout table");
    std::printf("%zu layout tables: %d as the exe's, %d tuned\n", ui_table().size(), exact, tuned);
}

void what_doesnt_fit_is_refused(const fs::path& data, const fs::path& work) {
    const auto restore = [&] {
        fs::create_directories(work / "ui");
        for (const auto& entry : fs::directory_iterator(data / "ui"))
            fs::copy_file(entry.path(), work / "ui" / entry.path().filename(), fs::copy_options::overwrite_existing);
    };
    const auto with_file = [&](const std::string& name, const std::function<void(Json&)>& change) {
        restore();
        Json copy = mw1o::support::parse_json(read(data / "ui" / name));
        change(copy);
        write(work / "ui" / name, dump(copy));
    };
    const auto with_cockpit = [&](const std::function<void(Json&)>& change) { with_file("cockpit.json", change); };
    const auto with_orders = [&](const std::function<void(Json&)>& change) { with_file("orders.json", change); };
    const auto load = [&] { load_layout(work); };
    const auto first_template = [](Json& c) -> Json& { return at(at(c, "templates"), 0); };
    const auto first_layout = [](Json& c) -> Json& { return at(at(c, "layouts"), 0); };

    with_cockpit([&](Json& c) { at(first_template(c), "cockpit_layout") = Json{3.0}; });
    refused(load, "\"cockpit_layout\" 3 names no layout", "a template naming a layout past the last");
    with_cockpit([&](Json& c) { array(at(c, "templates")).pop_back(); });
    refused(load, "\"templates\" has 7 entries, not 8", "a template missing");
    with_cockpit([&](Json& c) { object(at(at(first_template(c), "target_diagram"), "regions")).erase("head"); });
    refused(load, "no \"head\"", "an armour region missing");
    with_cockpit([&](Json& c) { object(at(at(first_template(c), "target_diagram"), "regions"))["tail"] = Json{1.0}; });
    refused(load, "unknown key \"tail\"", "an unknown armour location");
    with_cockpit([&](Json& c) { array(at(first_layout(c), "viewport")).pop_back(); });
    refused(load, "viewport is not a list of 4 numbers", "a rectangle of three");
    with_cockpit([&](Json& c) { at(at(first_layout(c), "viewport"), 0) = Json{40000.0}; });
    refused(load, "viewport is not a whole number from -32768 to 32767", "a coordinate past a signed word");
    with_cockpit([&](Json& c) { object(first_layout(c))["colour"] = Json{1.0}; });
    refused(load, "unknown key \"colour\"", "an unknown layout field");
    with_cockpit([&](Json& c) { object(at(at(first_layout(c), "bitmap_placements"), 0)).erase("bitmap"); });
    refused(load, "no \"bitmap\"", "a placement without its bitmap");
    with_cockpit([&](Json& c) { object(at(at(c, "landscape_camera_rows"), 1)).erase("arctic"); });
    refused(load, "no \"arctic\"", "a landscape missing");
    with_cockpit([&](Json& c) { at(at(at(c, "landscape_camera_rows"), 0), "desert") = Json{1.0}; });
    refused(load, "expected an object", "a camera row that isn't one");
    with_cockpit([&](Json& c) { array(at(at(at(c, "hud_colour_schemes"), 0), "range_buckets")).pop_back(); });
    refused(load, "\"range_buckets\" is not a list of 5", "a short colour list");
    with_cockpit([&](Json& c) { array(at(at(at(c, "subsystem_lamp_colours"), "engine"), 1)).pop_back(); });
    refused(load, "not 4 numbers", "a short lamp row");
    with_orders([&](Json& o) { array(at(at(o, "navigation_deltas"), "row")).pop_back(); });
    refused(load, "\"row\" has 10 entries, not 11", "a navigation delta missing");
    with_orders([&](Json& o) { at(at(at(at(o, "condition_colours"), "system"), 0), 0) = Json{-1.0}; });
    refused(load, "system is not a whole number from 0 to 65535", "a negative colour");
    const auto with_options = [&](const std::function<void(Json&)>& change) { with_file("options.json", change); };
    const auto options_record = [](Json& o, std::size_t index) -> Json& { return at(at(at(at(o, "menus"), "options"), "records"), index); };
    with_options([&](Json& o) { at(options_record(o, 0), "action") = Json{std::string("continue.nowhere")}; });
    refused(load, "\"action\" is not one of the campaign's actions", "an unknown action");
    with_options([&](Json& o) { at(options_record(o, 0), "down") = Json{7.0}; });
    refused(load, "the menu \"options\"'s link down 7 leads out of the menu", "a link one past the menu");
    with_options([&](Json& o) { at(options_record(o, 0), "up") = Json{200.0}; });
    refused(load, "link up 200 leads out of the menu", "a link far out of the menu");
    with_options([&](Json& o) { at(options_record(o, 0), "caption") = Json{std::string("teleport")}; });
    refused(load, "caption is not one of its names", "a record naming no caption");
    with_options([&](Json& o) { at(options_record(o, 0), "enabled") = Json{1.0}; });
    refused(load, "enabled is not true or false", "a number for a flag");
    with_options([&](Json& o) { at(options_record(o, 0), "text_record") = Json{false}; });
    refused(load, "unknown key \"caption\"", "a picture record naming a caption");
    with_options([&](Json& o) { at(at(at(at(o, "panels"), "news"), "lines"), 1) = Json{std::string("nothing")}; });
    refused(load, "a line is not one of its names", "a panel line naming no caption");
    with_options([&](Json& o) { array(at(at(at(o, "panels"), "news"), "lines")).pop_back(); });
    refused(load, "\"lines\" has 2 entries, not 3", "a panel with a line missing");
    with_options([&](Json& o) { object(at(o, "captions")).erase("quit"); });
    refused(load, "no \"quit\"", "a caption missing");
    with_options([&](Json& o) { array(at(at(at(o, "menus"), "news"), "records")).pop_back(); });
    refused(load, "\"records\" has 2 entries, not 3", "a menu with a record missing");
    with_file("events.json", [&](Json& e) { object(e)["captions"] = Json{Json::Object{}}; });
    refused(load, "unknown key \"captions\"", "captions on a screen that has none");
    with_file("events.json", [&](Json& e) { at(at(at(at(at(e, "menus"), "two_choices"), "records"), 0), "left") = Json{2.0}; });
    refused(load, "the menu \"two_choices\"'s link left 2 leads out of the menu", "an event link out of its menu");
    const auto with_crew = [&](const std::function<void(Json&)>& change) { with_file("crew.json", change); };
    with_crew([&](Json& c) { at(at(at(at(c, "panels"), "company"), "lines"), 0) = Json{std::string("assignment_1")}; });
    refused(load, "a line names a colour caption", "a panel line naming a colour caption");
    with_crew([&](Json& c) { at(at(at(c, "tables"), "pilot_assignment_captions"), 0) = Json{std::string("bar_title")}; });
    refused(load, "names a caption with a head, not a colour caption", "a pointer table naming a head caption");
    with_crew([&](Json& c) { object(at(at(c, "captions"), "assignment_1"))["at"] = Json{Json::Array{Json{1.0}, Json{2.0}}}; });
    refused(load, "unknown key \"at\"", "a colour caption with a place");
    with_crew([&](Json& c) { array(at(at(c, "tables"), "pilot_card_column_origins")).pop_back(); });
    refused(load, "the table \"pilot_card_column_origins\" is not a list of 4", "a short word table");
    with_crew([&](Json& c) { at(at(at(at(at(c, "menus"), "crew"), "records"), 1), "caption") = Json{std::string("crew_leave")}; });
    load();  // the layout accepts a caption on a pilot row that code fills in
    with_crew([&](Json& c) { at(at(at(at(at(c, "menus"), "company"), "records"), 0), "caption") = Json{}; });
    load();  // and a text record with none (null: code fills it in)
    with_file("hub.json", [&](Json& h) { object(at(at(at(h, "tables"), "notification_boxes"), 2))["width"] = Json{1.0}; });
    refused(load, "unknown key \"width\"", "an unknown field in a table's row");
    with_file("hub.json", [&](Json& h) { object(at(at(at(h, "tables"), "notification_boxes"), 2)).erase("rows"); });
    refused(load, "no \"rows\"", "a table's row missing a field");
    with_file("hub.json", [&](Json& h) { at(at(at(at(at(h, "menus"), "planet"), "records"), 5), "down") = Json{6.0}; });
    refused(load, "the menu \"planet\"'s link down 6 leads out of the menu", "a hub link out of its menu");
    with_file("contract_office.json", [&](Json& c) { object(at(c, "tables")).erase("requested_terms"); });
    refused(load, "no \"requested_terms\"", "a table missing");
    const auto with_complex = [&](const std::function<void(Json&)>& change) { with_file("complex.json", change); };
    const auto repair_record = [](Json& c, std::size_t index) -> Json& { return at(at(at(at(c, "menus"), "repair"), "records"), index); };
    with_complex([&](Json& c) { at(at(at(c, "panels"), "owned"), "lines_shown") = Json{15.0}; });
    refused(load, "the panel \"owned\" shows 15 lines of 14", "a list panel showing more lines than it has");
    with_complex([&](Json& c) { object(at(at(c, "panels"), "repair")).erase("lines_shown"); });
    refused(load, "no \"lines_shown\"", "a list panel without its shown count");
    with_complex([&](Json& c) { object(at(at(c, "panels"), "sale"))["lines_shown"] = Json{1.0}; });
    refused(load, "unknown key \"lines_shown\"", "a shown count on a panel that isn't a list");
    with_complex([&](Json& c) { at(repair_record(c, 20), "down") = Json{22.0}; });
    load();  // the repair menu may link one past its last record, as the exe's does
    with_complex([&](Json& c) { at(repair_record(c, 21), "down") = Json{23.0}; });
    refused(load, "the menu \"repair\"'s link down 23 leads out of the menu", "a link two past the repair menu");
    with_complex([&](Json& c) { object(at(at(c, "tables"), "detail_picture_x_offsets"))["elemental"] = Json{1.0}; });
    refused(load, "unknown key \"elemental\"", "an unknown unit kind");
    with_complex([&](Json& c) { at(at(at(c, "tables"), "body_diagram_by_kind"), "wasp") = Json{8.0}; });
    refused(load, "the table \"body_diagram_by_kind\"'s \"wasp\" names no diagram", "a kind naming a diagram past the last");
    with_complex([&](Json& c) { array(at(at(at(at(c, "tables"), "body_diagrams"), 0), "regions")).pop_back(); });
    refused(load, "\"regions\" has 8 entries, not 9", "a body diagram with a region missing");
    with_complex([&](Json& c) { at(at(at(at(at(c, "tables"), "body_diagrams"), 3), "background"), 1) = Json{-4.0}; });
    refused(load, "background is not a whole number from 0 to 65535", "a negative diagram coordinate");
    with_file("dealer.json", [&](Json& d) { at(at(at(d, "tables"), "compatibility_panels"), 2) = Json{std::string("market")}; });
    load();  // the layout takes any of the screen's panels here; the importer, only the compatibility panels
    with_file("dealer.json", [&](Json& d) { at(at(at(d, "tables"), "compatibility_panels"), 2) = Json{std::string("shelf")}; });
    refused(load, "the table \"compatibility_panels\"'s entry is not one of its names", "a pointer table naming no panel");
    restore();
    fs::remove(work / "ui" / "orders.json");
    refused(load, "can't read", "a missing file");
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::printf("usage: ui_test <data dir>\n");
        return 2;
    }
    const fs::path work = fs::current_path() / "ui_test_work";
    fs::remove_all(work);
    try {
        the_layout_rebuilds_the_exes_tables(argv[1]);
        what_doesnt_fit_is_refused(argv[1], work);
    } catch (const std::exception& e) {
        check(false, std::string("unexpected: ") + e.what());
    }
    fs::remove_all(work);
    std::printf("%s\n", g_failures ? "FAILED" : "ok");
    return g_failures ? 1 : 0;
}
