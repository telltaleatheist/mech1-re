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
    {"options.panel.options", "MW_MAIN.EXE", 0x1090, 0x9D52, 30, "e0a2160bbe99b001"},
    {"options.panel.restore", "MW_MAIN.EXE", 0x1090, 0x9E4D, 44, "3081c32d2e124a96"},
    {"options.panel.confirmation", "MW_MAIN.EXE", 0x1090, 0x9CBE, 22, "4b38e486de30b83d"},
    {"options.panel.news", "MW_MAIN.EXE", 0x1090, 0x2500, 22, "1a17de559884dea0"},
    {"options.menu.options", "MW_MAIN.EXE", 0x1090, 0x9DEB, 98, "d5ccae4bc492fc62"},
    {"options.menu.restore", "MW_MAIN.EXE", 0x1090, 0x9F7B, 182, "ab4a3e363b3f2fee"},
    {"options.menu.confirmation", "MW_MAIN.EXE", 0x1090, 0x9D34, 28, "5ac6ddd4bf60fe54"},
    {"options.menu.news", "MW_MAIN.EXE", 0x1090, 0x2538, 42, "9cc9894e3375fc8d"},
    {"options.caption.save", "MW_MAIN.EXE", 0x1090, 0x9D70, 5, "c71d644845f1d66f"},
    {"options.caption.restore", "MW_MAIN.EXE", 0x1090, 0x9D80, 5, "c563aaa319babb0f"},
    {"options.caption.sound", "MW_MAIN.EXE", 0x1090, 0x9D92, 5, "1d307fe21036d462"},
    {"options.caption.detail", "MW_MAIN.EXE", 0x1090, 0x9DA6, 5, "87358b86b90902a1"},
    {"options.caption.restart", "MW_MAIN.EXE", 0x1090, 0x9DB8, 5, "285981a2220c9bd5"},
    {"options.caption.quit", "MW_MAIN.EXE", 0x1090, 0x9DCA, 5, "f0361d35e8b20b86"},
    {"options.caption.continue", "MW_MAIN.EXE", 0x1090, 0x9DDB, 5, "0f03aad8c9cc5dc2"},
    {"options.caption.confirm_quit", "MW_MAIN.EXE", 0x1090, 0x9CD4, 5, "7d723828410d6b01"},
    {"options.caption.confirm_restart", "MW_MAIN.EXE", 0x1090, 0x9CFA, 5, "7d723828410d6b01"},
    {"options.caption.yes", "MW_MAIN.EXE", 0x1090, 0x9D23, 5, "ab4ecdf9578c0268"},
    {"options.caption.no", "MW_MAIN.EXE", 0x1090, 0x9D2C, 5, "55a8608166141f0f"},
    {"options.caption.news_previous", "MW_MAIN.EXE", 0x1090, 0x2516, 5, "4e36f68d37c6e981"},
    {"options.caption.news_next", "MW_MAIN.EXE", 0x1090, 0x2524, 5, "b487ba941f046f4a"},
    {"options.caption.news_done", "MW_MAIN.EXE", 0x1090, 0x252E, 5, "b8005ccc0704736f"},
    {"options.caption.restore_heading", "MW_MAIN.EXE", 0x1090, 0x9E79, 5, "1c7658a45df5dbfd"},
    {"options.caption.save_1", "MW_MAIN.EXE", 0x1090, 0x9E94, 5, "639623e31472ceb2"},
    {"options.caption.save_2", "MW_MAIN.EXE", 0x1090, 0x9EA6, 5, "fc6c6f3f952a904e"},
    {"options.caption.save_3", "MW_MAIN.EXE", 0x1090, 0x9EB8, 5, "57e069014157b03b"},
    {"options.caption.save_4", "MW_MAIN.EXE", 0x1090, 0x9ECA, 5, "543090eb8c4eeffd"},
    {"options.caption.save_5", "MW_MAIN.EXE", 0x1090, 0x9EDC, 5, "ce6c0d147432db5d"},
    {"options.caption.save_6", "MW_MAIN.EXE", 0x1090, 0x9EEE, 5, "0f1e9381796e04fa"},
    {"options.caption.save_7", "MW_MAIN.EXE", 0x1090, 0x9F00, 5, "1449aa03c2456a12"},
    {"options.caption.save_8", "MW_MAIN.EXE", 0x1090, 0x9F12, 5, "449d4d6382d61bcd"},
    {"options.caption.save_9", "MW_MAIN.EXE", 0x1090, 0x9F24, 5, "96bf8ce3a3e68299"},
    {"options.caption.save_10", "MW_MAIN.EXE", 0x1090, 0x9F36, 5, "14510fa62e6c24dc"},
    {"options.caption.save_11", "MW_MAIN.EXE", 0x1090, 0x9F48, 5, "8b5d229e9b522781"},
    {"options.caption.save_12", "MW_MAIN.EXE", 0x1090, 0x9F5A, 5, "42bd4b69f29bca72"},
    {"options.caption.restore_cancel", "MW_MAIN.EXE", 0x1090, 0x9F6C, 5, "890c536eb5f51e58"},
    {"events.menu.two_choices", "MW_MAIN.EXE", 0x1090, 0x5E1C, 28, "7102f11317385cd8"},
    {"events.menu.three_choices", "MW_MAIN.EXE", 0x1090, 0x5E38, 42, "bcbfc1632d33da55"},
};
}  // namespace

std::span<const UiPiece> ui_table() { return kTable; }

}  // namespace mw1o::ui
