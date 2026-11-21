// src/Import/Story: the importer on a made-up exe (no game files needed).
// A copy is accepted piece by piece and a refusal names the pieces; the story
// file round-trips every byte but NUL; import.json goes last; a missing,
// interrupted or outdated story file is imported again.
#include "../src/Import/Story.h"
#include "../src/Support/Sha256.h"

#include <cstdio>
#include <fstream>
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

std::string hash16(const std::string& bytes) {
    return mw1o::support::sha256_hex({reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size()}).substr(0, 16);
}

void write(const fs::path& path, const std::string& bytes) {
    fs::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary) << bytes;
}

std::string read(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

// What threw, or "" if nothing did.
template <class F>
std::string thrown(F f) {
    try {
        f();
    } catch (const std::exception& e) {
        return e.what();
    }
    return "";
}

}  // namespace

int main() {
    const fs::path root = fs::current_path() / "story_test_files";
    fs::remove_all(root);
    const fs::path game = root / "game";
    const fs::path home = root / "home";

    // An MZ exe with a two-paragraph header; its image is loaded at 0800, so
    // 0801:0000 is file offset 0x20 + 0x10.
    std::string every;  // 01..FF: every byte a text can hold
    for (int b = 1; b < 256; ++b) every += static_cast<char>(b);
    const std::string first = every + '\0';
    const std::string second = std::string("second\x02text") + '\0';
    std::string exe(0x30, '\0');
    exe[0] = 'M';
    exe[1] = 'Z';
    exe[8] = 2;
    exe += first + second + "tail";
    write(game / "mw_main.exe", exe);  // lower case on disk: found ignoring case

    const std::uint16_t second_at = static_cast<std::uint16_t>(first.size());
    // The table holds views: the hashes must outlive it.
    const std::string first_hash = hash16(first), second_hash = hash16(second), bytes_hash = hash16(second.substr(0, 4));
    const std::vector<ImportPiece> table = {
        {ImportPiece::Kind::String, "test.first", "MW_MAIN.EXE", 0x0801, 0x0000,
         static_cast<std::uint16_t>(first.size()), first_hash},
        {ImportPiece::Kind::String, "test.second", "MW_MAIN.EXE", 0x0801, second_at,
         static_cast<std::uint16_t>(second.size()), second_hash},
    };

    check(story_state(home, table) == StoryState::Missing, "no story file: missing");
    check(thrown([&] { import_story(game, home, table); }).empty(), "a matching copy imports");
    check(story_state(home, table) == StoryState::Current, "after the import: current");
    const StoryText text = load_story_text(home);
    check(text.size() == 2, "two texts");
    check(text.count("test.first") && text.at("test.first") == every, "bytes 01..FF survive, without the NUL");
    check(text.count("test.second") && text.at("test.second") == "second\x02text", "a control code survives");
    const std::string manifest = read(story_dir(home) / "import.json");
    check(manifest.find("\"import_version\": 1") != std::string::npos, "import.json has the version");
    check(manifest.find(mw1o::support::sha256_hex({reinterpret_cast<const std::uint8_t*>(exe.data()), exe.size()})) !=
              std::string::npos,
          "import.json has the exe's SHA-256");
    check(manifest.find(import_table_hash(table)) != std::string::npos, "import.json has the table's hash");
    check(read(story_dir(home) / "text.json").find("\\u00ff") != std::string::npos, "text.json writes 8-bit as \\u00NN");

    // Another table (a piece added): outdated, and ensure_story imports again.
    std::vector<ImportPiece> longer = table;
    longer.push_back({ImportPiece::Kind::Bytes, "test.bytes", "MW_MAIN.EXE", 0x0801, second_at, 4, bytes_hash});
    check(story_state(home, longer) == StoryState::Outdated, "a new table: outdated");
    check(thrown([&] { check(ensure_story(game, home, longer), "ensure_story imports an outdated file"); }).empty(),
          "ensure_story with the new table");
    check(story_state(home, longer) == StoryState::Current, "re-imported: current");
    check(!ensure_story(game, home, longer), "ensure_story leaves a current file alone");

    // Another import version: outdated.
    std::string old = read(story_dir(home) / "import.json");
    old.replace(old.find("\"import_version\": 1"), 19, "\"import_version\": 0");
    write(story_dir(home) / "import.json", old);
    check(story_state(home, longer) == StoryState::Outdated, "another import version: outdated");

    // Interrupted (text.json written, import.json not yet): missing.
    fs::remove(story_dir(home) / "import.json");
    check(story_state(home, longer) == StoryState::Missing, "no import.json: missing");

    // A copy that differs in one piece: refused, naming that piece only.
    std::string damaged = exe;
    damaged[0x30 + second_at + 1] ^= 1;
    write(game / "mw_main.exe", damaged);
    const std::string why = thrown([&] { import_story(game, home, table); });
    check(why.find("test.second (MW_MAIN.EXE 0801:") != std::string::npos, "the refusal names the piece: " + why);
    check(why.find("test.first") == std::string::npos, "the refusal leaves out the pieces that match: " + why);
    check(why.find("1 of 2") != std::string::npos, "the refusal counts the pieces: " + why);
    check(story_state(home, longer) == StoryState::Missing, "a refused import writes no import.json");
    const std::string ensure_why = thrown([&] { ensure_story(game, home, table); });
    check(ensure_why.find("test.second") != std::string::npos, "ensure_story passes the refusal on: " + ensure_why);

    // A piece past the end of the exe: refused, not read.
    std::vector<ImportPiece> past = {{ImportPiece::Kind::Bytes, "test.past", "MW_MAIN.EXE", 0x0900, 0, 4, "0000000000000000"}};
    write(game / "mw_main.exe", exe);
    check(thrown([&] { import_story(game, home, past); }).find("test.past") != std::string::npos,
          "a piece past the end is refused");

    // No exe at all.
    fs::remove(game / "mw_main.exe");
    check(thrown([&] { import_story(game, home, table); }).find("MW_MAIN.EXE is missing") != std::string::npos,
          "a missing exe is named");

    // A broken text.json: the error names the file.
    write(story_dir(home) / "text.json", "{\"texts\": [{\"id\": \"a\", \"text\": \"x\"},]}");
    check(thrown([&] { load_story_text(home); }).find("text.json") != std::string::npos, "a broken text.json is named");
    write(story_dir(home) / "text.json",
          "{\"texts\": [{\"id\": \"a\", \"text\": \"x\"}, {\"id\": \"a\", \"text\": \"y\"}]}");
    check(thrown([&] { load_story_text(home); }).find("twice") != std::string::npos, "an id twice is refused");

    fs::remove_all(root);
    if (g_failures) {
        std::printf("story_test: %d failed\n", g_failures);
        return 1;
    }
    std::printf("story_test: all passed\n");
    return 0;
}
