// The story's typed records (docs/phase2/PLAN.md), beside story/text.json:
//
//   story/pilots.json    {"pilots": [{"portrait", "gunnery", "piloting",
//                         "name", "dialogue"}, ...]}            pilots 1..N
//   story/planets.json   {"planets": [{"number", "house", "landscape_and_market",
//                         "maximum_mission_type", "facility_display", "map_x",
//                         "map_y", "travel_time_weight", "population", "name",
//                         "description"}, ...], "alphabetical": [planet ids]}
//   story/missions.json  {"missions": [{"scenario", "unidentified_2", "text"}, ...],
//                         "categories": [[mission ids], ...]}
//   story/event_text.json {"records": [{"directory": index or null, "pages":
//                         [{"marker", "format", "rows", "text"}, ...]}, ...]}
//                         the event text bank (MW_MAIN 1AA5), records in the
//                         exe's storage order; "marker" is the page's leading FF
//
// Numbers are the original's; "name", "dialogue", "description" and "text"
// are text ids into story/text.json. Ids count from 1 in the original's
// order. The importer writes these from the import table's pieces (the
// records' headers and the original's offset tables, turned into ids); the
// game rebuilds the original's storage from them, laid out as the exe lays
// it out, so the same story gives the same bytes and offsets.
#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "Story.h"

namespace mw1o::import {

struct PilotRecord {
    std::uint8_t portrait = 0, gunnery = 0, piloting = 0;
    std::string name, dialogue;  // text ids
};

struct PlanetRecord {
    std::uint8_t number = 0, house = 0, landscape_and_market = 0, maximum_mission_type = 0;
    std::uint8_t facility_display = 0, map_x = 0, map_y = 0, travel_time_weight = 0;
    std::uint16_t population = 0;
    std::string name, description;  // text ids
};

struct Planets {
    std::vector<PlanetRecord> planets;
    std::vector<int> alphabetical;  // planet ids, in the travel screen's order
};

struct MissionRecord {
    std::uint8_t scenario = 0, unidentified_2 = 0;
    std::string text;  // text id
};

struct Missions {
    std::vector<MissionRecord> missions;
    std::vector<std::vector<int>> categories;  // mission ids per category; each a consecutive run
};

struct EventTextPage {
    bool marker = false;  // the page starts with FF (its panel discards the return)
    std::uint8_t format = 0, rows = 0;
    std::string text;  // text id; the text keeps its control codes and FF placeholders
};

struct EventTextRecord {
    std::optional<int> directory;  // the directory entry pointing at it; none for text no entry reaches
    std::vector<EventTextPage> pages;
};

// A verified piece's bytes and the offset it was read at (its SEG:OFF's OFF).
struct PieceBytes {
    std::uint16_t offset = 0;
    std::string bytes;
};
using PieceMap = std::map<std::string, PieceBytes, std::less<>>;

// The files' contents from the pieces (pilot.N.prefix, planet.N.header,
// planets.offsets, planets.alphabetical, mission.N.header, missions.*). A
// table without a kind's pieces writes an empty file of that kind. Throws
// ImportError if the pieces don't fit together.
std::string pilots_json(const PieceMap& pieces);
std::string planets_json(const PieceMap& pieces);
std::string missions_json(const PieceMap& pieces);
std::string event_text_json(const PieceMap& pieces);  // event_text.directory, event_text.<key>.<page>.*

// The files, read strictly. Throw ImportError naming the file.
std::vector<PilotRecord> load_pilots(const std::filesystem::path& write_dir);
Planets load_planets(const std::filesystem::path& write_dir);
Missions load_missions(const std::filesystem::path& write_dir);
std::vector<EventTextRecord> load_event_text(const std::filesystem::path& write_dir);

// The original's storage, rebuilt. `base` is the storage's offset in the data
// segment; offsets written are data-segment offsets, as the exe holds them.
// The rest of `storage` is zeroed. Throw ImportError if anything doesn't fit
// (a text id missing, a name longer than its field, the storage too small,
// counts that differ from the arrays').

// Each pilot: portrait, gunnery, piloting, the name in 11 bytes (NUL-padded),
// the dialogue and its NUL. offsets[0] is 0 (no pilot 0).
void build_pilots(const std::vector<PilotRecord>& pilots, const StoryText& texts, std::span<std::uint8_t> storage,
                  std::uint16_t base, std::span<std::uint16_t> offsets);

// Each planet: its 10-byte header, the name and the description, each NUL-ended.
void build_planets(const Planets& planets, const StoryText& texts, std::span<std::uint8_t> storage,
                   std::uint16_t base, std::span<std::uint16_t> offsets, std::span<std::uint16_t> alphabetical);

// Each mission: scenario, unidentified_2, the text and its NUL. entries[i] is
// mission i+1's offset; a category's list offset points at its first mission's
// entry (entry_base + 2 * index) and its maximum index is its count - 1. An
// empty category is written as the exe has it: maximum 0, list offset 0.
void build_missions(const Missions& missions, const StoryText& texts, std::span<std::uint8_t> storage,
                    std::uint16_t base, std::span<std::uint8_t> maximum_indices, std::span<std::uint16_t> list_offsets,
                    std::span<std::uint16_t> entries, std::uint16_t entry_base);

// The event text bank: each record's pages ([FF] format rows text NUL) and a 0
// after them, in order; directory[i] is the offset of the record whose
// directory is i, and 0 where none is.
void build_event_text(const std::vector<EventTextRecord>& records, const StoryText& texts,
                      std::span<std::uint8_t> storage, std::uint16_t base, std::span<std::uint16_t> directory);

}  // namespace mw1o::import
