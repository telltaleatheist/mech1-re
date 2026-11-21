#include "Records.h"

#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>

#include "../Support/Json.h"

namespace mw1o::import {

namespace fs = std::filesystem;
using support::Json;
using support::json_quote;
using support::parse_json;

namespace {

constexpr std::size_t kPilotPrefix = 3, kPilotName = 11, kPlanetHeader = 10, kMissionHeader = 2;

std::string id_of(const char* kind, std::size_t n, const char* field) {
    return std::string(kind) + "." + std::to_string(n) + "." + field;
}

const PieceBytes* find_piece(const PieceMap& pieces, const std::string& id) {
    const auto it = pieces.find(id);
    return it == pieces.end() ? nullptr : &it->second;
}

const PieceBytes& need_piece(const PieceMap& pieces, const std::string& id, std::size_t size) {
    const PieceBytes* piece = find_piece(pieces, id);
    if (!piece || piece->bytes.size() != size)
        throw ImportError("the import table's piece " + id + " is missing or not " + std::to_string(size) + " bytes");
    return *piece;
}

std::uint8_t byte_at(const std::string& bytes, std::size_t at) { return static_cast<std::uint8_t>(bytes[at]); }
std::uint16_t word_at(const std::string& bytes, std::size_t at) {
    return static_cast<std::uint16_t>(byte_at(bytes, at) | byte_at(bytes, at + 1) << 8);
}

// The id (from 1) of the record of `kind` whose `field` piece starts at `offset`.
int record_at(const PieceMap& pieces, const char* kind, const char* field, std::size_t count, std::uint16_t offset) {
    for (std::size_t n = 1; n <= count; ++n)
        if (pieces.at(id_of(kind, n, field)).offset == offset) return static_cast<int>(n);
    throw ImportError(std::string("an offset table names no ") + kind + " record");
}

std::size_t count_of(const PieceMap& pieces, const char* kind, const char* field) {
    std::size_t n = 0;
    while (find_piece(pieces, id_of(kind, n + 1, field))) ++n;
    return n;
}

std::string ids(const std::vector<int>& list) {
    std::string out = "[";
    for (std::size_t i = 0; i < list.size(); ++i) out += (i ? ", " : "") + std::to_string(list[i]);
    return out + "]";
}

std::string read_text_file(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw ImportError("can't read " + path.string());
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

// Strict reading of one record file: the shape is checked, and every error
// names the file.
class Reader {
public:
    explicit Reader(fs::path path) : path_(std::move(path)) {}

    Json parse() {
        try {
            return parse_json(read_text_file(path_));
        } catch (const support::JsonError& e) {
            fail(e.what());
        }
    }
    [[noreturn]] void fail(const std::string& why) const { throw ImportError(path_.string() + ": " + why); }

    const Json& member(const Json& object, std::string_view key) const {
        if (!object.is_object()) fail("expected an object");
        const Json* found = object.find(key);
        if (!found) fail("no \"" + std::string(key) + "\"");
        return *found;
    }
    const Json::Array& array(const Json& object, std::string_view key) const {
        const Json& v = member(object, key);
        if (!v.is_array()) fail("\"" + std::string(key) + "\" is not an array");
        return v.array();
    }
    double whole(const Json& v, std::string_view what, double maximum) const {
        if (!v.is_number() || v.number() < 0 || v.number() > maximum || std::floor(v.number()) != v.number())
            fail(std::string(what) + " is not a whole number from 0 to " + std::to_string(static_cast<long>(maximum)));
        return v.number();
    }
    std::uint8_t u8(const Json& object, std::string_view key) const {
        return static_cast<std::uint8_t>(whole(member(object, key), key, 255));
    }
    std::uint16_t u16(const Json& object, std::string_view key) const {
        return static_cast<std::uint16_t>(whole(member(object, key), key, 65535));
    }
    std::string text(const Json& object, std::string_view key) const {
        const Json& v = member(object, key);
        if (!v.is_string() || v.string().empty()) fail("\"" + std::string(key) + "\" is not a text id");
        return v.string();
    }
    std::vector<int> id_list(const Json& list, std::size_t count) const {
        if (!list.is_array()) fail("expected a list of ids");
        std::vector<int> out;
        for (const Json& v : list.array()) {
            const int id = static_cast<int>(whole(v, "an id", 65535));
            if (id < 1 || static_cast<std::size_t>(id) > count) fail("the id " + std::to_string(id) + " names no record");
            out.push_back(id);
        }
        return out;
    }

private:
    fs::path path_;
};

const std::string& text_for(const StoryText& texts, const std::string& id) {
    const auto it = texts.find(id);
    if (it == texts.end()) throw ImportError("the story file has no text \"" + id + "\"");
    if (it->second.find('\0') != std::string::npos) throw ImportError("the story text \"" + id + "\" holds a NUL");
    return it->second;
}

// Appends bytes to a storage being rebuilt; refuses to run past its end.
class Writer {
public:
    Writer(std::span<std::uint8_t> storage, std::uint16_t base, const char* what)
        : storage_(storage), base_(base), what_(what) {}

    std::uint16_t here() const { return static_cast<std::uint16_t>(base_ + at_); }
    void byte(std::uint8_t b) {
        room(1);
        storage_[at_++] = b;
    }
    void bytes(const std::string& text, std::size_t field) {  // `field` bytes, NUL-padded; text must leave one NUL
        if (text.size() + 1 > field) throw ImportError(std::string(what_) + ": a text is longer than its field");
        room(field);
        std::memcpy(storage_.data() + at_, text.data(), text.size());
        std::memset(storage_.data() + at_ + text.size(), 0, field - text.size());
        at_ += field;
    }
    void string(const std::string& text) { bytes(text, text.size() + 1); }
    void finish() { std::memset(storage_.data() + at_, 0, storage_.size() - at_); }

private:
    void room(std::size_t n) const {
        if (at_ + n > storage_.size()) throw ImportError(std::string(what_) + " don't fit their storage");
    }
    std::span<std::uint8_t> storage_;
    std::uint16_t base_;
    const char* what_;
    std::size_t at_ = 0;
};

}  // namespace

// ---- import: the files from the pieces ----

std::string pilots_json(const PieceMap& pieces) {
    const std::size_t count = count_of(pieces, "pilot", "prefix");
    std::string out = "{\n  \"pilots\": [";
    for (std::size_t n = 1; n <= count; ++n) {
        const auto& prefix = need_piece(pieces, id_of("pilot", n, "prefix"), kPilotPrefix).bytes;
        out += std::string(n > 1 ? "," : "") + "\n    {\"portrait\": " + std::to_string(byte_at(prefix, 0)) +
               ", \"gunnery\": " + std::to_string(byte_at(prefix, 1)) +
               ", \"piloting\": " + std::to_string(byte_at(prefix, 2)) +
               ", \"name\": " + json_quote(id_of("pilot", n, "name")) +
               ", \"dialogue\": " + json_quote(id_of("pilot", n, "dialogue")) + "}";
    }
    return out + (count ? "\n  " : "") + "]\n}\n";
}

std::string planets_json(const PieceMap& pieces) {
    const std::size_t count = count_of(pieces, "planet", "header");
    std::string out = "{\n  \"planets\": [";
    for (std::size_t n = 1; n <= count; ++n) {
        const auto& h = need_piece(pieces, id_of("planet", n, "header"), kPlanetHeader).bytes;
        out += std::string(n > 1 ? "," : "") + "\n    {\"number\": " + std::to_string(byte_at(h, 0)) +
               ", \"house\": " + std::to_string(byte_at(h, 1)) +
               ", \"landscape_and_market\": " + std::to_string(byte_at(h, 2)) +
               ", \"maximum_mission_type\": " + std::to_string(byte_at(h, 3)) +
               ", \"facility_display\": " + std::to_string(byte_at(h, 4)) +
               ", \"map_x\": " + std::to_string(byte_at(h, 5)) + ", \"map_y\": " + std::to_string(byte_at(h, 6)) +
               ", \"travel_time_weight\": " + std::to_string(byte_at(h, 7)) +
               ", \"population\": " + std::to_string(word_at(h, 8)) +
               ", \"name\": " + json_quote(id_of("planet", n, "name")) +
               ", \"description\": " + json_quote(id_of("planet", n, "description")) + "}";
    }
    std::vector<int> alphabetical;
    if (count) {
        // The offset table must list the planets in id order; the alphabetical
        // table's offsets become planet ids.
        const auto& offsets = need_piece(pieces, "planets.offsets", count * 2).bytes;
        for (std::size_t n = 1; n <= count; ++n)
            if (word_at(offsets, (n - 1) * 2) != pieces.at(id_of("planet", n, "header")).offset)
                throw ImportError("planets.offsets doesn't list the planets in order");
        const PieceBytes* alpha = find_piece(pieces, "planets.alphabetical");
        if (!alpha || alpha->bytes.size() % 2) throw ImportError("the import table's piece planets.alphabetical is missing");
        for (std::size_t at = 0; at < alpha->bytes.size(); at += 2)
            alphabetical.push_back(record_at(pieces, "planet", "header", count, word_at(alpha->bytes, at)));
    }
    return out + (count ? "\n  " : "") + "],\n  \"alphabetical\": " + ids(alphabetical) + "\n}\n";
}

std::string missions_json(const PieceMap& pieces) {
    const std::size_t count = count_of(pieces, "mission", "header");
    std::string out = "{\n  \"missions\": [";
    for (std::size_t n = 1; n <= count; ++n) {
        const auto& h = need_piece(pieces, id_of("mission", n, "header"), kMissionHeader).bytes;
        out += std::string(n > 1 ? "," : "") + "\n    {\"scenario\": " + std::to_string(byte_at(h, 0)) +
               ", \"unidentified_2\": " + std::to_string(byte_at(h, 1)) +
               ", \"text\": " + json_quote(id_of("mission", n, "text")) + "}";
    }
    std::string categories = "[]";
    if (count) {
        const auto& entries = need_piece(pieces, "missions.entries", count * 2);
        for (std::size_t n = 1; n <= count; ++n)
            if (word_at(entries.bytes, (n - 1) * 2) != pieces.at(id_of("mission", n, "header")).offset)
                throw ImportError("missions.entries doesn't list the missions in order");
        const auto& maxima = find_piece(pieces, "missions.maximum_indices");
        const auto& lists = find_piece(pieces, "missions.list_offsets");
        if (!maxima || !lists || lists->bytes.size() != maxima->bytes.size() * 2)
            throw ImportError("the import table's missions.maximum_indices and missions.list_offsets don't agree");
        categories = "[";
        for (std::size_t c = 0; c < maxima->bytes.size(); ++c) {
            const std::uint16_t list = word_at(lists->bytes, c * 2);
            std::vector<int> run;
            if (list) {  // 0: no list (the exe's empty category)
                if (list < entries.offset || (list - entries.offset) % 2) throw ImportError("a mission list offset is outside the entries");
                const std::size_t first = (list - entries.offset) / 2;
                const std::size_t size = std::size_t(byte_at(maxima->bytes, c)) + 1;
                if (first + size > count) throw ImportError("a mission category runs past the entries");
                for (std::size_t i = 0; i < size; ++i) run.push_back(static_cast<int>(first + i + 1));
            } else if (byte_at(maxima->bytes, c) != 0) {
                throw ImportError("a mission category without a list has a maximum index");
            }
            categories += (c ? ", " : "") + ids(run);
        }
        categories += "]";
    }
    return out + (count ? "\n  " : "") + "],\n  \"categories\": " + categories + "\n}\n";
}

// ---- the game: the files, read strictly ----

std::vector<PilotRecord> load_pilots(const fs::path& write_dir) {
    Reader r(story_dir(write_dir) / "pilots.json");
    const Json file = r.parse();
    std::vector<PilotRecord> out;
    for (const Json& p : r.array(file, "pilots"))
        out.push_back({r.u8(p, "portrait"), r.u8(p, "gunnery"), r.u8(p, "piloting"), r.text(p, "name"), r.text(p, "dialogue")});
    return out;
}

Planets load_planets(const fs::path& write_dir) {
    Reader r(story_dir(write_dir) / "planets.json");
    const Json file = r.parse();
    Planets out;
    for (const Json& p : r.array(file, "planets"))
        out.planets.push_back({r.u8(p, "number"), r.u8(p, "house"), r.u8(p, "landscape_and_market"),
                               r.u8(p, "maximum_mission_type"), r.u8(p, "facility_display"), r.u8(p, "map_x"),
                               r.u8(p, "map_y"), r.u8(p, "travel_time_weight"), r.u16(p, "population"),
                               r.text(p, "name"), r.text(p, "description")});
    out.alphabetical = r.id_list(r.member(file, "alphabetical"), out.planets.size());
    return out;
}

Missions load_missions(const fs::path& write_dir) {
    Reader r(story_dir(write_dir) / "missions.json");
    const Json file = r.parse();
    Missions out;
    for (const Json& m : r.array(file, "missions"))
        out.missions.push_back({r.u8(m, "scenario"), r.u8(m, "unidentified_2"), r.text(m, "text")});
    for (const Json& c : r.array(file, "categories")) {
        out.categories.push_back(r.id_list(c, out.missions.size()));
        const auto& run = out.categories.back();
        for (std::size_t i = 1; i < run.size(); ++i)
            if (run[i] != run[i - 1] + 1) r.fail("a category's missions are not consecutive");
        if (run.size() > 256) r.fail("a category has more than 256 missions");
    }
    return out;
}

// ---- the game: the original's storage, rebuilt ----

void build_pilots(const std::vector<PilotRecord>& pilots, const StoryText& texts, std::span<std::uint8_t> storage,
                  std::uint16_t base, std::span<std::uint16_t> offsets) {
    if (offsets.size() != pilots.size() + 1)
        throw ImportError("the story has " + std::to_string(pilots.size()) + " pilots, the game " +
                          std::to_string(offsets.size() - 1));
    Writer w(storage, base, "the pilots");
    offsets[0] = 0;
    for (std::size_t i = 0; i < pilots.size(); ++i) {
        const PilotRecord& p = pilots[i];
        offsets[i + 1] = w.here();
        w.byte(p.portrait);
        w.byte(p.gunnery);
        w.byte(p.piloting);
        w.bytes(text_for(texts, p.name), kPilotName);
        w.string(text_for(texts, p.dialogue));
    }
    w.finish();
}

void build_planets(const Planets& planets, const StoryText& texts, std::span<std::uint8_t> storage,
                   std::uint16_t base, std::span<std::uint16_t> offsets, std::span<std::uint16_t> alphabetical) {
    if (offsets.size() != planets.planets.size() || alphabetical.size() != planets.alphabetical.size())
        throw ImportError("the story has " + std::to_string(planets.planets.size()) + " planets (" +
                          std::to_string(planets.alphabetical.size()) + " listed), the game " +
                          std::to_string(offsets.size()) + " (" + std::to_string(alphabetical.size()) + ")");
    Writer w(storage, base, "the planets");
    for (std::size_t i = 0; i < planets.planets.size(); ++i) {
        const PlanetRecord& p = planets.planets[i];
        offsets[i] = w.here();
        for (std::uint8_t b : {p.number, p.house, p.landscape_and_market, p.maximum_mission_type, p.facility_display,
                               p.map_x, p.map_y, p.travel_time_weight})
            w.byte(b);
        w.byte(static_cast<std::uint8_t>(p.population));
        w.byte(static_cast<std::uint8_t>(p.population >> 8));
        w.string(text_for(texts, p.name));
        w.string(text_for(texts, p.description));
    }
    w.finish();
    for (std::size_t i = 0; i < planets.alphabetical.size(); ++i)
        alphabetical[i] = offsets[std::size_t(planets.alphabetical[i] - 1)];
}

void build_missions(const Missions& missions, const StoryText& texts, std::span<std::uint8_t> storage,
                    std::uint16_t base, std::span<std::uint8_t> maximum_indices, std::span<std::uint16_t> list_offsets,
                    std::span<std::uint16_t> entries, std::uint16_t entry_base) {
    if (entries.size() != missions.missions.size() || maximum_indices.size() != missions.categories.size() ||
        list_offsets.size() != missions.categories.size())
        throw ImportError("the story has " + std::to_string(missions.missions.size()) + " missions in " +
                          std::to_string(missions.categories.size()) + " categories, the game " +
                          std::to_string(entries.size()) + " in " + std::to_string(maximum_indices.size()));
    Writer w(storage, base, "the mission descriptions");
    for (std::size_t i = 0; i < missions.missions.size(); ++i) {
        const MissionRecord& m = missions.missions[i];
        entries[i] = w.here();
        w.byte(m.scenario);
        w.byte(m.unidentified_2);
        w.string(text_for(texts, m.text));
    }
    w.finish();
    for (std::size_t c = 0; c < missions.categories.size(); ++c) {
        const auto& run = missions.categories[c];
        maximum_indices[c] = run.empty() ? 0 : static_cast<std::uint8_t>(run.size() - 1);
        list_offsets[c] = run.empty() ? 0 : static_cast<std::uint16_t>(entry_base + (run.front() - 1) * 2);
    }
}

}  // namespace mw1o::import
