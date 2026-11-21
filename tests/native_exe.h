// Shared by rules_exe_test and ui_exe_test: a piece table against the
// player's exes. Every piece's bytes must hash as the table says (a tuned
// piece is only reported). BTECH's pieces are read in its unpacked image
// (src/Support/BtechImage.cpp). Prints ids and counts only, never the game's
// bytes.
#pragma once

#include "../src/Support/NativePieces.h"
#include "../src/Support/Sha256.h"
#include "mechwarrior/combat_installation.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace native_exe {

inline constexpr std::uint16_t kLoadSegment = 0x0800, kBtechLoadSegment = 0x0810;

inline int g_failures = 0;

inline void check(bool ok, const std::string& what) {
    if (!ok) {
        ++g_failures;
        std::printf("FAIL %s\n", what.c_str());
    }
}

inline std::vector<std::uint8_t> read(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

inline fs::path find_file(const fs::path& dir, const std::string& upper_name) {
    for (const auto& entry : fs::directory_iterator(dir)) {
        std::string name = entry.path().filename().string();
        for (auto& c : name) c = (c >= 'a' && c <= 'z') ? char(c - 'a' + 'A') : c;
        if (name == upper_name) return entry.path();
    }
    return {};
}

// An MZ exe's load image (its header dropped).
inline std::vector<std::uint8_t> load_image(const fs::path& path) {
    auto data = read(path);
    if (data.size() < 0x1C || data[0] != 'M' || data[1] != 'Z') return {};
    const std::size_t header = std::size_t(data[8] | data[9] << 8) * 16;
    return {data.begin() + std::ptrdiff_t(std::min(header, data.size())), data.end()};
}

inline std::vector<std::uint8_t> btech_image(const fs::path& path) {
    const auto packed = read(path);
    std::uint8_t* image = nullptr;
    BtechRevision revision = BTECH_REVISION_REFERENCE;
    if (packed.empty() || !BtechDecodeImage(packed.data(), packed.size(), &image, &revision)) return {};
    std::vector<std::uint8_t> bytes(image, image + BTECH_IMAGE_BYTES);
    std::free(image);
    return bytes;
}

// Checks every piece of `exe` in `image`; returns how many match.
inline int check_pieces(std::span<const mw1o::support::NativePiece> table, std::string_view exe,
                 const std::vector<std::uint8_t>& image, const std::string& label) {
    int same = 0;
    for (const mw1o::support::NativePiece& piece : table) {
        if (piece.exe != exe) continue;
        const std::uint16_t load = exe == "BTECH.EXE" ? kBtechLoadSegment : kLoadSegment;
        const std::size_t at = std::size_t(piece.segment - load) * 16 + piece.offset;
        if (at + piece.length > image.size()) {
            check(false, label + ": " + std::string(piece.id) + " runs past the image");
            continue;
        }
        if (piece.sha16.empty()) {
            std::printf("  tuned: %s\n", std::string(piece.id).c_str());
            continue;
        }
        const bool ok = mw1o::support::sha256_hex({image.data() + at, piece.length}).substr(0, 16) == piece.sha16;
        check(ok, label + ": " + std::string(piece.id) + " differs");
        same += ok;
    }
    return same;
}

}  // namespace native_exe
