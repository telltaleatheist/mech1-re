// Written by tools/import/make_import_table.py --ui from ui_pieces.txt; don't edit.
#include "UiTable.h"

namespace mw1o::ui {

namespace {
constexpr UiPiece kTable[] = {
    {"cockpit.inspection_diagram_origins", "BTECH.EXE", 0x304E, 0x1878, 32, "bcbec1086135d78a"},
    {"cockpit.inspection_diagram_regions", "BTECH.EXE", 0x304E, 0x1898, 576, "bea7202dfa964250"},
    {"cockpit.target_diagram_anchors", "BTECH.EXE", 0x304E, 0x1AD8, 32, "7485121b4952477d"},
    {"cockpit.target_diagram_regions", "BTECH.EXE", 0x304E, 0x1AF8, 576, "915cea1b87da9cc9"},
    {"cockpit.layout_by_template", "BTECH.EXE", 0x304E, 0x1D38, 16, "78fc7c7a763ba71a"},
    {"cockpit.layouts", "BTECH.EXE", 0x304E, 0x1D48, 318, "68c66d9cf55520aa"},
    {"cockpit.landscape_camera_rows", "BTECH.EXE", 0x304E, 0x1E88, 180, "a1c8f15bac8a08ad"},
    {"cockpit.hud_colour_schemes", "BTECH.EXE", 0x304E, 0x1F3C, 200, "886bc6ae66fd47ef"},
    {"cockpit.subsystem_lamp_colours", "BTECH.EXE", 0x304E, 0x23CE, 28, "8849ad012db88597"},
    {"orders.selection_first_rows", "BTECH.EXE", 0x304E, 0x0092, 4, "8b629f0ae492bf75"},
    {"orders.selection_last_rows", "BTECH.EXE", 0x304E, 0x0096, 4, "b5717496361a1d11"},
    {"orders.navigation_panel_deltas", "BTECH.EXE", 0x304E, 0x009A, 22, "ef7122c7086c5a4b"},
    {"orders.navigation_row_deltas", "BTECH.EXE", 0x304E, 0x00B0, 22, "54e6ebd040b2a1a4"},
    {"orders.engine_condition_colours", "BTECH.EXE", 0x304E, 0x030E, 16, "f2ca7a127eff9f9b"},
    {"orders.system_condition_colours", "BTECH.EXE", 0x304E, 0x031E, 16, "c0ba92c2d26b99f4"},
};
}  // namespace

std::span<const UiPiece> ui_table() { return kTable; }

}  // namespace mw1o::ui
