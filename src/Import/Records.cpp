#include "Records.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>

#include "../Support/Json.h"
#include "StrictReader.h"

namespace mw1o::import {

namespace fs = std::filesystem;
using support::Json;
using support::json_quote;
using support::parse_json;
using detail::Reader;

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

// ---- the event text bank ----

std::string event_text_json(const PieceMap& pieces) {
    struct Page {
        std::string key;
        std::size_t number;
        const PieceBytes* header;
    };
    const std::string prefix = "event_text.", suffix = ".header";
    std::vector<Page> pages;
    for (const auto& [id, piece] : pieces) {
        if (id.size() <= prefix.size() + suffix.size() || id.compare(0, prefix.size(), prefix) != 0 ||
            id.compare(id.size() - suffix.size(), suffix.size(), suffix) != 0)
            continue;
        const std::string middle = id.substr(prefix.size(), id.size() - prefix.size() - suffix.size());
        const auto dot = middle.rfind('.');
        if (dot == std::string::npos || dot + 1 == middle.size() ||
            middle.find_first_not_of("0123456789", dot + 1) != std::string::npos)
            throw ImportError("the import table's piece " + id + " is not event_text.<key>.<page>.header");
        pages.push_back({middle.substr(0, dot), std::stoul(middle.substr(dot + 1)), &piece});
    }
    if (pages.empty()) return "{\n  \"records\": []\n}\n";
    std::sort(pages.begin(), pages.end(), [](const Page& a, const Page& b) { return a.header->offset < b.header->offset; });

    // Records: runs of one key's pages, numbered from 1, in storage order.
    struct Record {
        std::string key;
        std::vector<const Page*> pages;
        int directory = -1;
    };
    std::vector<Record> records;
    for (const Page& page : pages) {
        if (records.empty() || records.back().key != page.key) {
            for (const Record& r : records)
                if (r.key == page.key) throw ImportError("the event text " + page.key + "'s pages are not together");
            records.push_back({page.key, {}});
        }
        records.back().pages.push_back(&page);
        if (page.number != records.back().pages.size())
            throw ImportError("the event text " + page.key + "'s pages are out of order");
        const std::string& h = page.header->bytes;
        if (!(h.size() == 2 || (h.size() == 3 && byte_at(h, 0) == 0xFF)))
            throw ImportError("an event text page header is not [FF] format rows");
    }
    const PieceBytes* directory = find_piece(pieces, "event_text.directory");
    if (!directory || directory->bytes.size() % 2)
        throw ImportError("the import table's piece event_text.directory is missing");
    for (std::size_t i = 0; i < directory->bytes.size() / 2; ++i) {
        const std::uint16_t at = word_at(directory->bytes, i * 2);
        if (!at) continue;
        auto it = std::find_if(records.begin(), records.end(), [&](const Record& r) { return r.pages[0]->header->offset == at; });
        if (it == records.end()) throw ImportError("event text directory entry " + std::to_string(i) + " names no record");
        if (it->key != std::to_string(i) || it->directory >= 0)
            throw ImportError("event text directory entry " + std::to_string(i) + " doesn't match its record's key");
        it->directory = static_cast<int>(i);
    }
    std::string out = "{\n  \"records\": [";
    for (std::size_t r = 0; r < records.size(); ++r) {
        const Record& record = records[r];
        if (record.directory < 0 && record.key.find_first_not_of("0123456789") == std::string::npos)
            throw ImportError("the event text " + record.key + " has no directory entry");
        out += std::string(r ? "," : "") + "\n    {\"directory\": " +
               (record.directory < 0 ? std::string("null") : std::to_string(record.directory)) + ", \"pages\": [";
        for (std::size_t p = 0; p < record.pages.size(); ++p) {
            const std::string& h = record.pages[p]->header->bytes;
            const bool marker = h.size() == 3;
            out += std::string(p ? ", " : "") + "{\"marker\": " + (marker ? "true" : "false") +
                   ", \"format\": " + std::to_string(byte_at(h, marker ? 1 : 0)) +
                   ", \"rows\": " + std::to_string(byte_at(h, marker ? 2 : 1)) + ", \"text\": " +
                   json_quote("event_text." + record.key + "." + std::to_string(p + 1) + ".text") + "}";
        }
        out += "]}";
    }
    return out + "\n  ]\n}\n";
}

std::vector<EventTextRecord> load_event_text(const fs::path& write_dir) {
    Reader r(story_dir(write_dir) / "event_text.json");
    const Json file = r.parse();
    std::vector<EventTextRecord> out;
    std::vector<bool> used;
    for (const Json& record : r.array(file, "records")) {
        EventTextRecord rec;
        const Json& directory = r.member(record, "directory");
        if (!std::holds_alternative<std::nullptr_t>(directory.value)) {
            const int index = static_cast<int>(r.whole(directory, "directory", 65535));
            if (std::size_t(index) >= used.size()) used.resize(std::size_t(index) + 1);
            if (used[std::size_t(index)]) r.fail("two records have the directory entry " + std::to_string(index));
            used[std::size_t(index)] = true;
            rec.directory = index;
        }
        for (const Json& page : r.array(record, "pages")) {
            const Json& marker = r.member(page, "marker");
            if (!std::holds_alternative<bool>(marker.value)) r.fail("\"marker\" is not true or false");
            rec.pages.push_back({std::get<bool>(marker.value), r.u8(page, "format"), r.u8(page, "rows"), r.text(page, "text")});
        }
        if (rec.pages.empty()) r.fail("a record has no pages");
        out.push_back(std::move(rec));
    }
    return out;
}

void build_event_text(const std::vector<EventTextRecord>& records, const StoryText& texts,
                      std::span<std::uint8_t> storage, std::uint16_t base, std::span<std::uint16_t> directory) {
    std::fill(directory.begin(), directory.end(), std::uint16_t(0));
    Writer w(storage, base, "the event texts");
    for (const EventTextRecord& record : records) {
        if (record.directory) {
            if (std::size_t(*record.directory) >= directory.size())
                throw ImportError("the event text directory has " + std::to_string(directory.size()) + " entries, not " +
                                  std::to_string(*record.directory + 1));
            directory[std::size_t(*record.directory)] = w.here();
        }
        for (const EventTextPage& page : record.pages) {
            if (page.marker) w.byte(0xFF);
            w.byte(page.format);
            w.byte(page.rows);
            w.string(text_for(texts, page.text));
        }
        w.byte(0);
    }
    w.finish();
}

}  // namespace mw1o::import
