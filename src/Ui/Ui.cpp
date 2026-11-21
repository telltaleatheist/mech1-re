#include "Ui.h"

#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "../Rules/Rules.h"
#include "../Support/StrictReader.h"
#include "UiActions.h"
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
};
struct MenuSpec {
    std::string_view name;
    std::size_t records;
    MenuCaptions captions;
    // The exe's structure has one link one past the last record, which the
    // original's past-end probe answers (PLAN 6.1); otherwise refused.
    bool past_end_link;
};
// A table is a list of words; rows of named words, which the exe keeps one
// field after another (all the first fields, then all the second); or a list
// of references to colour captions (the exe's pointer table).
enum class TableKind { Words, Rows, CaptionRefs };
struct TableSpec {
    std::string_view name;
    TableKind kind;
    std::size_t count;
    std::span<const std::string_view> fields;  // Rows only
};
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

void put_table(const Reader& r, std::string& out, const Json& v, const TableSpec& table, const Captions& captions) {
    const std::string what = "the table \"" + std::string(table.name) + "\"";
    if (!v.is_array() || v.array().size() != table.count)
        r.fail(what + " is not a list of " + std::to_string(table.count));
    const Json::Array& entries = v.array();
    switch (table.kind) {
    case TableKind::Words:
        for (const Json& n : entries) put_word(out, r.word(n, what));
        break;
    case TableKind::CaptionRefs:
        for (const Json& n : entries) put_word(out, captions.colour_caption(r, n, what + "'s entry"));
        break;
    case TableKind::Rows:
        for (const Json& row : entries) r.only(row, table.fields);
        for (std::string_view field : table.fields)
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
        r.only(p, {"at", "width", "bottom_inset", "border", "line_spacing", "background", "lines"});
        std::string bytes;
        put_unsigned_point(r, bytes, r.member(p, "at"), "at");
        put_word(bytes, r.u16(p, "width"));
        put_word(bytes, r.u16(p, "bottom_inset"));
        const auto& lines = r.array(p, "lines", spec.lines);
        put_word(bytes, static_cast<std::uint16_t>(lines.size()));
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
        put_table(r, bytes, *tables[i], screen.tables[i], captions);
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
                                    {"notification_boxes", TableKind::Rows, 4, kNotificationBoxFields}};
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
                                     {"pilot_assignment_captions", TableKind::CaptionRefs, 12, {}}};
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
constexpr ScreenSpec kScreens[] = {
    {"options", kOptionsCaptions, {}, kOptionsPanels, kOptionsMenus, {}},
    {"events", {}, {}, {}, kEventMenus, {}},
    {"hub", {}, {}, {}, kHubMenus, kHubTables},
    {"crew", kCrewCaptions, kCrewColourCaptions, kCrewPanels, kCrewMenus, kCrewTables},
    {"contract_office", kContractCaptions, {}, kContractPanels, kContractMenus, kContractTables},
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
