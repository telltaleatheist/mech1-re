// The UI words (docs/phase2/PLAN.md section 7) on a made-up exe, no game
// files: HMAC-SHA-256 against RFC 4231; a word's hash, like every piece's, is
// keyed by its exe's key region, so a plain SHA-256 or another exe's key
// doesn't match; story/words.json keeps a string without its NUL and a block
// whole; load_words takes every word at its table length once and refuses
// anything else, naming the word. The words here are made up ("w1", "w22"):
// no real word is in a test source.
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

void write(const fs::path& path, const std::string& bytes) {
    fs::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary) << bytes;
}

std::string read(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

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
    // RFC 4231, test case 2.
    check(mw1o::support::hmac_sha256_hex(span_of("Jefe"), span_of("what do ya want for nothing?")) ==
              "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843",
          "HMAC-SHA-256 is RFC 4231's");

    const fs::path root = fs::current_path() / "words_test_files";
    fs::remove_all(root);
    const fs::path game = root / "game", home = root / "home";

    // An MZ exe with a two-paragraph header, its image loaded at 0800, long
    // enough to hold MW_MAIN's key region; the made-up words at 0801:0000.
    const std::string word = std::string("w1") + '\0';             // a string word
    const std::string block = std::string("w22\0\0x", 6);          // a block with NULs inside
    const std::string text = std::string("t") + '\0';              // a story text beside them
    const KeyRegion region = key_region("MW_MAIN.EXE");
    const std::size_t key_at = std::size_t(region.segment - kLoadSegment) * 16 + region.offset;
    std::string image = word + block + text;
    image.resize(key_at + region.length + 16, '\0');
    for (std::size_t i = 0; i < region.length; ++i) image[key_at + i] = static_cast<char>(i * 7 + 3);
    std::string exe(0x20, '\0');
    exe[0] = 'M';
    exe[1] = 'Z';
    exe[8] = 2;
    exe += std::string(0x10, '\0') + image;  // 0801:0000 is image byte 0x10
    write(game / "MW_MAIN.EXE", exe);

    const std::string full_image = exe.substr(0x20);
    const auto key_digest = mw1o::support::sha256_digest(span_of(full_image.substr(key_at, region.length)));
    const std::string key(reinterpret_cast<const char*>(key_digest.data()), key_digest.size());
    auto keyed = [&](const std::string& bytes) { return mw1o::support::hmac_sha256_hex(span_of(key), span_of(bytes)).substr(0, 16); };
    auto plain = [&](const std::string& bytes) { return mw1o::support::sha256_hex(span_of(bytes)).substr(0, 16); };
    const std::string word_hash = keyed(word), block_hash = keyed(block), text_hash = keyed(text);
    const std::string word_plain = plain(word);
    const auto at = [](std::size_t n) { return static_cast<std::uint16_t>(n); };
    const std::vector<ImportPiece> table = {
        {ImportPiece::Kind::Word, "test.word.w1", "MW_MAIN.EXE", 0x0801, 0, at(word.size()), word_hash},
        {ImportPiece::Kind::WordBytes, "test.word.w22", "MW_MAIN.EXE", 0x0801, at(word.size()), at(block.size()), block_hash},
        {ImportPiece::Kind::String, "test.text", "MW_MAIN.EXE", 0x0801, at(word.size() + block.size()), at(text.size()), text_hash},
    };

    check(thrown([&] { import_story(game, home, table); }).empty(), "a matching copy imports, words and all");
    check(story_state(home, table) == StoryState::Current, "after the import: current");
    Words words;
    check(thrown([&] { words = load_words(home, table); }).empty(), "words.json loads");
    check(words.size() == 2 && words["test.word.w1"] == word, "a string word keeps its NUL in memory");
    check(words["test.word.w22"] == block, "a block word is kept whole, NULs and all");
    check(load_story_text(home).count("test.text") == 1 && load_story_text(home).count("test.word.w1") == 0,
          "words go to words.json, not text.json");
    const std::string file = read(story_dir(home) / "words.json");
    check(file.find("\"address\": \"0801:0000\"") != std::string::npos, "words.json records the address");
    check(file.find("\\u0000") != std::string::npos, "a block's NULs are written as \\u0000");

    // words.json missing: the story counts as missing.
    fs::remove(story_dir(home) / "words.json");
    check(story_state(home, table) == StoryState::Missing, "no words.json: missing");

    // A word with a plain SHA-256: refused (a word's hash is keyed).
    std::vector<ImportPiece> unkeyed = table;
    unkeyed[0].sha16 = word_plain;
    std::string why = thrown([&] { import_story(game, home, unkeyed); });
    check(why.find("test.word.w1 (MW_MAIN.EXE 0801:0000)") != std::string::npos, "a plain hash on a word is refused: " + why);

    // Another key (a byte of the key region changed): every piece refused, the text too.
    std::string rekeyed = exe;
    rekeyed[0x20 + key_at + 5] ^= 1;
    write(game / "MW_MAIN.EXE", rekeyed);
    why = thrown([&] { import_story(game, home, table); });
    check(why.find("test.word.w1") != std::string::npos && why.find("test.word.w22") != std::string::npos,
          "another exe's key refuses the words: " + why);
    check(why.find("test.text") != std::string::npos && why.find("3 of 3") != std::string::npos,
          "another exe's key refuses the story text too: " + why);

    // A changed word: refused, naming it.
    std::string changed = exe;
    changed[0x30 + 1] ^= 1;
    write(game / "MW_MAIN.EXE", changed);
    why = thrown([&] { import_story(game, home, table); });
    check(why.find("test.word.w1") != std::string::npos && why.find("1 of 3") != std::string::npos,
          "a changed word is refused, named: " + why);
    write(game / "MW_MAIN.EXE", exe);
    check(thrown([&] { import_story(game, home, table); }).empty(), "the copy imports again");

    // Hand-edited words.json: each refused, naming the word.
    const fs::path path = story_dir(home) / "words.json";
    const std::string good = read(path);
    auto refused = [&](const std::string& contents, const std::string& expect, const std::string& what) {
        write(path, contents);
        const std::string e = thrown([&] { load_words(home, table); });
        check(e.find("words.json") != std::string::npos && e.find(expect) != std::string::npos, what + ": " + e);
    };
    auto entry = [](const std::string& id, const std::string& t) {
        return "{\"id\": \"" + id + "\", \"address\": \"0801:0000\", \"text\": \"" + t + "\"}";
    };
    const std::string w22 = entry("test.word.w22", "w22\\u0000\\u0000x");
    refused("{\"words\": [" + entry("test.word.w1", "w11") + ", " + w22 + "]}", "test.word.w1\" is 4 bytes, not 3",
            "a word longer than its table length");
    refused("{\"words\": [" + entry("test.word.w1", "w1") + ", " + entry("test.word.w22", "w22\\u0000\\u0000") + "]}",
            "test.word.w22\" is 5 bytes, not 6", "a block one byte short");
    refused("{\"words\": [" + entry("test.word.w1", "w1") + ", " + w22 + ", " + entry("test.word.w3", "w3") + "]}",
            "test.word.w3\" is not one the game has", "an unknown id");
    refused("{\"words\": [" + entry("test.word.w1", "w1") + "]}", "test.word.w22\" is missing", "a missing id");
    refused("{\"words\": [" + entry("test.word.w1", "w\\u00001") + ", " + w22 + "]}", "holds a NUL",
            "a NUL inside a string word");
    refused("{\"words\": [" + entry("test.word.w1", "w1") + ", " + entry("test.word.w1", "w1") + ", " + w22 + "]}",
            "twice", "a word twice");
    refused("{\"words\": [], \"extra\": 1}", "not {\"words\"", "another shape");
    write(path, good);
    check(thrown([&] { load_words(home, table); }).empty(), "the written words.json loads again");

    fs::remove_all(root);
    std::printf("words_test: %s\n", g_failures ? "FAILED" : "all passed");
    return g_failures ? 1 : 0;
}
