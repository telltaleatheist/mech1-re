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
// The UI words (docs/phase2/PLAN.md section 7) are pieces of the same table,
// of the two Word kinds, written to story/words.json. Their hash is keyed
// (HMAC-SHA-256, src/Support/Hmac.h) by the SHA-256 of a region of the same
// exe's code (key_region), which the repo never holds, so a short word can't
// be found from its hash by trying a dictionary.
//
// import_table.cpp is written by tools/import/make_import_table.py from
// pieces.txt and word_pieces.txt; add a piece there and run the tool on a game
// folder.
#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace mw1o::import {

struct ImportPiece {
    // String: text read up to and including its NUL (the length counts the
    // NUL; the story file keeps the text without it). Bytes: a fixed length.
    // Word and WordBytes: a UI word, as String and Bytes, with a keyed hash.
    enum class Kind { String, Bytes, Word, WordBytes };
    Kind kind;
    std::string_view id;
    std::string_view exe;  // the file in the game folder, e.g. "MW_MAIN.EXE"
    std::uint16_t segment;  // as the traces see it, the image loaded at load_segment(exe)
    std::uint16_t offset;
    std::uint16_t length;
    std::string_view sha16;  // the first 16 hex digits of the piece's SHA-256 (a word's: its keyed hash)
};

constexpr bool is_word(ImportPiece::Kind kind) {
    return kind == ImportPiece::Kind::Word || kind == ImportPiece::Kind::WordBytes;
}
constexpr bool is_string(ImportPiece::Kind kind) {
    return kind == ImportPiece::Kind::String || kind == ImportPiece::Kind::Word;
}

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

// The region of `exe`'s image whose SHA-256 keys its words' hashes: code, in
// both of BTECH's revisions the same once rebased (measured for slice 5a).
struct KeyRegion {
    std::uint16_t segment;
    std::uint16_t offset;
    std::uint16_t length;
};
constexpr KeyRegion key_region(std::string_view exe) {
    return exe == kBtechExe ? KeyRegion{0x2C53, 0x0010, 0x1400}   // in the video code (2A36)
                            : KeyRegion{0x081B, 0x0100, 0x2000};  // in MW_MAIN's campaign code
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
