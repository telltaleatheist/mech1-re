// The UI words (docs/phase2/PLAN.md section 7) imported from the player's exes
// are the exe's bytes: each word in story/words.json against the decoded
// image at its address, compared in memory, for the game folder's BTECH.EXE
// and, when given, the reference revision's.
//
// Needs the player's files, so ctest skips it (77) unless MW1O_GAME is set;
// MW1O_BTECH_REFERENCE adds the other revision. Prints ids, lengths and hash
// prefixes only, never the words.
#include "../src/Import/ImportTable.h"
#include "../src/Import/Story.h"
#include "../src/Support/Sha256.h"
#include "mechwarrior/combat_installation.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
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

fs::path find_file(const fs::path& dir, const std::string& upper_name) {
    for (const auto& entry : fs::directory_iterator(dir)) {
        std::string name = entry.path().filename().string();
        for (auto& c : name) c = (c >= 'a' && c <= 'z') ? char(c - 'a' + 'A') : c;
        if (name == upper_name) return entry.path();
    }
    return {};
}

std::string hash16(const std::string& bytes) {
    return mw1o::support::sha256_hex({reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size()}).substr(0, 16);
}

// Imports from a folder with the game's MW_MAIN.EXE and `btech`, and checks
// every word against `btech`'s decoded image. The number of words checked.
int check_revision(const fs::path& work, const fs::path& game, const fs::path& btech, const char* name) {
    const fs::path folder = work / "game", write = work / "write";
    fs::remove_all(work);
    fs::create_directories(folder);
    fs::copy_file(find_file(game, "MW_MAIN.EXE"), folder / "MW_MAIN.EXE");
    fs::copy_file(btech, folder / "BTECH.EXE");
    const auto packed = read(btech);
    std::uint8_t* decoded = nullptr;
    if (!BtechDecodeImage(packed.data(), packed.size(), &decoded, nullptr)) {
        check(false, std::string(name) + ": BTECH.EXE doesn't unpack");
        return 0;
    }
    const std::string image(reinterpret_cast<const char*>(decoded), BTECH_IMAGE_BYTES);
    std::free(decoded);
    Words words;
    try {
        import_story(folder, write);
        words = load_words(write);
    } catch (const std::exception& e) {
        check(false, std::string(name) + ": the import failed");
        std::printf("  %s\n", e.what());  // ids and addresses only
        return 0;
    }
    int checked = 0;
    for (const auto& piece : import_table()) {
        if (!is_word(piece.kind) || piece.exe != kBtechExe) continue;
        const std::size_t at = std::size_t(piece.segment - kBtechLoadSegment) * 16 + piece.offset;
        const auto it = words.find(piece.id);
        const std::string exe = image.substr(at, piece.length);
        if (it == words.end() || it->second != exe) {
            check(false, std::string(name) + ": " + std::string(piece.id) + " is not the exe's (" +
                             std::to_string(it == words.end() ? 0 : it->second.size()) + " bytes, " +
                             (it == words.end() ? std::string("missing") : hash16(it->second)) + " against " +
                             std::to_string(exe.size()) + " bytes, " + hash16(exe) + ")");
            continue;
        }
        ++checked;
    }
    return checked;
}

}  // namespace

int main() {
    const char* game = std::getenv("MW1O_GAME");
    if (!game) {
        std::printf("skipped: set MW1O_GAME (and MW1O_BTECH_REFERENCE for the other revision)\n");
        return 77;
    }
    const fs::path work = fs::current_path() / "words_exe_work";
    int words = 0;
    for (const auto& piece : import_table()) words += is_word(piece.kind) ? 1 : 0;
    const int own = check_revision(work / "own", game, find_file(game, "BTECH.EXE"), "the game folder's BTECH.EXE");
    std::printf("%d of %d words are the game folder's BTECH.EXE's bytes\n", own, words);
    check(own == words, "every word checked in the game folder's revision");
    if (const char* reference = std::getenv("MW1O_BTECH_REFERENCE")) {
        const int ref = check_revision(work / "reference", game, reference, "the reference BTECH.EXE");
        std::printf("%d of %d words are the reference BTECH.EXE's bytes\n", ref, words);
        check(ref == words, "every word checked in the reference revision");
    }
    fs::remove_all(work);
    std::printf("%s\n", g_failures ? "FAILED" : "ok");
    return g_failures ? 1 : 0;
}
