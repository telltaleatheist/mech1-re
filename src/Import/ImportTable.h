// The import table: the pieces of the original's creative content (its
// story, docs/phase2/PLAN.md) that the game takes from the player's own
// exes, each by id with its place in the exe, its length and the first 16 hex
// digits of its SHA-256. The repo ships this table, never the bytes; the
// importer (Story.h) reads the pieces from the player's copy and writes them
// to the story file in the player's data folder.
//
// A copy is accepted piece by piece: every piece must match its hash, and a
// refusal names the pieces that differ. Bytes the game doesn't import are not
// checked, so copies that differ only there are accepted.
//
// import_table.cpp is written by tools/import/make_import_table.py from
// pieces.txt; add a piece there and run the tool on a game folder.
#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace mw1o::import {

struct ImportPiece {
    // String: text read up to and including its NUL (the length counts the
    // NUL; the story file keeps the text without it). Bytes: a fixed length.
    enum class Kind { String, Bytes };
    Kind kind;
    std::string_view id;
    std::string_view exe;  // the file in the game folder, e.g. "MW_MAIN.EXE"
    std::uint16_t segment;  // as the traces see it, the image loaded at load_segment(exe)
    std::uint16_t offset;
    std::uint16_t length;
    std::string_view sha16;  // the first 16 hex digits of the piece's SHA-256
};

// Where DOS loaded the image in the traces: a piece's segment is relative to
// it, so SEG:OFF is file offset (header) + (SEG - kLoadSegment) * 16 + OFF.
inline constexpr std::uint16_t kLoadSegment = 0x0800;

// BTECH.EXE is EXEPACKed: its pieces are addressed in the unpacked image,
// rebased into the reference layout (src/Support/BtechImage.cpp) and loaded at
// 0810 as the game sees it, so one row serves both revisions where their bytes
// agree. BTECH's data segment is 304E.
inline constexpr std::string_view kBtechExe = "BTECH.EXE";
inline constexpr std::uint16_t kBtechLoadSegment = 0x0810;

// The segment `exe`'s image is loaded at.
constexpr std::uint16_t load_segment(std::string_view exe) {
    return exe == kBtechExe ? kBtechLoadSegment : kLoadSegment;
}

// The shape of the story file. A data folder written by another version, or
// with another table (import_table_hash), is imported again.
inline constexpr int kImportVersion = 1;

// The revision the table's hashes come from.
inline constexpr std::string_view kRevision = "1990-01-22";

std::span<const ImportPiece> import_table();

// "SSSS:OOOO", as the story file records it.
std::string piece_address(const ImportPiece& piece);

// A hash of every row of `table` (kind, id, exe, address, length, hash).
std::string import_table_hash(std::span<const ImportPiece> table);

}  // namespace mw1o::import
