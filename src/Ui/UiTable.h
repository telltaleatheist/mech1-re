// The layout table: every original table the layout in data/ui/ stands for
// (docs/phase2/PLAN.md section 6), as src/Support/NativePieces.h describes.
// The game never reads these from an exe; ui_test checks that data/ui
// rebuilds each one byte for byte, and ui_exe_test compares with the
// player's exes.
//
// ui_table.cpp is written by tools/import/make_import_table.py --ui from
// ui_pieces.txt.
#pragma once

#include <span>
#include <string_view>

#include "../Support/NativePieces.h"

namespace mw1o::ui {

using UiPiece = support::NativePiece;

std::span<const UiPiece> ui_table();

// The piece with this id, or null.
inline const UiPiece* find_ui_piece(std::string_view id) { return support::find_piece(ui_table(), id); }

}  // namespace mw1o::ui
