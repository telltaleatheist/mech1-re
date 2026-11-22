// The layout (docs/phase2/PLAN.md section 6): where things go on screen and
// in what colour, read from <data dir>/ui/*.json and laid out as the exe
// lays it out (UiTable.h). Numbers and our own names only, never exe text.
//
//   cockpit.json (BTECH)
//     {"templates": [8, in template order: {
//        "cockpit_layout": index into "layouts",
//        "inspection_diagram": {"origin": [x, y], "regions": {armour: rect}},
//        "target_diagram": {"anchor": [x, y], "regions": {armour: rect}}}],
//      "layouts": [3 cockpit layouts, BtechCockpitLayout's members],
//      "landscape_camera_rows": [2 colour schemes of {"desert", "tropical",
//        "arctic"}, BtechLandscapeCameraColourRow's members],
//      "hud_colour_schemes": [2, BtechHudColourScheme's members],
//      "subsystem_lamp_colours": {"subsystem": [2 schemes of 3],
//        "engine": [2 schemes of 4]},
//      "word_refs": {"legends": [2], "weapon_labels": [10],
//        "radar_ranges": [4], "range_classes": [4], "energy_ammo": word},
//      "condition_labels": {"two_hit": [3], "engine": [4], "actuator": [5]}}
//     A word is a name after "cockpit.word." in src/Import/word_pieces.txt,
//     laid out as that word's offset; a condition label is an index into the
//     inspection's 6 condition words.
//   orders.json (BTECH, the lance order panel)
//     {"selection_rows": {"first": [2], "last": [2]},
//      "navigation_deltas": {"panel": [11], "row": [11]},
//      "condition_colours": {"engine": [2 schemes of 4],
//        "system": [2 schemes of 4]},
//      "start": {"lance_panel_text_x": [3]},
//      "word_refs": {"orders": [7], "tactical": [9], "damage": [7],
//        "weapons": [10]},
//      "display_indices": [11]}
//     A word is a name after "orders.word."; each display index names one of
//     the 7 order words for an order code. "start" holds what play writes but a fresh EXEC reads first.
//   MW_MAIN's screens (options, events, hub, crew, contract_office,
//   complex, dealer, travel; each leaves out the members it has none of)
//     {"captions": {name: {"at": [x, y], "colour": c} | {"colour": c}},
//      "panels": {name: {"at": [x, y], "width", "bottom_inset", "border",
//        "line_spacing", "background", "lines_shown" (list panels only),
//        "lines": [caption names]}},
//      "menus": {name: {"records": [{"enabled": bool, "text_record": bool,
//        "at": [x, y], "action": "continue.mech_complex" | null,
//        "caption": name | null (a text record) | "picture": n (otherwise),
//        "up", "down", "left", "right": record index | null}]}},
//      "tables": {name: [words] | {unit kind: word} | [{field: word}] |
//        [colour caption names] | [panel names, of those it may hold] |
//        [{"background": rect,
//        "regions": [9 rects]}] | {mech kind: diagram index | null}}}
//     A caption is its head; its text follows in the exe until slice 5. A
//     colour caption (crew's pilot assignment captions) is a colour byte
//     alone, named by a pointer table rather than a panel. A null caption
//     on a text record is one code fills in. A table of rows is kept as the
//     exe keeps it, one field after another. A list panel (the owned and
//     market lists, repair, the dealer's components) has a line for every
//     row while its header counts the lines it starts out showing. Body
//     diagram regions follow the campaign's body damage order.
//     Actions are the continuation enums' members, by name; a link must stay
//     inside its menu. The event menus' records name the story's captions by
//     the original address story/events.json keeps ("story_caption").
//
// In the cockpit, a point is [x, y] and a rectangle [x, y, width, height],
// signed (a screen's points are unsigned). Armour
// locations are keyed by COMBAT_ARMOR_* (src/Rules/Rules.h's names). The
// colour schemes follow hudColourSchemeIndex (0, then 1). Fields the port
// hasn't named keep their offset: "unknown_<offset>". An index that names
// nothing (a cockpit layout past the last) is refused.
#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>

#include "../Support/NativePieces.h"

namespace mw1o::ui {

struct UiError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Reads <data_dir>/ui/ strictly and returns every layout piece (UiTable.h)
// by id, as the exe lays it out. Throws UiError naming the file.
support::NativeBytes load_layout(const std::filesystem::path& data_dir);

}  // namespace mw1o::ui
