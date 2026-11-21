// Native pieces: original tables our data files stand for (the rules,
// src/Rules; the layout, src/Ui), each by id with its place in the exe, its
// length and the first 16 hex digits of the exe's SHA-256 of it. The tables
// are written by tools/import/make_import_table.py; addresses are as
// src/Import/ImportTable.h's.
//
// NativeStore holds a table's pieces as the exe lays them out, rebuilt from
// our files, for the C doors (mechwarrior/rules.h, mechwarrior/ui_layout.h).
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <span>
#include <string>
#include <string_view>

namespace mw1o::support {

struct NativePiece {
    std::string_view id;
    std::string_view exe;
    std::uint16_t segment;
    std::uint16_t offset;
    std::uint16_t length;
    std::string_view sha16;  // empty: tuned on purpose, no longer the exe's
};

// The piece with this id in `table`, or null.
inline const NativePiece* find_piece(std::span<const NativePiece> table, std::string_view id) {
    for (const NativePiece& piece : table)
        if (piece.id == id) return &piece;
    return nullptr;
}

using NativeBytes = std::map<std::string, std::string, std::less<>>;

// One table's rebuilt pieces. Each call names the failure on stderr, with
// `what` ("rules", "layout") saying whose piece it was.
class NativeStore {
public:
    NativeStore(std::span<const NativePiece> table, const char* what) : table_(table), what_(what) {}

    void hold(NativeBytes bytes) { bytes_ = std::move(bytes); }
    // The whole piece; `size` must be its length.
    bool copy_native(const char* id, std::uint8_t* bytes, std::size_t size) const;
    // `count` bytes, or little-endian words, from byte `at` of the piece.
    bool copy_bytes(const char* id, std::uint8_t* bytes, std::size_t count, std::size_t at) const;
    bool copy_words(const char* id, std::uint16_t* words, std::size_t count, std::size_t at) const;
    // The piece's length, or 0 if there is none.
    std::size_t size(const char* id) const;

private:
    const char* range(const char* id, std::size_t count, std::size_t at) const;

    std::span<const NativePiece> table_;
    const char* what_;
    NativeBytes bytes_;
};

}  // namespace mw1o::support
