#include "Story.h"

#include "Events.h"
#include "Music.h"
#include "Records.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <set>
#include <vector>

#include "../Support/Hmac.h"
#include "../Support/Json.h"
#include "../Support/Sha256.h"
#include "mechwarrior/combat_installation.h"

namespace mw1o::import {

namespace fs = std::filesystem;
using support::json_quote;
using support::parse_json;
using support::sha256_hex;

namespace {

fs::path manifest_path(const fs::path& write_dir) { return story_dir(write_dir) / "import.json"; }
fs::path text_path(const fs::path& write_dir) { return story_dir(write_dir) / "text.json"; }
fs::path words_path(const fs::path& write_dir) { return story_dir(write_dir) / "words.json"; }
fs::path native_path(const fs::path& write_dir) { return story_dir(write_dir) / "native.json"; }

std::string hex_of(std::string_view bytes) {
    static const char digits[] = "0123456789abcdef";
    std::string out;
    for (unsigned char c : bytes) {
        out += digits[c >> 4];
        out += digits[c & 15];
    }
    return out;
}
// The typed records (Records.h), beside the text.
constexpr const char* kRecordFiles[] = {"pilots.json", "planets.json", "missions.json", "event_text.json",
                                        "events.json", "music.json"};

std::vector<std::uint8_t> read_file(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw ImportError("can't read " + path.string());
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (in.bad()) throw ImportError("can't read " + path.string());
    return bytes;
}

// The temporary file is renamed over the old one, so a reader never sees half a file.
void write_file(const fs::path& path, const std::string& contents) {
    fs::create_directories(path.parent_path());
    const fs::path temp = path.string() + ".part";
    {
        std::ofstream out(temp, std::ios::binary);
        out << contents;
        out.close();
        if (!out) throw ImportError("can't write " + temp.string());
    }
    fs::rename(temp, path);
}

// The game folder's own spelling of `name` (DOS names are any case on disk).
fs::path find_game_file(const fs::path& game_dir, std::string_view name) {
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(game_dir, ec)) {
        std::string found = entry.path().filename().string();
        if (found.size() != name.size()) continue;
        bool same = true;
        for (std::size_t i = 0; i < found.size() && same; ++i) {
            const auto upper = [](char c) { return (c >= 'a' && c <= 'z') ? char(c - 'a' + 'A') : c; };
            same = upper(found[i]) == upper(name[i]);
        }
        if (same) return entry.path();
    }
    throw ImportError(std::string(name) + " is missing from " + game_dir.string());
}

struct Exe {
    std::vector<std::uint8_t> file;
    std::vector<std::uint8_t> image;  // the load image, as DOS loads it at load_segment()
    std::string revision;             // BTECH's, by name; empty for the others
};

static_assert(kBtechLoadSegment == BTECH_IMAGE_LOAD_SELECTOR, "BTECH's image loads where the game decodes it to");

Exe load_exe(const fs::path& path, std::string_view name) {
    Exe exe{read_file(path), {}, {}};
    if (exe.file.size() < 0x1C || exe.file[0] != 'M' || exe.file[1] != 'Z')
        throw ImportError(path.string() + " is not a DOS program");
    if (name == kBtechExe) {
        std::uint8_t* image = nullptr;
        BtechRevision revision{};
        if (!BtechDecodeImage(exe.file.data(), exe.file.size(), &image, &revision))
            throw ImportError(path.string() + " is not a BTECH.EXE this game knows (by size and checksum)");
        exe.image.assign(image, image + BTECH_IMAGE_BYTES);
        exe.revision = BtechRevisionName(revision);
        std::free(image);
    } else {
        const std::size_t header = std::size_t(exe.file[8] | exe.file[9] << 8) * 16;
        if (header > exe.file.size()) throw ImportError(path.string() + " is not a DOS program");
        exe.image.assign(exe.file.begin() + std::ptrdiff_t(header), exe.file.end());
    }
    return exe;
}

std::string_view bytes_view(const std::vector<std::uint8_t>& bytes, std::size_t at, std::size_t size) {
    return {reinterpret_cast<const char*>(bytes.data()) + at, size};
}

std::string sha256_of(std::string_view bytes) {
    return sha256_hex({reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size()});
}

std::span<const std::uint8_t> span_of(std::string_view bytes) {
    return {reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size()};
}

// The key of `exe`'s piece hashes: the SHA-256 of its key region, or empty if
// the image is too short to hold it (then no piece matches).
std::string piece_key(const Exe& exe, std::string_view name) {
    const KeyRegion region = key_region(name);
    const std::uint16_t base = load_segment(name);
    const std::size_t at = std::size_t(region.segment - base) * 16 + region.offset;
    if (region.segment < base || at + region.length > exe.image.size()) return {};
    const support::Digest key = support::sha256_digest(span_of(bytes_view(exe.image, at, region.length)));
    return std::string(reinterpret_cast<const char*>(key.data()), key.size());
}

// A piece's hash as the table keeps it: HMAC-SHA-256 under its exe's key.
std::string piece_hash(std::string_view bytes, const std::string& key) {
    if (key.empty()) return {};
    return support::hmac_sha256_hex(span_of(key), span_of(bytes));
}

}  // namespace

std::string piece_address(const ImportPiece& piece) {
    char text[16];
    std::snprintf(text, sizeof text, "%04X:%04X", piece.segment, piece.offset);
    return text;
}

std::string import_table_hash(std::span<const ImportPiece> table) {
    std::string all;
    for (const auto& piece : table) {
        all += std::to_string(static_cast<int>(piece.kind)) + ' ';
        all += piece.id;
        all += ' ';
        all += piece.exe;
        all += ' ' + piece_address(piece) + ' ' + std::to_string(piece.length) + ' ';
        all += piece.sha16;
        all += '\n';
    }
    return sha256_of(all);
}

fs::path story_dir(const fs::path& write_dir) { return write_dir / "story"; }

StoryState story_state(const fs::path& write_dir, std::span<const ImportPiece> table) {
    std::error_code ec;
    if (!fs::exists(manifest_path(write_dir), ec) || !fs::exists(text_path(write_dir), ec) ||
        !fs::exists(words_path(write_dir), ec) || !fs::exists(native_path(write_dir), ec))
        return StoryState::Missing;
    for (const char* name : kRecordFiles)
        if (!fs::exists(story_dir(write_dir) / name, ec)) return StoryState::Missing;
    try {
        const auto bytes = read_file(manifest_path(write_dir));
        const auto manifest = parse_json(bytes_view(bytes, 0, bytes.size()));
        const auto* version = manifest.find("import_version");
        if (!version || version->number() != kImportVersion) return StoryState::Outdated;
        const auto* hash = manifest.find("table_sha256");
        if (!hash || hash->string() != import_table_hash(table)) return StoryState::Outdated;
    } catch (const std::exception&) {
        return StoryState::Outdated;
    }
    return StoryState::Current;
}

void import_story(const fs::path& game_dir, const fs::path& write_dir, std::span<const ImportPiece> table) {
    std::map<std::string, Exe, std::less<>> exes;
    std::map<std::string, std::string, std::less<>> keys;
    std::vector<std::string> refused;
    std::string texts, words, natives;
    PieceMap pieces;
    for (const auto& piece : table) {
        auto it = exes.find(piece.exe);
        if (it == exes.end())
            it = exes.emplace(std::string(piece.exe), load_exe(find_game_file(game_dir, piece.exe), piece.exe)).first;
        const Exe& exe = it->second;
        auto key = keys.find(piece.exe);
        if (key == keys.end()) key = keys.emplace(std::string(piece.exe), piece_key(exe, piece.exe)).first;
        const std::uint16_t base = load_segment(piece.exe);
        const std::size_t at = std::size_t(piece.segment - base) * 16 + piece.offset;
        const bool inside = piece.segment >= base && at + piece.length <= exe.image.size();
        const std::string_view bytes = inside ? bytes_view(exe.image, at, piece.length) : std::string_view();
        const std::string hash = inside ? piece_hash(bytes, key->second) : std::string();
        if (!inside || hash.size() < 16 || hash.substr(0, 16) != piece.sha16) {
            refused.push_back(std::string(piece.id) + " (" + std::string(piece.exe) + " " + piece_address(piece) + ")");
            continue;
        }
        if (is_word(piece.kind)) {  // a string's text without its NUL; a block whole, its NULs as \u0000
            if (!words.empty()) words += ",\n";
            words += "    {\"id\": " + json_quote(piece.id) + ", \"address\": " + json_quote(piece_address(piece)) +
                     ", \"text\": " + json_quote(is_string(piece.kind) ? bytes.substr(0, bytes.size() - 1) : bytes) + "}";
            continue;
        }
        if (is_native(piece.kind)) {
            if (!natives.empty()) natives += ",\n";
            natives += "    {\"id\": " + json_quote(piece.id) + ", \"address\": " + json_quote(piece_address(piece)) +
                       ", \"bytes\": " + json_quote(hex_of(bytes)) + "}";
            continue;
        }
        pieces[std::string(piece.id)] = PieceBytes{piece.offset, std::string(bytes)};
        if (piece.kind != ImportPiece::Kind::String) continue;
        if (!texts.empty()) texts += ",\n";
        texts += "    {\"id\": " + json_quote(piece.id) + ", \"address\": " + json_quote(piece_address(piece)) +
                 ", \"text\": " + json_quote(bytes.substr(0, bytes.size() - 1)) + "}";
    }
    if (!refused.empty()) {
        std::string list;
        for (const auto& name : refused) list += "\n  " + name;
        throw ImportError("the game folder " + game_dir.string() + " is not the " + std::string(kRevision) +
                          " MechWarrior, or is damaged: " + std::to_string(refused.size()) + " of " +
                          std::to_string(table.size()) + " pieces differ:" + list);
    }

    // The manifest goes last: a folder without one has no finished import.
    std::error_code ec;
    fs::remove(manifest_path(write_dir), ec);
    write_file(text_path(write_dir), "{\n  \"texts\": [\n" + texts + "\n  ]\n}\n");
    write_file(words_path(write_dir), "{\n  \"words\": [\n" + words + "\n  ]\n}\n");
    write_file(native_path(write_dir), "{\n  \"native\": [\n" + natives + "\n  ]\n}\n");
    write_file(story_dir(write_dir) / kRecordFiles[0], pilots_json(pieces));
    write_file(story_dir(write_dir) / kRecordFiles[1], planets_json(pieces));
    write_file(story_dir(write_dir) / kRecordFiles[2], missions_json(pieces));
    write_file(story_dir(write_dir) / kRecordFiles[3], event_text_json(pieces));
    write_file(story_dir(write_dir) / kRecordFiles[4], events_json(pieces));
    write_file(story_dir(write_dir) / kRecordFiles[5], music_json(pieces));
    std::string sources, btech_revision;
    for (const auto& [name, exe] : exes) {
        if (!sources.empty()) sources += ", ";
        sources += json_quote(name) + ": " + json_quote(sha256_of(bytes_view(exe.file, 0, exe.file.size())));
        if (name == kBtechExe) btech_revision = exe.revision;
    }
    write_file(manifest_path(write_dir),
               "{\n  \"import_version\": " + std::to_string(kImportVersion) + ",\n  \"sources\": {" + sources +
                   "},\n  \"revision\": " + json_quote(kRevision) +
                   (btech_revision.empty() ? std::string() : ",\n  \"btech_revision\": " + json_quote(btech_revision)) +
                   ",\n  \"table_sha256\": " +
                   json_quote(import_table_hash(table)) + "\n}\n");
}

bool ensure_story(const fs::path& game_dir, const fs::path& write_dir, std::span<const ImportPiece> table) {
    const auto state = story_state(write_dir, table);
    if (state == StoryState::Current) return false;
    try {
        import_story(game_dir, write_dir, table);
    } catch (const std::exception& e) {
        throw ImportError(std::string("the story file in ") + story_dir(write_dir).string() + " is " +
                          (state == StoryState::Missing ? "missing" : "from an older version") +
                          ", and importing it from your copy of the game failed: " + e.what());
    }
    return true;
}

Words load_words(const fs::path& write_dir, std::span<const ImportPiece> table) {
    const fs::path path = words_path(write_dir);
    try {
        const auto bytes = read_file(path);
        const auto file = parse_json(bytes_view(bytes, 0, bytes.size()));
        if (file.object().size() != 1 || !file.find("words")) throw ImportError("it is not {\"words\": [...]}");
        Words out;
        for (const auto& entry : file.find("words")->array()) {
            const auto* id = entry.find("id");
            const auto* text = entry.find("text");
            if (!id || !text) throw ImportError("a word without an \"id\" or a \"text\"");
            const ImportPiece* piece = nullptr;
            for (const auto& p : table)
                if (is_word(p.kind) && p.id == id->string()) piece = &p;
            if (!piece) throw ImportError("the word " + json_quote(id->string()) + " is not one the game has");
            std::string word = text->string();
            if (is_string(piece->kind)) {
                if (word.find('\0') != std::string::npos)
                    throw ImportError("the word " + json_quote(id->string()) + " holds a NUL");
                word += '\0';
            }
            if (word.size() != piece->length)
                throw ImportError("the word " + json_quote(id->string()) + " is " + std::to_string(word.size()) +
                                  " bytes, not " + std::to_string(piece->length));
            if (!out.emplace(id->string(), std::move(word)).second)
                throw ImportError("the word " + json_quote(id->string()) + " twice");
        }
        for (const auto& p : table)
            if (is_word(p.kind) && !out.count(p.id)) throw ImportError("the word " + json_quote(p.id) + " is missing");
        return out;
    } catch (const std::exception& e) {
        throw ImportError(path.string() + ": " + e.what());
    }
}

Native load_native(const fs::path& write_dir, std::span<const ImportPiece> table) {
    const fs::path path = native_path(write_dir);
    try {
        const auto bytes = read_file(path);
        const auto file = parse_json(bytes_view(bytes, 0, bytes.size()));
        if (file.object().size() != 1 || !file.find("native")) throw ImportError("it is not {\"native\": [...]}");
        Native out;
        for (const auto& entry : file.find("native")->array()) {
            const auto* id = entry.find("id");
            const auto* hex = entry.find("bytes");
            if (!id || !hex) throw ImportError("a span without an \"id\" or \"bytes\"");
            const ImportPiece* piece = nullptr;
            for (const auto& p : table)
                if (is_native(p.kind) && p.id == id->string()) piece = &p;
            if (!piece) throw ImportError("the span " + json_quote(id->string()) + " is not one the game has");
            const std::string& digits = hex->string();
            const auto value = [&](char c) -> int {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                throw ImportError("the span " + json_quote(id->string()) + " is not lower-case hex");
            };
            if (digits.size() != std::size_t(piece->length) * 2)
                throw ImportError("the span " + json_quote(id->string()) + " is " + std::to_string(digits.size() / 2) +
                                  " bytes, not " + std::to_string(piece->length));
            std::string span;
            for (std::size_t i = 0; i < digits.size(); i += 2) span += char(value(digits[i]) << 4 | value(digits[i + 1]));
            if (!out.emplace(id->string(), std::move(span)).second)
                throw ImportError("the span " + json_quote(id->string()) + " twice");
        }
        for (const auto& p : table)
            if (is_native(p.kind) && !out.count(p.id)) throw ImportError("the span " + json_quote(p.id) + " is missing");
        return out;
    } catch (const std::exception& e) {
        throw ImportError(path.string() + ": " + e.what());
    }
}

std::string load_btech_revision(const fs::path& write_dir) {
    const fs::path path = manifest_path(write_dir);
    try {
        const auto bytes = read_file(path);
        const auto manifest = parse_json(bytes_view(bytes, 0, bytes.size()));
        const auto* revision = manifest.find("btech_revision");
        return revision ? revision->string() : std::string();
    } catch (const std::exception& e) {
        throw ImportError(path.string() + ": " + e.what());
    }
}

StoryText load_story_text(const fs::path& write_dir) {
    const fs::path path = text_path(write_dir);
    try {
        const auto bytes = read_file(path);
        const auto file = parse_json(bytes_view(bytes, 0, bytes.size()));
        const auto* texts = file.find("texts");
        if (!texts) throw ImportError("it has no \"texts\"");
        StoryText out;
        for (const auto& entry : texts->array()) {
            const auto* id = entry.find("id");
            const auto* text = entry.find("text");
            if (!id || !text) throw ImportError("a text without an \"id\" or a \"text\"");
            if (!out.emplace(id->string(), text->string()).second)
                throw ImportError("the id " + json_quote(id->string()) + " twice");
        }
        return out;
    } catch (const std::exception& e) {
        throw ImportError(path.string() + ": " + e.what());
    }
}

}  // namespace mw1o::import
