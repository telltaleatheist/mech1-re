// The rule table against the player's exes: every piece's bytes hash as the
// table says (a tuned piece is only reported). BTECH's pieces are read in its
// unpacked image (src/Support/BtechImage.cpp), in both revisions when a
// reference BTECH.EXE is given.
//
// Needs the player's files, so ctest skips it (77) unless this is set:
//   MW1O_GAME             a game folder (MW_MAIN.EXE, BTECH.EXE)
//   MW1O_BTECH_REFERENCE  optional: a reference BTECH.EXE
// Prints ids and counts only, never the game's bytes.
#include "../src/Rules/RuleTable.h"
#include "../src/Support/Sha256.h"
#include "mechwarrior/combat_installation.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace mw1o::rules;

namespace {

constexpr std::uint16_t kLoadSegment = 0x0800, kBtechLoadSegment = 0x0810;

int g_failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        ++g_failures;
        std::printf("FAIL %s\n", what.c_str());
    }
}

std::vector<std::uint8_t> read(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

fs::path find_file(const fs::path& dir, const std::string& upper_name) {
    for (const auto& entry : fs::directory_iterator(dir)) {
        std::string name = entry.path().filename().string();
        for (auto& c : name) c = (c >= 'a' && c <= 'z') ? char(c - 'a' + 'A') : c;
        if (name == upper_name) return entry.path();
    }
    return {};
}

// An MZ exe's load image (its header dropped).
std::vector<std::uint8_t> load_image(const fs::path& path) {
    auto data = read(path);
    if (data.size() < 0x1C || data[0] != 'M' || data[1] != 'Z') return {};
    const std::size_t header = std::size_t(data[8] | data[9] << 8) * 16;
    return {data.begin() + std::ptrdiff_t(std::min(header, data.size())), data.end()};
}

std::vector<std::uint8_t> btech_image(const fs::path& path) {
    const auto packed = read(path);
    std::uint8_t* image = nullptr;
    BtechRevision revision = BTECH_REVISION_REFERENCE;
    if (packed.empty() || !BtechDecodeImage(packed.data(), packed.size(), &image, &revision)) return {};
    std::vector<std::uint8_t> bytes(image, image + BTECH_IMAGE_BYTES);
    std::free(image);
    return bytes;
}

// Checks every piece of `exe` in `image`; returns how many match.
int check_pieces(std::string_view exe, const std::vector<std::uint8_t>& image, const std::string& label) {
    int same = 0;
    for (const RulePiece& piece : rule_table()) {
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

}  // namespace

int main() {
    const char* game = std::getenv("MW1O_GAME");
    if (!game) {
        std::printf("skipped: set MW1O_GAME (and optionally MW1O_BTECH_REFERENCE)\n");
        return 77;
    }
    const auto main_image = load_image(find_file(game, "MW_MAIN.EXE"));
    const auto btech = btech_image(find_file(game, "BTECH.EXE"));
    check(!main_image.empty(), "MW_MAIN.EXE doesn't load");
    check(!btech.empty(), "BTECH.EXE doesn't unpack");
    const int a = check_pieces("MW_MAIN.EXE", main_image, "MW_MAIN.EXE");
    const int b = check_pieces("BTECH.EXE", btech, "BTECH.EXE");
    std::printf("%d MW_MAIN and %d BTECH rule tables as the table's\n", a, b);
    if (const char* reference = std::getenv("MW1O_BTECH_REFERENCE")) {
        const auto image = btech_image(reference);
        check(!image.empty(), "the reference BTECH.EXE doesn't unpack");
        std::printf("%d BTECH rule tables as the table's in the reference revision\n",
                    check_pieces("BTECH.EXE", image, "the reference BTECH.EXE"));
    }
    std::printf("%s\n", g_failures ? "FAILED" : "ok");
    return g_failures ? 1 : 0;
}
