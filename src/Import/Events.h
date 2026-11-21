// story/events.json: the campaign's event script as typed records, not bytes
// (docs/phase2/PLAN.md), and the story-planet pools.
//
//   {"captions": [{"colour", "address", "text"}, ...],
//    "events": [{"id", "class", "script": [command, ...]}, ...],
//    "list_entries": [event or null, ...], "lists": [entry or null, ...],
//    "fixed_event": event,
//    "planet_pools": [[planet number, ...], ...]}
//
// A command is {"kind": "condition" or "action", "opcode", operands...}; a
// script alternates the two, as the original's does, and its operands are the
// original's, named: random_threshold, date {month, submonth, year},
// planet_selector, seen_event, house_selector and threshold_half, window
// {start, end}, cash_delta, choices [{caption, branch}]. References count from
// 1: an event is its place in "events", an entry its place in "list_entries",
// a caption its place in "captions". Events are listed in the order the
// original's importer meets them, so the game's arrays fill as they did. A
// caption's text is a text id; "address" is where it sat, which the choice
// menus (still read from the exe until the layout slice) look it up by.
#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "Records.h"

namespace mw1o::import {

struct EventDate {
    std::uint8_t month = 0, submonth = 0, year = 0;
};

struct EventChoice {
    int caption = 0;             // from 1
    std::optional<int> branch;   // an event, from 1; none: the stored zero branch
};

struct EventCommand {
    bool action = false;
    std::uint8_t opcode = 0;
    std::uint8_t random_threshold = 0, seen_event = 0, house_selector = 0;
    std::int8_t planet_selector = 0, threshold_half = 0;
    EventDate date, window_start, window_end;
    std::uint32_t cash_delta = 0;
    std::vector<EventChoice> choices;
};

struct EventRecord {
    std::uint8_t id = 0, class_code = 0;
    std::vector<EventCommand> script;
};

struct EventCaption {
    std::uint8_t colour = 0;
    std::uint16_t address = 0;
    std::string text;  // text id
};

struct Events {
    std::vector<EventCaption> captions;
    std::vector<EventRecord> events;
    std::vector<std::optional<int>> list_entries;  // events, from 1
    std::vector<std::optional<int>> lists;         // list entries, from 1
    int fixed_event = 0;
    std::vector<std::vector<std::uint8_t>> planet_pools;
};

// From the pieces events.data (MW_MAIN 1090:5CF4..655C: the captions, the
// choice menus, the dispatch tables, the lists and the records) and
// events.planet_pools (1090:8FDA..8FF3), decoded as the original's importer
// decodes them. A table without them writes an empty file. Throws ImportError.
std::string events_json(const PieceMap& pieces);

// Read strictly; throws ImportError naming the file.
Events load_events(const std::filesystem::path& write_dir);

}  // namespace mw1o::import
