#include "Ui.h"

#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "../Rules/Rules.h"
#include "../Support/StrictReader.h"
#include "UiTable.h"

namespace mw1o::ui {

namespace fs = std::filesystem;
using support::Json;
using Reader = support::StrictReader<UiError>;

namespace {

constexpr std::size_t kTemplates = 8, kCockpitLayouts = 3, kColourSchemes = 2;
constexpr std::array<std::string_view, 3> kLandscapes = {"desert", "tropical", "arctic"};
const auto& kArmour = rules::kArmourLocations;

// A record's members, in the exe's order. A count of 0 is one value,
// otherwise a list of that many.
enum class Kind { U8, U16, S16, Point, Rect, Placement };
struct Field {
    std::string_view name;
    Kind kind;
    std::uint8_t count;
};

void put_word(std::string& out, std::uint16_t w) {
    out += static_cast<char>(w & 0xFF);
    out += static_cast<char>(w >> 8);
}

void put_signed(const Reader& r, std::string& out, const Json& v, std::string_view what) {
    put_word(out, static_cast<std::uint16_t>(r.signed_word(v, what)));
}

// `count` signed words: a point (2) or a rectangle (4).
void put_signed_list(const Reader& r, std::string& out, const Json& v, std::size_t count, std::string_view what) {
    if (!v.is_array() || v.array().size() != count)
        r.fail(std::string(what) + " is not a list of " + std::to_string(count) + " numbers");
    for (const Json& n : v.array()) put_signed(r, out, n, what);
}

void put_value(const Reader& r, std::string& out, const Json& v, Kind kind, std::string_view what) {
    switch (kind) {
    case Kind::U8: out += static_cast<char>(r.byte(v, what)); break;
    case Kind::U16: put_word(out, r.word(v, what)); break;
    case Kind::S16: put_signed(r, out, v, what); break;
    case Kind::Point: put_signed_list(r, out, v, 2, what); break;
    case Kind::Rect: put_signed_list(r, out, v, 4, what); break;
    case Kind::Placement:
        r.only(v, {"bitmap", "at"});
        put_word(out, r.u16(v, "bitmap"));
        put_signed_list(r, out, r.member(v, "at"), 2, what);
        break;
    }
}

// One record of `fields` from `object`, which must hold exactly those
// members; `stride` is its native length.
void put_record(const Reader& r, std::string& out, const Json& object, std::span<const Field> fields, std::size_t stride) {
    std::vector<std::string_view> names;
    for (const Field& f : fields) names.push_back(f.name);
    r.only(object, names);
    const std::size_t start = out.size();
    for (const Field& f : fields) {
        const Json& v = r.member(object, f.name);
        if (f.count == 0) {
            put_value(r, out, v, f.kind, f.name);
            continue;
        }
        if (!v.is_array() || v.array().size() != f.count)
            r.fail("\"" + std::string(f.name) + "\" is not a list of " + std::to_string(f.count));
        for (const Json& e : v.array()) put_value(r, out, e, f.kind, f.name);
    }
    if (out.size() - start != stride) r.fail("a record is " + std::to_string(out.size() - start) + " bytes, not " + std::to_string(stride));
}

// A list of `count` lists of `inner` words: per colour scheme, say.
void put_word_rows(const Reader& r, std::string& out, const Json& object, std::string_view key, std::size_t count,
                   std::size_t inner, bool is_signed) {
    for (const Json& row : r.array(object, key, count)) {
        if (!row.is_array() || row.array().size() != inner)
            r.fail("\"" + std::string(key) + "\" has a row that is not " + std::to_string(inner) + " numbers");
        for (const Json& v : row.array()) {
            if (is_signed) put_signed(r, out, v, key);
            else put_word(out, r.word(v, key));
        }
    }
}

// BtechCockpitLayout (0x6A bytes).
constexpr Field kLayoutFields[] = {
    {"viewport", Kind::Rect, 0},          {"radar_panel", Kind::Rect, 0},
    {"target_panel", Kind::Rect, 0},      {"weapon_list", Kind::Point, 0},
    {"heat_gauge", Kind::Point, 0},       {"movement_gauge", Kind::Point, 0},
    {"jump_charge_gauge", Kind::Point, 0}, {"firing_indicator_bank_0", Kind::Point, 3},
    {"firing_indicator_bank_1", Kind::Point, 3}, {"unknown_40", Kind::U8, 4},
    {"heading_strip_y", Kind::S16, 0},    {"bitmap_placements", Kind::Placement, 6},
};
// BtechLandscapeCameraColourRow (0x1E bytes).
constexpr Field kCameraRowFields[] = {
    {"player_fill_palette_a", Kind::U8, 0}, {"unknown_01", Kind::U8, 0},
    {"shared_fill_palette_b", Kind::U8, 0}, {"unknown_03", Kind::U8, 0},
    {"unknown_04", Kind::U8, 2},            {"map_boundary", Kind::U16, 0},
    {"map_retreat_boundary", Kind::U16, 0}, {"unit_markers", Kind::U16, 5},
    {"order_tracking_line", Kind::U16, 0},  {"hud_reticle", Kind::U8, 0},
    {"unknown_17", Kind::U8, 0},            {"heading_tick_palette_index", Kind::U16, 0},
    {"unknown_1a", Kind::U8, 4},
};
// BtechHudColourScheme (0x64 bytes, all words).
constexpr Field kHudColourFields[] = {
    {"inspection_background", Kind::U16, 0}, {"inspection_subsystem_text", Kind::U16, 0},
    {"inspection_equipment_text", Kind::U16, 0}, {"inspection_equipment_heading", Kind::U16, 0},
    {"inspection_footer", Kind::U16, 0},     {"inspection_mech_name", Kind::U16, 0},
    {"order_highlight", Kind::U16, 0},       {"order_reference_highlight", Kind::U16, 0},
    {"order_conditions", Kind::U16, 5},      {"radar_player", Kind::U16, 0},
    {"radar_friendly", Kind::U16, 0},        {"radar_opposing", Kind::U16, 0},
    {"radar_special_target", Kind::U16, 0},  {"radar_background", Kind::U16, 0},
    {"radar_terrain_bands", Kind::U16, 4},   {"weapon_ready", Kind::U16, 0},
    {"weapon_unavailable", Kind::U16, 0},    {"weapon_reloading", Kind::U16, 0},
    {"ammo_text", Kind::U16, 0},             {"selected_mount", Kind::U16, 0},
    {"unselected_mount", Kind::U16, 0},      {"forward_ticks", Kind::U16, 0},
    {"reverse_ticks", Kind::U16, 0},         {"movement_zero_reference", Kind::U16, 0},
    {"selected_movement_tick", Kind::U16, 0}, {"jump_rising", Kind::U16, 0},
    {"jump_falling", Kind::U16, 0},          {"jump_charged", Kind::U16, 0},
    {"jump_below_threshold", Kind::U16, 0},  {"jump_reference", Kind::U16, 0},
    {"heat_rising", Kind::U16, 0},           {"heat_falling", Kind::U16, 0},
    {"range_buckets", Kind::U16, 5},         {"unknown_58", Kind::U16, 2},
    {"undamaged", Kind::U16, 0},             {"above_half_condition", Kind::U16, 0},
    {"damaged", Kind::U16, 0},               {"destroyed", Kind::U16, 0},
};

// A diagram: its point (`point_key`) and one rectangle per armour location.
void put_diagram(const Reader& r, std::string& points, std::string& regions, const Json& diagram, std::string_view point_key) {
    r.only(diagram, {point_key, "regions"});
    put_signed_list(r, points, r.member(diagram, point_key), 2, point_key);
    const auto rects = r.keyed(diagram, "regions", kArmour);
    for (std::size_t armour = 0; armour < kArmour.size(); ++armour)
        put_signed_list(r, regions, *rects[armour], 4, kArmour[armour]);
}

void read_cockpit(const fs::path& data_dir, support::NativeBytes& out) {
    Reader r(data_dir / "ui" / "cockpit.json");
    const Json file = r.parse();
    r.only(file, {"templates", "layouts", "landscape_camera_rows", "hud_colour_schemes", "subsystem_lamp_colours"});
    std::string origins, inspection, anchors, target, layout_indices;
    for (const Json& t : r.array(file, "templates", kTemplates)) {
        r.only(t, {"cockpit_layout", "inspection_diagram", "target_diagram"});
        const std::uint16_t layout = r.u16(t, "cockpit_layout");
        if (layout >= kCockpitLayouts) r.fail("a template's \"cockpit_layout\" " + std::to_string(layout) + " names no layout");
        put_word(layout_indices, layout);
        put_diagram(r, origins, inspection, r.member(t, "inspection_diagram"), "origin");
        put_diagram(r, anchors, target, r.member(t, "target_diagram"), "anchor");
    }
    std::string layouts;
    for (const Json& l : r.array(file, "layouts", kCockpitLayouts)) put_record(r, layouts, l, kLayoutFields, 0x6A);
    std::string camera;
    for (const Json& scheme : r.array(file, "landscape_camera_rows", kColourSchemes)) {
        if (!scheme.is_object()) r.fail("a scheme of \"landscape_camera_rows\" is not an object");
        r.only(scheme, kLandscapes);
        for (std::string_view landscape : kLandscapes) put_record(r, camera, r.member(scheme, landscape), kCameraRowFields, 0x1E);
    }
    std::string schemes;
    for (const Json& s : r.array(file, "hud_colour_schemes", kColourSchemes)) put_record(r, schemes, s, kHudColourFields, 0x64);
    const Json& lamps = r.member(file, "subsystem_lamp_colours");
    r.only(lamps, {"subsystem", "engine"});
    std::string lamp_words;
    put_word_rows(r, lamp_words, lamps, "subsystem", kColourSchemes, 3, false);
    put_word_rows(r, lamp_words, lamps, "engine", kColourSchemes, 4, false);
    out["cockpit.inspection_diagram_origins"] = origins;
    out["cockpit.inspection_diagram_regions"] = inspection;
    out["cockpit.target_diagram_anchors"] = anchors;
    out["cockpit.target_diagram_regions"] = target;
    out["cockpit.layout_by_template"] = layout_indices;
    out["cockpit.layouts"] = layouts;
    out["cockpit.landscape_camera_rows"] = camera;
    out["cockpit.hud_colour_schemes"] = schemes;
    out["cockpit.subsystem_lamp_colours"] = lamp_words;
}

void read_orders(const fs::path& data_dir, support::NativeBytes& out) {
    Reader r(data_dir / "ui" / "orders.json");
    const Json file = r.parse();
    r.only(file, {"selection_rows", "navigation_deltas", "condition_colours"});
    const auto signed_list = [&](const Json& object, std::string_view key, std::size_t count) {
        std::string bytes;
        for (const Json& v : r.array(object, key, count)) put_signed(r, bytes, v, key);
        return bytes;
    };
    const Json& rows = r.member(file, "selection_rows");
    r.only(rows, {"first", "last"});
    out["orders.selection_first_rows"] = signed_list(rows, "first", 2);
    out["orders.selection_last_rows"] = signed_list(rows, "last", 2);
    const Json& deltas = r.member(file, "navigation_deltas");
    r.only(deltas, {"panel", "row"});
    out["orders.navigation_panel_deltas"] = signed_list(deltas, "panel", 11);
    out["orders.navigation_row_deltas"] = signed_list(deltas, "row", 11);
    const Json& colours = r.member(file, "condition_colours");
    r.only(colours, {"engine", "system"});
    put_word_rows(r, out["orders.engine_condition_colours"], colours, "engine", kColourSchemes, 4, false);
    put_word_rows(r, out["orders.system_condition_colours"], colours, "system", kColourSchemes, 4, false);
}

}  // namespace

support::NativeBytes load_layout(const fs::path& data_dir) {
    support::NativeBytes out;
    read_cockpit(data_dir, out);
    read_orders(data_dir, out);
    for (const UiPiece& piece : ui_table()) {
        const auto it = out.find(piece.id);
        if (it == out.end()) throw UiError("data/ui builds no \"" + std::string(piece.id) + "\"");
        if (it->second.size() != piece.length)
            throw UiError("data/ui's \"" + std::string(piece.id) + "\" is " + std::to_string(it->second.size()) +
                          " bytes, not " + std::to_string(piece.length));
    }
    return out;
}

}  // namespace mw1o::ui
