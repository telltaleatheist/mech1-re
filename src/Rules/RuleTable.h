// The rule table: every original table the rules in data/rules/ stand for
// (docs/phase2/PLAN.md section 5), by id, with its place in the exe, its
// length and the first 16 hex digits of the exe's SHA-256 of it. The game
// never reads these from an exe; rules_test checks that data/rules rebuilds
// each one byte for byte, and rules_exe_test compares with the player's exes.
//
// rule_table.cpp is written by tools/import/make_import_table.py --rules from
// rule_pieces.txt. Addresses are as src/Import/ImportTable.h's.
#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace mw1o::rules {

struct RulePiece {
    std::string_view id;
    std::string_view exe;
    std::uint16_t segment;
    std::uint16_t offset;
    std::uint16_t length;
    std::string_view sha16;  // empty: tuned on purpose, no longer the exe's
};

std::span<const RulePiece> rule_table();

// The piece with this id, or null.
inline const RulePiece* find_rule_piece(std::string_view id) {
    for (const RulePiece& piece : rule_table())
        if (piece.id == id) return &piece;
    return nullptr;
}

}  // namespace mw1o::rules
