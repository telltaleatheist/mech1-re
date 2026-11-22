// BTECH.EXE's two known revisions (the reference and the 1990-01-22 build)
// import to the same story wherever their bytes agree: one table, one
// code path (src/Support/BtechImage.cpp unpacks and rebases both).
//
// Needs the player's files, so ctest skips it (77) unless these are set:
//   MW1O_GAME             a game folder (MW_MAIN.EXE is taken from it)
//   MW1O_BTECH_REFERENCE  a reference BTECH.EXE
//   MW1O_BTECH_1990       a 1990-01-22 BTECH.EXE
// Prints ids and counts only, never the game's text.
#include "../src/Import/ImportTable.h"
#include "../src/Import/Story.h"
#include "mechwarrior/combat_installation.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace mw1o::import;

namespace {

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

std::vector<std::uint8_t> unpacked(const fs::path& exe, BtechRevision want) {
    const auto packed = read(exe);
    std::uint8_t* image = nullptr;
    BtechRevision got = BTECH_REVISION_REFERENCE;
    if (!BtechDecodeImage(packed.data(), packed.size(), &image, &got)) {
        check(false, exe.string() + " doesn't unpack");
        return {};
    }
    check(got == want, exe.string() + " is not the revision it was given as");
    std::vector<std::uint8_t> bytes(image, image + BTECH_IMAGE_BYTES);
    std::free(image);
    return bytes;
}

fs::path find_file(const fs::path& dir, const std::string& upper_name) {
    for (const auto& entry : fs::directory_iterator(dir)) {
        std::string name = entry.path().filename().string();
        for (auto& c : name) c = (c >= 'a' && c <= 'z') ? char(c - 'a' + 'A') : c;
        if (name == upper_name) return entry.path();
    }
    return {};
}

// A game folder with MW_MAIN.EXE, MW_EGA.EXE and the given BTECH.EXE (copies:
// links need privileges on Windows).
fs::path game_folder(const fs::path& root, const fs::path& game, const fs::path& btech) {
    fs::remove_all(root);
    fs::create_directories(root);
    fs::copy_file(find_file(game, "MW_MAIN.EXE"), root / "MW_MAIN.EXE");
    fs::copy_file(find_file(game, "MW_EGA.EXE"), root / "MW_EGA.EXE");
    fs::copy_file(btech, root / "BTECH.EXE");
    return root;
}

}  // namespace

int main() {
    const char* game = std::getenv("MW1O_GAME");
    const char* reference = std::getenv("MW1O_BTECH_REFERENCE");
    const char* build1990 = std::getenv("MW1O_BTECH_1990");
    if (!game || !reference || !build1990) {
        std::printf("skipped: set MW1O_GAME, MW1O_BTECH_REFERENCE and MW1O_BTECH_1990\n");
        return 77;
    }

    // Where the two unpacked images differ in a piece the table takes.
    const auto a = unpacked(reference, BTECH_REVISION_REFERENCE);
    const auto b = unpacked(build1990, BTECH_REVISION_1990_01_22);
    if (a.empty() || b.empty()) return 1;
    std::set<std::string> differ;
    int btech_pieces = 0;
    for (const auto& piece : import_table()) {
        if (piece.exe != kBtechExe) continue;
        ++btech_pieces;
        const std::size_t at = std::size_t(piece.segment - kBtechLoadSegment) * 16 + piece.offset;
        if (std::vector<std::uint8_t>(a.begin() + at, a.begin() + at + piece.length) !=
            std::vector<std::uint8_t>(b.begin() + at, b.begin() + at + piece.length))
            differ.insert(std::string(piece.id));
    }
    std::printf("%d BTECH pieces, %zu differ between the revisions\n", btech_pieces, differ.size());
    for (const auto& id : differ) std::printf("  differs: %s\n", id.c_str());

    // Each revision's import: the same text for every piece whose bytes agree.
    // In the directory ctest runs it from (the build), removed at the end.
    const fs::path work = fs::current_path() / "btech_revisions_work";
    StoryText texts[2];
    Words words[2];
    std::string music[2];  // BTECH's one Bytes piece, music.adlib_instruments, imports to music.json
    Native native[2];      // BTECH's native spans (story/native.json)
    const fs::path exes[2] = {reference, build1990};
    for (int r = 0; r < 2; ++r) {
        const fs::path folder = game_folder(work / ("game" + std::to_string(r)), game, exes[r]);
        const fs::path write = work / ("write" + std::to_string(r));
        try {
            import_story(folder, write, import_table());
            texts[r] = load_story_text(write);
            words[r] = load_words(write);
            native[r] = load_native(write);
            std::ifstream in(story_dir(write) / "music.json", std::ios::binary);
            music[r].assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
        } catch (const ImportError& e) {
            // Only pieces that really differ may refuse a revision.
            const std::string message = e.what();
            bool explained = !differ.empty();
            for (const auto& piece : import_table())
                if (piece.exe == kBtechExe && !differ.count(std::string(piece.id)) &&
                    message.find(std::string(piece.id) + " (") != std::string::npos)
                    explained = false;
            check(explained, std::string(r ? "the 1990-01-22" : "the reference") +
                                 " revision is refused for pieces whose bytes agree");
        }
    }
    if (!texts[0].empty() && !texts[1].empty()) {
        int same = 0;
        for (const auto& piece : import_table()) {
            if (piece.exe != kBtechExe || differ.count(std::string(piece.id))) continue;
            if (is_word(piece.kind)) {  // the words, from words.json
                const auto x = words[0].find(piece.id), y = words[1].find(piece.id);
                check(x != words[0].end() && y != words[1].end() && x->second == y->second,
                      std::string(piece.id) + " imports differently");
                ++same;
                continue;
            }
            if (is_native(piece.kind)) {
                const auto x = native[0].find(piece.id), y = native[1].find(piece.id);
                check(x != native[0].end() && y != native[1].end() && x->second == y->second,
                      std::string(piece.id) + " imports differently");
                ++same;
                continue;
            }
            if (piece.kind == ImportPiece::Kind::Bytes) {
                check(!music[0].empty() && music[0] == music[1], std::string(piece.id) + " imports differently");
                ++same;
                continue;
            }
            const auto x = texts[0].find(piece.id), y = texts[1].find(piece.id);
            check(x != texts[0].end() && y != texts[1].end() && x->second == y->second,
                  std::string(piece.id) + " imports differently");
            ++same;
        }
        std::printf("%d BTECH pieces import identically in both\n", same);
    }
    fs::remove_all(work);
    std::printf("%s\n", g_failures ? "FAILED" : "ok");
    return g_failures ? 1 : 0;
}
