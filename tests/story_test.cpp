// src/Import/Story: the importer on a made-up exe (no game files needed).
// A copy is accepted piece by piece and a refusal names the pieces; every
// piece's hash is keyed by the exe's key region, so a plain SHA-256 is
// refused; the story file round-trips every byte but NUL; import.json goes
// last; a missing, interrupted or outdated story file is imported again, and
// so is one written before every piece was keyed (import version 1).
#include "../src/Import/Story.h"
#include "../src/Support/Hmac.h"
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

std::span<const std::uint8_t> span_of(const std::string& s) {
    return {reinterpret_cast<const std::uint8_t*>(s.data()), s.size()};
}

std::string plain16(const std::string& bytes) { return mw1o::support::sha256_hex(span_of(bytes)).substr(0, 16); }

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
    // 0801:0000 is file offset 0x20 + 0x10. The image runs on past MW_MAIN's
    // key region, filled with a made-up pattern.
    std::string every;  // 01..FF: every byte a text can hold
    for (int b = 1; b < 256; ++b) every += static_cast<char>(b);
    const std::string first = every + '\0';
    const std::string second = std::string("second\x02text") + '\0';
    std::string exe(0x30, '\0');
    exe[0] = 'M';
    exe[1] = 'Z';
    exe[8] = 2;
    exe += first + second + "tail";
    const KeyRegion region = key_region("MW_MAIN.EXE");
    const std::size_t key_at = std::size_t(region.segment - kLoadSegment) * 16 + region.offset;
    exe.resize(0x20 + key_at + region.length + 16, '\0');
    for (std::size_t i = 0; i < region.length; ++i) exe[0x20 + key_at + i] = static_cast<char>(i * 5 + 1);
    write(game / "mw_main.exe", exe);  // lower case on disk: found ignoring case
    const auto key_digest = mw1o::support::sha256_digest(span_of(exe.substr(0x20 + key_at, region.length)));
    const std::string key(reinterpret_cast<const char*>(key_digest.data()), key_digest.size());
    auto hash16 = [&](const std::string& bytes) {
        return mw1o::support::hmac_sha256_hex(span_of(key), span_of(bytes)).substr(0, 16);
    };

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
    const std::string version = "\"import_version\": " + std::to_string(kImportVersion);
    check(manifest.find(version) != std::string::npos, "import.json has the version");
    check(manifest.find(mw1o::support::sha256_hex({reinterpret_cast<const std::uint8_t*>(exe.data()), exe.size()})) !=
              std::string::npos,
          "import.json has the exe's SHA-256");
    check(manifest.find(import_table_hash(table)) != std::string::npos, "import.json has the table's hash");
    check(read(story_dir(home) / "text.json").find("\\u00ff") != std::string::npos, "text.json writes 8-bit as \\u00NN");

    // A table of plain SHA-256s (the rule before every piece was keyed): every
    // piece refused.
    const std::string first_plain = plain16(first), second_plain = plain16(second);
    std::vector<ImportPiece> plain = table;
    plain[0].sha16 = first_plain;
    plain[1].sha16 = second_plain;
    std::string plain_why = thrown([&] { import_story(game, home, plain); });
    check(plain_why.find("test.first") != std::string::npos && plain_why.find("2 of 2") != std::string::npos,
          "plain hashes are refused: " + plain_why);
    check(story_state(home, table) == StoryState::Current, "a refused import leaves the story as it was");

    // A story folder written before every piece was keyed: import version 1
    // and the plain table's hash. Outdated, and ensure_story imports it again.
    std::string before = manifest;
    before.replace(before.find(version), version.size(), "\"import_version\": 1");
    before.replace(before.find(import_table_hash(table)), import_table_hash(table).size(), import_table_hash(plain));
    write(story_dir(home) / "import.json", before);
    check(story_state(home, table) == StoryState::Outdated, "an old-format story folder: outdated");
    check(thrown([&] { check(ensure_story(game, home, table), "ensure_story imports the old folder"); }).empty(),
          "ensure_story re-imports the old folder");
    check(story_state(home, table) == StoryState::Current, "the old folder re-imported: current");
    check(read(story_dir(home) / "import.json") == manifest, "the old folder's import.json is as a fresh import's");
    check(load_story_text(home) == text, "the old folder's text re-imported");

    // Another table (a piece added): outdated, and ensure_story imports again.
    std::vector<ImportPiece> longer = table;
    longer.push_back({ImportPiece::Kind::Bytes, "test.bytes", "MW_MAIN.EXE", 0x0801, second_at, 4, bytes_hash});
    check(story_state(home, longer) == StoryState::Outdated, "a new table: outdated");
    check(thrown([&] { check(ensure_story(game, home, longer), "ensure_story imports an outdated file"); }).empty(),
          "ensure_story with the new table");
    check(story_state(home, longer) == StoryState::Current, "re-imported: current");
    check(!ensure_story(game, home, longer), "ensure_story leaves a current file alone");

    // A native span (story/native.json): its bytes, whole, by id.
    std::vector<ImportPiece> native = table;
    native.push_back({ImportPiece::Kind::Native, "test.native", "MW_MAIN.EXE", 0x0801, second_at, 4, bytes_hash});
    check(thrown([&] { import_story(game, home, native); }).empty(), "a table with a native span imports");
    Native spans;
    check(thrown([&] { spans = load_native(home, native); }).empty(), "native.json loads");
    check(spans.size() == 1 && spans.count("test.native") && spans.at("test.native") == second.substr(0, 4),
          "the native span's bytes, whole");
    check(load_story_text(home).count("test.native") == 0, "a native span is not a text");
    const std::string native_json = read(story_dir(home) / "native.json");
    const auto native_refused = [&](const std::string& contents, const std::string& why, const std::string& what) {
        write(story_dir(home) / "native.json", contents);
        const std::string got = thrown([&] { load_native(home, native); });
        check(got.find("native.json") != std::string::npos && got.find(why) != std::string::npos, what + ": " + got);
    };
    native_refused("{\"native\": []}", "\"test.native\" is missing", "a missing span is refused");
    native_refused("{\"native\": [{\"id\": \"test.native\", \"bytes\": \"0011\"}]}", "is 2 bytes, not 4",
                   "a span of another length is refused");
    native_refused("{\"native\": [{\"id\": \"test.other\", \"bytes\": \"00112233\"}]}", "not one the game has",
                   "an unknown span is refused");
    native_refused("{\"native\": [{\"id\": \"test.native\", \"bytes\": \"0011223G\"}]}", "not lower-case hex",
                   "a span that isn't hex is refused");
    write(story_dir(home) / "native.json", native_json);
    // A folder from before native.json (import version 2): missing, so imported again.
    fs::remove(story_dir(home) / "native.json");
    check(story_state(home, native) == StoryState::Missing, "no native.json: missing");
    check(thrown([&] { check(ensure_story(game, home, native), "ensure_story imports it"); }).empty(),
          "ensure_story writes native.json");
    check(read(story_dir(home) / "native.json") == native_json, "native.json as a fresh import's");
    check(thrown([&] { check(ensure_story(game, home, longer), "back to the longer table"); }).empty(),
          "ensure_story with the longer table again");

    // Another import version: outdated.
    std::string old = read(story_dir(home) / "import.json");
    old.replace(old.find(version), version.size(), "\"import_version\": 0");
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
    std::vector<ImportPiece> past = {{ImportPiece::Kind::Bytes, "test.past", "MW_MAIN.EXE", 0x0A00, 0, 4, "0000000000000000"}};
    write(game / "mw_main.exe", exe);
    check(thrown([&] { import_story(game, home, past); }).find("test.past") != std::string::npos,
          "a piece past the end is refused");

    // A write that fails blames the folder, not the copy, and changes nothing.
    check(thrown([&] { import_story(game, home, table); }).empty(), "a good import again");
    const std::string good_text = read(story_dir(home) / "text.json"), good_manifest = read(story_dir(home) / "import.json");
    check(!good_text.empty() && story_state(home, table) == StoryState::Current, "current again");
    const fs::path blocker = story_dir(home) / "events.json.part";  // a folder in the way: it can't be opened
    fs::create_directories(blocker / "x");
    const std::string blocked = thrown([&] { repair_story("a reason", game, home, table); });
    check(blocked.find("is damaged (a reason)") != std::string::npos && blocked.find("couldn't be written again") != std::string::npos &&
              blocked.find("events.json") != std::string::npos && blocked.find("good copy") == std::string::npos,
          "a folder that can't be written is named, not the copy: " + blocked);
    check(story_state(home, table) == StoryState::Current && read(story_dir(home) / "text.json") == good_text &&
              read(story_dir(home) / "import.json") == good_manifest, "a failed write leaves the story as it was");
    check(!fs::exists(story_dir(home) / "text.json.part"), "and none of its own files");
    fs::remove_all(blocker);
#if !defined(_WIN32)
    fs::permissions(story_dir(home), fs::perms::owner_read | fs::perms::owner_exec);
    const bool writable = static_cast<bool>(std::ofstream(story_dir(home) / "probe"));  // root
    const std::string read_only = thrown([&] { repair_story("a reason", game, home, table); });
    fs::permissions(story_dir(home), fs::perms::owner_all);
    check(writable || (read_only.find("can't write") != std::string::npos && read_only.find("good copy") == std::string::npos &&
                       story_state(home, table) == StoryState::Current), "a read-only folder is named: " + read_only);
#endif

    // An import keeps the files it replaces, one generation.
    write(story_dir(home) / "text.json", "edited");
    check(thrown([&] { repair_story("a reason", game, home, table); }).empty(), "a damaged story is imported");
    check(read(story_dir(home) / "text.json") == good_text && read(replaced_dir(home) / "text.json") == "edited" &&
              fs::exists(replaced_dir(home) / "import.json"), "the replaced files are kept");
    write(story_dir(home) / "text.json", "again");
    write(replaced_dir(home) / "older.json", "{}");
    check(thrown([&] { repair_story("a reason", game, home, table); }).empty(), "and again");
    check(read(replaced_dir(home) / "text.json") == "again" && !fs::exists(replaced_dir(home) / "older.json"), "one generation");
    // A start after an import that stopped half-way doesn't lose what that one kept.
    write(story_dir(home) / "text.json", "half");
    fs::remove(story_dir(home) / "import.json");
    check(thrown([&] { check(ensure_story(game, home, table), "a stopped import is missing"); }).empty(), "imported after a stop");
    check(read(replaced_dir(home) / "text.json") == "again" && read(story_dir(home) / "text.json") == good_text,
          "what the stopped import kept is still there");

    // A rename that fails half-way leaves no import.json, so the mixed files are not used.
    fs::remove(story_dir(home) / "events.json");
    fs::create_directories(story_dir(home) / "events.json" / "x");  // a file can't be renamed over it
    check(thrown([&] { repair_story("a reason", game, home, table); }).find("can't write") != std::string::npos &&
              !fs::exists(story_dir(home) / "import.json"), "a half-finished import leaves no import.json");
    fs::remove_all(story_dir(home) / "events.json");
    check(thrown([&] { check(ensure_story(game, home, table), "then it is missing"); }).empty() &&
              story_state(home, table) == StoryState::Current, "and the next start imports it");

    // Texts: one missing or holding a NUL can't be used; a shorter one can.
    StoryText texts = load_story_text(home);
    check(thrown([&] { check_texts(texts, table); }).empty(), "the texts are there");
    texts["test.second"] = "short";
    texts["test.extra"] = "extra";
    check(thrown([&] { check_texts(texts, table); }).empty(), "a text changed by hand stays");
    texts["test.second"] = std::string("a\0b", 3);
    check(thrown([&] { check_texts(texts, table); }).find("holds a NUL") != std::string::npos, "break: a NUL");
    texts.erase("test.second");
    check(thrown([&] { check_texts(texts, table); }).find("test.second") != std::string::npos, "break: a text missing");

    // No exe at all.
    fs::remove(game / "mw_main.exe");
    const std::string no_copy = thrown([&] { repair_story("a reason", game, home, table); });
    check(no_copy.find("MW_MAIN.EXE is missing") != std::string::npos && no_copy.find("good copy") != std::string::npos &&
              story_state(home, table) == StoryState::Current, "a copy without its program is blamed: " + no_copy);
    check(thrown([&] { import_story(game, home, table); }).find("MW_MAIN.EXE is missing") != std::string::npos,
          "a missing exe is named");

    // A broken text.json: the error names the file.
    write(story_dir(home) / "text.json", "{\"texts\": [{\"id\": \"a\", \"text\": \"x\"},]}");
    check(thrown([&] { load_story_text(home); }).find("text.json") != std::string::npos, "a broken text.json is named");
    write(story_dir(home) / "text.json",
          "{\"texts\": [{\"id\": \"a\", \"text\": \"x\"}, {\"id\": \"a\", \"text\": \"y\"}]}");
    check(thrown([&] { load_story_text(home); }).find("twice") != std::string::npos, "an id twice is refused");

    // A folder from before the AdLib instruments left the table (S9): current,
    // with no exe beside it, whatever its music.json is.
    const fs::path old_home = root / "old", no_game = root / "no_game";
    check(import_table_hash(import_table()) == kMusicRetired.table,
          "the pieces changed since kMusicRetired (src/Import/ImportTable.h): remove the pair and this block (older "
          "folders are then imported again, from the player's exes), or set the pair to the new table's hash only if an "
          "older folder really has everything the new table reads");
    for (const char* name : {"text.json", "words.json", "native.json", "pilots.json", "planets.json", "missions.json",
                             "event_text.json", "events.json"})
        write(story_dir(old_home) / name, "{}");
    const auto old_manifest = [&](const std::string& hash) {
        write(story_dir(old_home) / "import.json", "{" + version + ", \"table_sha256\": \"" + hash + "\"}");
    };
    const auto kept = [&] {
        return story_state(old_home) == StoryState::Current &&
               thrown([&] { check(!ensure_story(no_game, old_home), "an old folder is not imported again"); }).empty();
    };
    old_manifest(std::string(kMusicRetired.accepted));
    check(kept(), "an old folder without music.json");
    write(story_dir(old_home) / "music.json", "{\"adlib_instruments\": []}");
    check(kept(), "an old folder with its music.json");
    write(story_dir(old_home) / "music.json", "{broken");
    check(kept(), "an old folder with a broken music.json");
    check(read(story_dir(old_home) / "music.json") == "{broken", "the old music.json is left as it is");
    check(story_state(old_home, table) == StoryState::Outdated, "the old hash counts for the game's own table only");
    old_manifest(std::string(64, '0'));
    check(story_state(old_home) == StoryState::Outdated, "another table's hash: outdated");
    old_manifest(import_table_hash(import_table()));
    check(kept(), "the table's own hash: current");

    fs::remove_all(root);
    if (g_failures) {
        std::printf("story_test: %d failed\n", g_failures);
        return 1;
    }
    std::printf("story_test: all passed\n");
    return 0;
}
