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
