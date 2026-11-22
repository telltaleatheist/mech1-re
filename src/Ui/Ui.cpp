#include "Ui.h"

#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "../Import/ImportTable.h"
#include "../Rules/Rules.h"
#include "../Support/StrictReader.h"
#include "UiActions.h"
#include "UiTable.h"

namespace mw1o::ui {

namespace fs = std::filesystem;
namespace import = mw1o::import;
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

// The data-segment offset of word `prefix` + `name` (src/Import/word_pieces.txt),
// as the exe's pointer tables hold it; a name that is no word is refused.
std::uint16_t word_offset(const Reader& r, std::string_view prefix, const Json& name) {
    if (!name.is_string() || name.string().empty()) r.fail("a word reference is not a word's name");
    const std::string id = std::string(prefix) + name.string();
    for (const auto& piece : import::import_table())
        if (import::is_word(piece.kind) && piece.id == id) return piece.offset;
    r.fail("\"" + id + "\" names no word");
}

// A list of word names as their offsets, `count` long.
std::string word_refs(const Reader& r, const Json& object, std::string_view key, std::string_view prefix, std::size_t count) {
    std::string out;
    for (const Json& name : r.array(object, key, count)) put_word(out, word_offset(r, prefix, name));
    return out;
}

// A list of indices into a list of `limit` labels, `count` long.
std::string label_indices(const Reader& r, const Json& object, std::string_view key, std::size_t count, std::size_t limit) {
    std::string out;
    for (const Json& v : r.array(object, key, count)) {
        const std::uint16_t index = r.word(v, key);
        if (index >= limit) r.fail("\"" + std::string(key) + "\" names label " + std::to_string(index) + " of " + std::to_string(limit));
        put_word(out, index);
    }
    return out;
}

void read_cockpit(const fs::path& data_dir, support::NativeBytes& out) {
    Reader r(data_dir / "ui" / "cockpit.json");
    const Json file = r.parse();
    r.only(file, {"templates", "layouts", "landscape_camera_rows", "hud_colour_schemes", "subsystem_lamp_colours",
                  "word_refs", "condition_labels"});
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
    const Json& refs = r.member(file, "word_refs");
    r.only(refs, {"legends", "weapon_labels", "radar_ranges", "range_classes", "energy_ammo"});
    constexpr std::string_view words = "cockpit.word.";
    out["cockpit.word_refs.legends"] = word_refs(r, refs, "legends", words, 2);
    out["cockpit.word_refs.weapon_labels"] = word_refs(r, refs, "weapon_labels", words, 10);
    out["cockpit.word_refs.radar_ranges"] = word_refs(r, refs, "radar_ranges", words, 4);
    out["cockpit.word_refs.range_classes"] = word_refs(r, refs, "range_classes", words, 4);
    put_word(out["cockpit.word_refs.energy_ammo"], word_offset(r, words, r.member(refs, "energy_ammo")));
    // The inspection's condition label (one of its 6) for each hit count.
    const Json& conditions = r.member(file, "condition_labels");
    r.only(conditions, {"two_hit", "engine", "actuator"});
    out["cockpit.condition_labels.two_hit"] = label_indices(r, conditions, "two_hit", 3, 6);
    out["cockpit.condition_labels.engine"] = label_indices(r, conditions, "engine", 4, 6);
    out["cockpit.condition_labels.actuator"] = label_indices(r, conditions, "actuator", 5, 6);
}

void read_orders(const fs::path& data_dir, support::NativeBytes& out) {
    Reader r(data_dir / "ui" / "orders.json");
    const Json file = r.parse();
    r.only(file, {"selection_rows", "navigation_deltas", "condition_colours", "start", "word_refs", "display_indices"});
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
    // Play writes the panels' x (one moves in reduced memory mode), but a
    // fresh EXEC reads them first, so they start from here.
    const Json& start = r.member(file, "start");
    r.only(start, {"lance_panel_text_x"});
    std::string& text_x = out["orders.start.lance_panel_text_x"];
    for (const Json& v : r.array(start, "lance_panel_text_x", 3)) put_word(text_x, r.word(v, "lance_panel_text_x"));
    const Json& refs = r.member(file, "word_refs");
    r.only(refs, {"orders", "tactical", "damage", "weapons"});
    constexpr std::string_view words = "orders.word.";
    out["orders.word_refs.orders"] = word_refs(r, refs, "orders", words, 7);
    out["orders.word_refs.tactical"] = word_refs(r, refs, "tactical", words, 9);
    out["orders.word_refs.damage"] = word_refs(r, refs, "damage", words, 7);
    out["orders.word_refs.weapons"] = word_refs(r, refs, "weapons", words, 10);
    // The order label (one of the 7) each order code shows.
    out["orders.display_indices"] = label_indices(r, file, "display_indices", 11, 7);
}

// MW_MAIN's screens: captions, panels, menus and tables (docs/phase2/PLAN.md
// 6.2). A screen file names its own captions; its panels, menus and tables
// refer to them by those names. Piece ids are "<file>.caption.<name>",
// "<file>.panel.<name>", "<file>.menu.<name>" and "<file>.table.<name>". A
// caption's piece is its 5-byte head (x, y, colour), or for a colour caption
// the one colour byte; its text follows in the exe until slice 5.
enum class MenuCaptions { Screen, Story };
struct PanelSpec {
    std::string_view name;
    std::size_t lines;
    // A list panel's header counts the lines it starts out showing
    // ("lines_shown"); code changes it as rows fill. Otherwise the header
    // counts every line.
    bool list = false;
};
struct MenuSpec {
    std::string_view name;
    std::size_t records;
    MenuCaptions captions;
    // The exe's structure has one link one past the last record, which the
    // original's past-end probe answers (PLAN 6.1); otherwise refused.
    bool past_end_link;
};
// A table is one of:
//   Words        a list of words;
//   KeyedWords   words keyed by `names` (unit kinds, say), in that order;
//   Rows         rows of the named words in `names`, which the exe keeps one
//                field after another (all the first fields, then the second);
//   CaptionRefs  a pointer table naming colour captions;
//   PanelRefs    a pointer table naming panels, those in `names` only;
//   Diagrams     body diagrams: a background rectangle and kBodyRegions
//                region rectangles each, [x, y, width, height];
//   DiagramRefs  keyed by `names`: the index of a diagram in the table
//                `target`, or null for none (the exe's pointer, or 0);
//   WordRefs     a pointer table naming the screen's words (slice 5), each a
//                name after "<screen>.word." in src/Import/word_pieces.txt.
enum class TableKind { Words, KeyedWords, Rows, CaptionRefs, PanelRefs, Diagrams, DiagramRefs, WordRefs };
struct TableSpec {
    std::string_view name;
    TableKind kind;
    std::size_t count;
    std::span<const std::string_view> names = {};
    std::string_view target = {};
};
constexpr std::size_t kBodyRegions = 9, kDiagramBytes = (1 + kBodyRegions) * 8;
struct ScreenSpec {
    std::string_view file;  // "options": data/ui/options.json, pieces "options.*"
    std::span<const std::string_view> captions;
    std::span<const std::string_view> colour_captions;
    std::span<const PanelSpec> panels;
    std::span<const MenuSpec> menus;
    std::span<const TableSpec> tables;
};

constexpr std::uint8_t kNoLink = 0xFF;

std::string piece_id(std::string_view file, std::string_view kind, std::string_view name) {
    return std::string(file) + "." + std::string(kind) + "." + std::string(name);
}

// A screen's caption names, heads first, then colour captions.
struct Captions {
    const ScreenSpec& screen;
    std::vector<std::string_view> names;
    explicit Captions(const ScreenSpec& s) : screen(s) {
        names.assign(s.captions.begin(), s.captions.end());
        names.insert(names.end(), s.colour_captions.begin(), s.colour_captions.end());
    }
    bool colour_only(std::size_t index) const { return index >= screen.captions.size(); }
    // The exe offset of the named caption's piece.
    std::uint16_t offset(const Reader& r, std::size_t index) const {
        const UiPiece* piece = find_ui_piece(piece_id(screen.file, "caption", names[index]));
        if (!piece) r.fail("the caption \"" + std::string(names[index]) + "\" has no piece in the layout table");
        return piece->offset;
    }
    // A panel line points at a caption's head.
    std::uint16_t head(const Reader& r, const Json& v, std::string_view what) const {
        const std::size_t index = r.name(v, names, what);
        if (colour_only(index)) r.fail(std::string(what) + " names a colour caption, which has no head");
        return offset(r, index);
    }
    // A text record or a pointer table points at a caption's colour byte.
    std::uint16_t colour_byte(const Reader& r, const Json& v, std::string_view what) const {
        const std::size_t index = r.name(v, names, what);
        return static_cast<std::uint16_t>(offset(r, index) + (colour_only(index) ? 0 : 4));
    }
    // A pointer table names colour captions only.
    std::uint16_t colour_caption(const Reader& r, const Json& v, std::string_view what) const {
        const std::size_t index = r.name(v, names, what);
        if (!colour_only(index)) r.fail(std::string(what) + " names a caption with a head, not a colour caption");
        return offset(r, index);
    }
};

void put_unsigned_point(const Reader& r, std::string& out, const Json& v, std::string_view what) {
    if (!v.is_array() || v.array().size() != 2) r.fail(std::string(what) + " is not a list of 2 numbers");
    for (const Json& n : v.array()) put_word(out, r.word(n, what));
}

std::uint16_t action_value(const Reader& r, const Json& v) {
    if (Reader::is_null(v)) return 0;
    if (v.is_string())
        for (const UiAction& action : ui_actions())
            if (action.name == v.string()) return action.value;
    r.fail("\"action\" is not one of the campaign's actions");
}

std::uint8_t link(const Reader& r, const Json& v, const MenuSpec& menu, std::string_view what) {
    if (Reader::is_null(v)) return kNoLink;
    const std::size_t limit = menu.records + (menu.past_end_link ? 1 : 0);
    const double index = r.whole(v, what, 254);
    if (index >= double(limit))
        r.fail("the menu \"" + std::string(menu.name) + "\"'s link " + std::string(what) + " " +
               std::to_string(static_cast<long>(index)) + " leads out of the menu");
    return static_cast<std::uint8_t>(index);
}

// A text record's caption is null where code fills it in (the crew menu's
// pilot rows).
void put_menu_record(const Reader& r, std::string& out, const Json& record, const Captions& captions, const MenuSpec& menu) {
    const bool story = menu.captions == MenuCaptions::Story;
    if (!record.is_object()) r.fail("a record of the menu \"" + std::string(menu.name) + "\" is not an object");
    const bool text_record = r.flag(r.member(record, "text_record"), "text_record");
    const std::string_view reference = story ? "story_caption" : text_record ? "caption" : "picture";
    r.only(record, {"enabled", "text_record", "at", "action", reference, "up", "down", "left", "right"});
    out += static_cast<char>(r.flag(r.member(record, "enabled"), "enabled") ? 1 : 0);
    out += static_cast<char>(text_record ? 1 : 0);
    put_unsigned_point(r, out, r.member(record, "at"), "at");
    put_word(out, action_value(r, r.member(record, "action")));
    const Json& ref = r.member(record, reference);
    if (story) put_word(out, Reader::is_null(ref) ? 0 : r.word(ref, reference));  // the story caption's original address
    else if (text_record) put_word(out, Reader::is_null(ref) ? 0 : captions.colour_byte(r, ref, "caption"));
    else put_word(out, r.word(ref, reference));
    for (std::string_view direction : {"up", "down", "left", "right"})
        out += static_cast<char>(link(r, r.member(record, direction), menu, direction));
}

void put_rectangle(const Reader& r, std::string& out, const Json& v, std::string_view what) {
    if (!v.is_array() || v.array().size() != 4) r.fail(std::string(what) + " is not a list of 4 numbers");
    for (const Json& n : v.array()) put_word(out, r.word(n, what));
}

// The exe offset of a screen's piece of `kind` named `name`.
std::uint16_t piece_offset(const Reader& r, std::string_view file, std::string_view kind, std::string_view name) {
    const UiPiece* piece = find_ui_piece(piece_id(file, kind, name));
    if (!piece) r.fail("the " + std::string(kind) + " \"" + std::string(name) + "\" has no piece in the layout table");
    return piece->offset;
}

void put_table(const Reader& r, std::string& out, const Json& v, const TableSpec& table, const ScreenSpec& screen,
               const Captions& captions) {
    const std::string what = "the table \"" + std::string(table.name) + "\"";
    if (table.kind == TableKind::KeyedWords || table.kind == TableKind::DiagramRefs) {
        if (!v.is_object()) r.fail(what + " is not an object");
        r.only(v, table.names);
        for (std::string_view key : table.names) {
            const Json& entry = r.member(v, key);
            if (table.kind == TableKind::KeyedWords) {
                put_word(out, r.word(entry, key));
                continue;
            }
            if (Reader::is_null(entry)) {
                put_word(out, 0);
                continue;
            }
            const TableSpec* target = nullptr;
            for (const TableSpec& t : screen.tables)
                if (t.name == table.target) target = &t;
            const double index = r.whole(entry, key, 65535);
            if (!target || index >= double(target->count))
                r.fail(what + "'s \"" + std::string(key) + "\" names no diagram");
            put_word(out, static_cast<std::uint16_t>(piece_offset(r, screen.file, "table", table.target) + index * kDiagramBytes));
        }
        return;
    }
    if (!v.is_array() || v.array().size() != table.count)
        r.fail(what + " is not a list of " + std::to_string(table.count));
    const Json::Array& entries = v.array();
    switch (table.kind) {
    case TableKind::KeyedWords:
    case TableKind::DiagramRefs:
        break;
    case TableKind::PanelRefs:
        for (const Json& n : entries)
            put_word(out, piece_offset(r, screen.file, "panel", table.names[r.name(n, table.names, what + "'s entry")]));
        break;
    case TableKind::Diagrams:
        for (const Json& diagram : entries) {
            r.only(diagram, {"background", "regions"});
            put_rectangle(r, out, r.member(diagram, "background"), "background");
            for (const Json& region : r.array(diagram, "regions", kBodyRegions)) put_rectangle(r, out, region, "a region");
        }
        break;
    case TableKind::Words:
        for (const Json& n : entries) put_word(out, r.word(n, what));
        break;
    case TableKind::CaptionRefs:
        for (const Json& n : entries) put_word(out, captions.colour_caption(r, n, what + "'s entry"));
        break;
    case TableKind::WordRefs:
        for (const Json& n : entries) put_word(out, word_offset(r, std::string(screen.file) + ".word.", n));
        break;
    case TableKind::Rows:
        for (const Json& row : entries) r.only(row, table.names);
        for (std::string_view field : table.names)
            for (const Json& row : entries) put_word(out, r.u16(row, field));
        break;
    }
}

void read_screen(const fs::path& data_dir, const ScreenSpec& screen, support::NativeBytes& out) {
    Reader r(data_dir / "ui" / (std::string(screen.file) + ".json"));
    const Json file = r.parse();
    const Captions captions(screen);
    // A screen without captions, panels or tables leaves out those members.
    std::vector<std::string_view> members;
    if (!captions.names.empty()) members.push_back("captions");
    if (!screen.panels.empty()) members.push_back("panels");
    members.push_back("menus");
    if (!screen.tables.empty()) members.push_back("tables");
    r.only(file, members);
    const auto caption_values = captions.names.empty() ? std::vector<const Json*>{} : r.keyed(file, "captions", captions.names);
    for (std::size_t i = 0; i < captions.names.size(); ++i) {
        const Json& c = *caption_values[i];
        std::string head;
        if (captions.colour_only(i)) {
            r.only(c, {"colour"});
        } else {
            r.only(c, {"at", "colour"});
            put_unsigned_point(r, head, r.member(c, "at"), "at");
        }
        head += static_cast<char>(r.u8(c, "colour"));
        out[piece_id(screen.file, "caption", captions.names[i])] = head;
    }
    std::vector<std::string_view> panel_names, menu_names, table_names;
    for (const PanelSpec& p : screen.panels) panel_names.push_back(p.name);
    for (const MenuSpec& m : screen.menus) menu_names.push_back(m.name);
    for (const TableSpec& t : screen.tables) table_names.push_back(t.name);
    const auto panels = screen.panels.empty() ? std::vector<const Json*>{} : r.keyed(file, "panels", panel_names);
    for (std::size_t i = 0; i < screen.panels.size(); ++i) {
        const PanelSpec& spec = screen.panels[i];
        const Json& p = *panels[i];
        if (spec.list) r.only(p, {"at", "width", "bottom_inset", "border", "line_spacing", "background", "lines_shown", "lines"});
        else r.only(p, {"at", "width", "bottom_inset", "border", "line_spacing", "background", "lines"});
        std::string bytes;
        put_unsigned_point(r, bytes, r.member(p, "at"), "at");
        put_word(bytes, r.u16(p, "width"));
        put_word(bytes, r.u16(p, "bottom_inset"));
        const auto& lines = r.array(p, "lines", spec.lines);
        const std::uint16_t shown = spec.list ? r.u16(p, "lines_shown") : static_cast<std::uint16_t>(lines.size());
        if (shown > lines.size())
            r.fail("the panel \"" + std::string(spec.name) + "\" shows " + std::to_string(shown) + " lines of " + std::to_string(lines.size()));
        put_word(bytes, shown);
        put_word(bytes, r.u16(p, "border"));
        put_word(bytes, r.u16(p, "line_spacing"));
        put_word(bytes, r.u16(p, "background"));
        for (const Json& line : lines) put_word(bytes, captions.head(r, line, "a line"));
        out[piece_id(screen.file, "panel", spec.name)] = bytes;
    }
    const auto menus = r.keyed(file, "menus", menu_names);
    for (std::size_t i = 0; i < screen.menus.size(); ++i) {
        const MenuSpec& spec = screen.menus[i];
        const Json& m = *menus[i];
        r.only(m, {"records"});
        std::string bytes;
        for (const Json& record : r.array(m, "records", spec.records)) put_menu_record(r, bytes, record, captions, spec);
        out[piece_id(screen.file, "menu", spec.name)] = bytes;
    }
    const auto tables = screen.tables.empty() ? std::vector<const Json*>{} : r.keyed(file, "tables", table_names);
    for (std::size_t i = 0; i < screen.tables.size(); ++i) {
        std::string bytes;
        put_table(r, bytes, *tables[i], screen.tables[i], screen, captions);
        out[piece_id(screen.file, "table", screen.tables[i].name)] = bytes;
    }
}

// options.json: the options, restore and quit/restart confirmation screens,
// and the News Net menu.
constexpr std::string_view kOptionsCaptions[] = {
    "save", "restore", "sound", "detail", "restart", "quit", "continue",
    "confirm_quit", "confirm_restart", "yes", "no",
    "news_previous", "news_next", "news_done",
    "restore_heading", "save_1", "save_2", "save_3", "save_4", "save_5", "save_6",
    "save_7", "save_8", "save_9", "save_10", "save_11", "save_12", "restore_cancel"};
constexpr PanelSpec kOptionsPanels[] = {{"options", 7}, {"restore", 14}, {"confirmation", 3}, {"news", 3}};
constexpr MenuSpec kOptionsMenus[] = {{"options", 7, MenuCaptions::Screen, false},
                                      {"restore", 13, MenuCaptions::Screen, false},
                                      {"confirmation", 2, MenuCaptions::Screen, false},
                                      {"news", 3, MenuCaptions::Screen, false}};
// events.json: the event choice menus, whose captions are the story's
// (story/events.json), named by the original address that file keeps.
constexpr MenuSpec kEventMenus[] = {{"two_choices", 2, MenuCaptions::Story, false},
                                    {"three_choices", 3, MenuCaptions::Story, false}};
// hub.json: the three hub menus (picture records), the calendar's month name
// widths and the four notification boxes.
constexpr MenuSpec kHubMenus[] = {{"planet", 6, MenuCaptions::Screen, false},
                                  {"contract", 3, MenuCaptions::Screen, false},
                                  {"remote_planet", 6, MenuCaptions::Screen, false}};
constexpr std::string_view kNotificationBoxFields[] = {"inset_x", "inset_y", "columns", "rows"};
constexpr TableSpec kHubTables[] = {{"calendar_month_name_widths", TableKind::Words, 12, {}},
                                    {"notification_boxes", TableKind::Rows, 4, kNotificationBoxFields},
                                    {"month_words", TableKind::WordRefs, 12, {}},
                                    {"notification_words", TableKind::WordRefs, 4, {}}};
// crew.json: the company menu, the commander profile, the crew screen, the
// bar, the recruit interview, the pilot card's columns and the pilot
// assignment captions (colour captions, named by the exe's pointer table).
constexpr std::string_view kCrewCaptions[] = {
    "company_title", "company_crew", "company_news", "company_leave",
    "profile_1", "profile_2", "profile_3", "profile_4", "profile_5", "profile_6",
    "profile_7", "profile_8", "profile_9", "profile_10", "profile_11",
    "crew_1", "crew_2", "crew_3", "crew_4", "crew_5", "crew_6", "crew_leave",
    "bar_title", "bar_drink", "bar_recruit", "bar_leave",
    "recruit_prompt", "recruit_hire", "recruit_reject"};
constexpr std::string_view kCrewColourCaptions[] = {
    "assignment_1", "assignment_2", "assignment_3", "assignment_4", "assignment_5", "assignment_6",
    "assignment_7", "assignment_8", "assignment_9", "assignment_10", "assignment_11", "assignment_12"};
constexpr PanelSpec kCrewPanels[] = {{"company", 4}, {"commander_profile", 11}, {"crew", 7}, {"bar", 4}, {"recruit", 3}};
constexpr MenuSpec kCrewMenus[] = {{"company", 3, MenuCaptions::Screen, false},
                                   {"crew", 5, MenuCaptions::Screen, false},
                                   {"bar", 3, MenuCaptions::Screen, false},
                                   {"recruit", 2, MenuCaptions::Screen, false}};
constexpr TableSpec kCrewTables[] = {{"pilot_card_column_origins", TableKind::Words, 4, {}},
                                     {"pilot_assignment_captions", TableKind::CaptionRefs, 12, {}},
                                     {"reputation_words", TableKind::WordRefs, 4, {}},
                                     {"attitude_words", TableKind::WordRefs, 9, {}},
                                     {"skill_words", TableKind::WordRefs, 4, {}}};
// contract_office.json: the office and the offer's actions, the requested
// terms' positions and digit counts, and the contract date's year offsets.
constexpr std::string_view kContractCaptions[] = {
    "request_offer", "leave_office", "negotiate", "next_offer", "back_to_office"};
constexpr PanelSpec kContractPanels[] = {{"office", 2}, {"actions", 3}};
constexpr MenuSpec kContractMenus[] = {{"office", 2, MenuCaptions::Screen, false},
                                       {"actions", 6, MenuCaptions::Screen, false}};
constexpr std::string_view kRequestedTermFields[] = {"x", "y", "digits"};
constexpr TableSpec kContractTables[] = {{"requested_terms", TableKind::Rows, 3, kRequestedTermFields},
                                         {"date_year_x_offsets", TableKind::Words, 12, {}}};
// complex.json: the Mech Complex, the owned unit list, the 'Mech and
// DropShip service screens, the sale and rearm confirmations, component
// repair, the detail pictures and the body diagrams. The owned list and the
// repair panel are list panels.
constexpr std::string_view kComplexCaptions[] = {
    "complex_title", "complex_owned", "complex_ammo", "complex_market", "complex_leave",
    "owned_heading", "owned_1", "owned_2", "owned_3", "owned_4", "owned_5", "owned_6", "owned_7",
    "owned_8", "owned_9", "owned_10", "owned_11", "owned_12", "owned_13",
    "mech_status", "service_repair", "service_repair_all", "service_rearm", "service_sell", "service_leave",
    "dropship_status", "dropship_sell", "dropship_leave",
    "sale_prompt", "sale_yes", "sale_no", "rearm_prompt", "rearm_yes", "rearm_no",
    "repair_title", "repair_system_1", "repair_system_2", "repair_system_3", "repair_system_4", "repair_system_5",
    "repair_system_6", "repair_system_7", "repair_system_8", "repair_system_9", "repair_system_10",
    "repair_body", "repair_footer", "repair_weapon_1", "repair_weapon_2", "repair_weapon_3", "repair_weapon_4",
    "repair_weapon_5", "repair_weapon_6", "repair_weapon_7", "repair_weapon_8", "repair_weapon_9",
    "repair_weapon_10", "repair_weapon_11"};
constexpr PanelSpec kComplexPanels[] = {{"complex", 5}, {"owned", 14, true}, {"mech_status", 6},
                                        {"dropship_status", 3}, {"sale", 3}, {"rearm", 3}, {"repair", 24, true}};
constexpr MenuSpec kComplexMenus[] = {{"complex", 4, MenuCaptions::Screen, false},
                                      {"owned", 13, MenuCaptions::Screen, false},
                                      {"mech_service", 5, MenuCaptions::Screen, false},
                                      {"dropship_service", 2, MenuCaptions::Screen, false},
                                      {"sale", 2, MenuCaptions::Screen, false},
                                      {"rearm", 2, MenuCaptions::Screen, false},
                                      {"repair", 22, MenuCaptions::Screen, true}};
constexpr std::span<const std::string_view> kUnitKinds = rules::kUnitKinds;
constexpr std::span<const std::string_view> kMechKinds = kUnitKinds.first(10);
constexpr TableSpec kComplexTables[] = {
    {"detail_picture_x_offsets", TableKind::KeyedWords, 0, kUnitKinds},
    {"detail_picture_y_origins", TableKind::KeyedWords, 0, kUnitKinds},
    {"animation_frames", TableKind::Words, 23},
    {"repair_title_leading_spaces", TableKind::KeyedWords, 0, kUnitKinds},
    {"body_damage_fill_colours", TableKind::Words, 4},
    {"body_diagram_by_kind", TableKind::DiagramRefs, 0, kMechKinds, "body_diagrams"},
    {"body_diagrams", TableKind::Diagrams, 8},
    {"damage_tier_words", TableKind::WordRefs, 4},
    {"ammo_family_words", TableKind::WordRefs, 6},
    {"repair_weapon_words", TableKind::WordRefs, 10}};
// dealer.json: the market list, the 'Mech and DropShip detail panels, the
// component panel, and reserve ammunition with its six compatibility
// panels, which a pointer table names.
constexpr std::string_view kDealerCaptions[] = {
    "market_heading", "market_1", "market_2", "market_3", "market_4", "market_5", "market_6", "market_7",
    "market_8", "market_9", "market_10", "market_11", "market_12", "market_13",
    "mech_title", "mech_buy", "mech_inspect", "mech_leave",
    "dropship_title", "dropship_buy", "dropship_leave",
    "component_title", "component_system_1", "component_system_2", "component_system_3", "component_system_4",
    "component_system_5", "component_system_6", "component_system_7", "component_system_8",
    "component_system_9", "component_system_10", "component_body", "component_weapon_heading",
    "component_weapon_1", "component_weapon_2", "component_weapon_3", "component_weapon_4", "component_weapon_5",
    "component_weapon_6", "component_weapon_7", "component_weapon_8", "component_weapon_9",
    "component_weapon_10", "component_done",
    "ammo_heading", "ammo_family_1", "ammo_family_2", "ammo_family_3", "ammo_family_4", "ammo_family_5",
    "ammo_family_6", "ammo_quantity_1", "ammo_quantity_2", "ammo_quantity_3", "ammo_quantity_4",
    "ammo_quantity_5", "ammo_quantity_6", "ammo_done",
    "compatibility_1", "compatibility_2", "compatibility_3", "compatibility_4", "compatibility_5",
    "compatibility_6"};
constexpr PanelSpec kDealerPanels[] = {{"market", 14, true}, {"mech", 4}, {"dropship", 3}, {"component", 24, true},
                                       {"ammo", 14}, {"compatibility_1", 1}, {"compatibility_2", 1},
                                       {"compatibility_3", 1}, {"compatibility_4", 1}, {"compatibility_5", 1},
                                       {"compatibility_6", 1}};
constexpr MenuSpec kDealerMenus[] = {{"market", 13, MenuCaptions::Screen, false},
                                     {"mech", 3, MenuCaptions::Screen, false},
                                     {"dropship", 2, MenuCaptions::Screen, false},
                                     {"component", 1, MenuCaptions::Screen, false},
                                     {"ammo", 13, MenuCaptions::Screen, false}};
constexpr std::string_view kCompatibilityPanels[] = {"compatibility_1", "compatibility_2", "compatibility_3",
                                                     "compatibility_4", "compatibility_5", "compatibility_6"};
constexpr TableSpec kDealerTables[] = {{"compatibility_panels", TableKind::PanelRefs, 6, kCompatibilityPanels}};
// travel.json: the travel map's panels and menu, the house browser, the
// house's planet list (30 planet rows the code fills in), and the transport
// animations. The travel map's last record is the one its others link left
// to; its caption is null, as code fills it in.
constexpr std::string_view kTravelCaptions[] = {
    "search", "planet_1", "planet_2", "planet_3", "planet_4", "planet_5", "planet_6", "planet_7", "planet_8",
    "planet_9", "planet_10", "planet_11", "planet_12", "planet_13", "planet_14", "planet_15", "planet_16",
    "planet_17", "planet_18", "planet_19", "planet_20", "planet_21", "planet_22", "planet_23", "planet_24",
    "planet_25", "planet_26", "planet_27", "planet_28", "planet_29", "planet_30", "planets_back",
    "browser_title", "house_1", "house_2", "house_3", "house_4", "house_5", "browser_close",
    "destination_name", "destination_detail", "confirm_travel", "leave", "browse_houses"};
constexpr PanelSpec kTravelPanels[] = {{"house_planets", 32}, {"house_browser", 7}, {"cost", 1},
                                       {"confirm", 1}, {"destination", 2}, {"browse", 1}};
constexpr MenuSpec kTravelMenus[] = {{"house_planets", 31, MenuCaptions::Screen, false},
                                     {"house_browser", 6, MenuCaptions::Screen, false},
                                     {"map", 5, MenuCaptions::Screen, false}};
constexpr TableSpec kTravelTables[] = {{"hired_transport_frames", TableKind::Words, 34},
                                       {"dropship_frames", TableKind::Words, 22},
                                       {"house_search_words", TableKind::WordRefs, 5}};
constexpr ScreenSpec kScreens[] = {
    {"options", kOptionsCaptions, {}, kOptionsPanels, kOptionsMenus, {}},
    {"events", {}, {}, {}, kEventMenus, {}},
    {"hub", {}, {}, {}, kHubMenus, kHubTables},
    {"crew", kCrewCaptions, kCrewColourCaptions, kCrewPanels, kCrewMenus, kCrewTables},
    {"contract_office", kContractCaptions, {}, kContractPanels, kContractMenus, kContractTables},
    {"complex", kComplexCaptions, {}, kComplexPanels, kComplexMenus, kComplexTables},
    {"dealer", kDealerCaptions, {}, kDealerPanels, kDealerMenus, kDealerTables},
    {"travel", kTravelCaptions, {}, kTravelPanels, kTravelMenus, kTravelTables},
};

}  // namespace

support::NativeBytes load_layout(const fs::path& data_dir) {
    support::NativeBytes out;
    read_cockpit(data_dir, out);
    read_orders(data_dir, out);
    for (const ScreenSpec& screen : kScreens) read_screen(data_dir, screen, out);
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
