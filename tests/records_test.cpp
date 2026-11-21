// src/Import/Records: the typed story files on made-up records (no game
// files). Pieces -> pilots/planets/missions.json -> load -> rebuilt storage
// laid out as the exe lays it out; the strict reader and the rebuild refuse
// what doesn't fit.
#include "../src/Import/Events.h"
#include "../src/Import/Records.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <functional>
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

void refused(const std::function<void()>& run, const std::string& want, const std::string& what) {
    try {
        run();
        check(false, what + ": accepted");
    } catch (const ImportError& e) {
        check(std::string(e.what()).find(want) != std::string::npos,
              what + ": \"" + e.what() + "\" doesn't say \"" + want + "\"");
    }
}

void write(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary) << text;
}

std::string word(std::uint16_t w) { return std::string{char(w & 0xFF), char(w >> 8)}; }

const fs::path kDir = fs::current_path() / "records_test_work";

// Two pilots, three planets (listed alphabetically as 3, 1), four missions in
// categories [], [1 2], [4], [3].
PieceMap made_up_pieces() {
    PieceMap p;
    p["pilot.1.prefix"] = {0x9177, std::string("\x05\x01\x02", 3)};
    p["pilot.2.prefix"] = {0x9190, std::string("\x07\x03\x00", 3)};
    p["planets.offsets"] = {0x6570, word(0x67B6) + word(0x67C8) + word(0x67D9)};
    p["planets.alphabetical"] = {0x6694, word(0x67D9) + word(0x67B6)};
    p["planet.1.header"] = {0x67B6, std::string("\x01\x02\x03\x04\x00\x10\x20\x05\x34\x12", 10)};
    p["planet.2.header"] = {0x67C8, std::string("\x02\x00\x01\x01\x01\x11\x21\x06\xFF\x00", 10)};
    p["planet.3.header"] = {0x67D9, std::string("\x03\x04\x02\x02\x00\x12\x22\x07\x00\x01", 10)};
    p["missions.maximum_indices"] = {0x8A16, std::string("\x00\x01\x00\x00", 4)};
    p["missions.list_offsets"] = {0x8A1B, word(0) + word(0x8A25) + word(0x8A2B) + word(0x8A29)};
    p["missions.entries"] = {0x8A25, word(0x8A69) + word(0x8A70) + word(0x8A75) + word(0x8A7B)};
    p["mission.1.header"] = {0x8A69, std::string("\x01\x02", 2)};
    p["mission.2.header"] = {0x8A70, std::string("\x03\x01", 2)};
    p["mission.3.header"] = {0x8A75, std::string("\x04\x07", 2)};
    p["mission.4.header"] = {0x8A7B, std::string("\x05\x05", 2)};
    return p;
}

StoryText made_up_texts() {
    return {{"pilot.1.name", "Ann       "}, {"pilot.1.dialogue", "Hello."}, {"pilot.2.name", "Bo"},
            {"pilot.2.dialogue", std::string("Hi\x0D\xE9", 4)},
            {"planet.1.name", "Aa"},        {"planet.1.description", "First."},
            {"planet.2.name", "Bb"},        {"planet.2.description", "Second"},
            {"planet.3.name", "Cc"},        {"planet.3.description", "Third"},
            {"mission.1.text", "Go now"},   {"mission.2.text", "Hold"},
            {"mission.3.text", "Raid!"},    {"mission.4.text", "Scout"}};
}

void the_files_round_trip() {
    const PieceMap pieces = made_up_pieces();
    write(story_dir(kDir) / "pilots.json", pilots_json(pieces));
    write(story_dir(kDir) / "planets.json", planets_json(pieces));
    write(story_dir(kDir) / "missions.json", missions_json(pieces));

    const auto pilots = load_pilots(kDir);
    check(pilots.size() == 2 && pilots[0].portrait == 5 && pilots[0].gunnery == 1 && pilots[0].piloting == 2 &&
              pilots[1].portrait == 7 && pilots[1].name == "pilot.2.name" && pilots[1].dialogue == "pilot.2.dialogue",
          "pilots.json keeps the prefixes and names the texts");
    const auto planets = load_planets(kDir);
    check(planets.planets.size() == 3 && planets.planets[0].population == 0x1234 && planets.planets[1].map_x == 0x11 &&
              planets.planets[2].house == 4 && planets.planets[0].description == "planet.1.description",
          "planets.json keeps the headers");
    check(planets.alphabetical == std::vector<int>{3, 1}, "the alphabetical table becomes planet ids");
    const auto missions = load_missions(kDir);
    check(missions.missions.size() == 4 && missions.missions[2].scenario == 4 && missions.missions[2].unidentified_2 == 7,
          "missions.json keeps the headers");
    check(missions.categories == std::vector<std::vector<int>>{{}, {1, 2}, {4}, {3}},
          "the category tables become runs of mission ids");

    // An empty table writes empty files of each kind.
    write(story_dir(kDir / "empty") / "pilots.json", pilots_json({}));
    write(story_dir(kDir / "empty") / "planets.json", planets_json({}));
    write(story_dir(kDir / "empty") / "missions.json", missions_json({}));
    check(load_pilots(kDir / "empty").empty() && load_planets(kDir / "empty").planets.empty() &&
              load_missions(kDir / "empty").missions.empty(),
          "a table without records writes empty files");
}

void the_storage_is_rebuilt_as_the_exe_lays_it_out() {
    const auto texts = made_up_texts();
    std::vector<std::uint8_t> storage(40, 0xEE);
    std::vector<std::uint16_t> offsets(3, 0xEEEE);
    build_pilots(load_pilots(kDir), texts, storage, 0x9177, offsets);
    const std::string want = std::string("\x05\x01\x02", 3) + std::string("Ann       \0", 11) + std::string("Hello.\0", 7) +
                             std::string("\x07\x03\x00", 3) + std::string("Bo\0\0\0\0\0\0\0\0\0", 11) +
                             std::string("Hi\x0D\xE9\0", 5);
    check(std::string(storage.begin(), storage.begin() + std::ptrdiff_t(want.size())) == want, "the pilots' bytes");
    check(std::all_of(storage.begin() + std::ptrdiff_t(want.size()), storage.end(), [](auto b) { return b == 0; }),
          "the rest of the pilots' storage is zeroed");
    check(offsets == std::vector<std::uint16_t>{0, 0x9177, 0x9177 + 21}, "the pilots' offsets (none for pilot 0)");

    std::vector<std::uint8_t> planets(64);
    std::vector<std::uint16_t> planet_offsets(3), alphabetical(2);
    build_planets(load_planets(kDir), texts, planets, 0x67B6, planet_offsets, alphabetical);
    check(planet_offsets == std::vector<std::uint16_t>{0x67B6, 0x67B6 + 20, 0x67B6 + 40}, "the planets' offsets");
    check(alphabetical == std::vector<std::uint16_t>{0x67B6 + 40, 0x67B6}, "the alphabetical offsets");
    check(planets[8] == 0x34 && planets[9] == 0x12 && std::string(planets.begin() + 10, planets.begin() + 20) ==
                                                         std::string("Aa\0First.\0", 10),
          "a planet's header (population little-endian), name and description");

    std::vector<std::uint8_t> missions(40), maxima(4);
    std::vector<std::uint16_t> lists(4), entries(4);
    build_missions(load_missions(kDir), texts, missions, 0x8A69, maxima, lists, entries, 0x8A25);
    check(entries == std::vector<std::uint16_t>{0x8A69, 0x8A69 + 9, 0x8A69 + 16, 0x8A69 + 24}, "the mission entries");
    check(maxima == std::vector<std::uint8_t>{0, 1, 0, 0}, "the maximum indices (an empty category is 0)");
    check(lists == std::vector<std::uint16_t>{0, 0x8A25, 0x8A25 + 6, 0x8A25 + 4},
          "the list offsets (an empty category is 0, as the exe has it)");
}

void what_doesnt_fit_is_refused() {
    const auto texts = made_up_texts();
    const auto pilots = load_pilots(kDir);
    std::vector<std::uint8_t> small(30);
    std::vector<std::uint16_t> offsets(3), wrong_count(4);
    refused([&] { build_pilots(pilots, texts, small, 0, offsets); }, "don't fit", "pilots past their storage");
    std::vector<std::uint8_t> storage(64);
    refused([&] { build_pilots(pilots, texts, storage, 0, wrong_count); }, "the story has 2 pilots", "a pilot count");
    auto long_name = texts;
    long_name["pilot.2.name"] = "Elevenchars";
    refused([&] { build_pilots(pilots, long_name, storage, 0, offsets); }, "longer than its field", "a name past 10");
    auto missing = texts;
    missing.erase("pilot.2.dialogue");
    refused([&] { build_pilots(pilots, missing, storage, 0, offsets); }, "no text \"pilot.2.dialogue\"", "a missing text");

    const fs::path bad = kDir / "bad";
    const auto pilots_file = story_dir(bad) / "pilots.json";
    write(pilots_file, "{\"pilots\": [{\"portrait\": 1.5, \"gunnery\": 0, \"piloting\": 0, \"name\": \"a\", \"dialogue\": \"b\"}]}");
    refused([&] { load_pilots(bad); }, "portrait is not a whole number", "a fractional number");
    write(pilots_file, "{\"pilots\": [{\"portrait\": 256, \"gunnery\": 0, \"piloting\": 0, \"name\": \"a\", \"dialogue\": \"b\"}]}");
    refused([&] { load_pilots(bad); }, "from 0 to 255", "a byte past 255");
    write(pilots_file, "{\"pilots\": [{\"portrait\": 1, \"gunnery\": 0, \"piloting\": 0, \"name\": \"a\"}]}");
    refused([&] { load_pilots(bad); }, "no \"dialogue\"", "a missing field");
    write(pilots_file, "{\"pilots\": [}");
    refused([&] { load_pilots(bad); }, "pilots.json: JSON at line 1", "broken JSON names the file and line");

    const auto missions_file = story_dir(bad) / "missions.json";
    write(missions_file, "{\"missions\": [{\"scenario\": 1, \"unidentified_2\": 0, \"text\": \"t\"},"
                         " {\"scenario\": 1, \"unidentified_2\": 0, \"text\": \"u\"}], \"categories\": [[2, 1]]}");
    refused([&] { load_missions(bad); }, "not consecutive", "a category out of order");
    write(missions_file, "{\"missions\": [{\"scenario\": 1, \"unidentified_2\": 0, \"text\": \"t\"}], \"categories\": [[2]]}");
    refused([&] { load_missions(bad); }, "names no record", "a category naming no mission");

    PieceMap broken = made_up_pieces();
    broken["planets.offsets"].bytes = word(0x67C8) + word(0x67B6) + word(0x67D9);
    refused([&] { planets_json(broken); }, "doesn't list the planets in order", "planet offsets out of order");
    broken = made_up_pieces();
    broken["planets.alphabetical"].bytes = word(0x1234);
    refused([&] { planets_json(broken); }, "names no planet record", "an alphabetical offset naming nothing");
}

// Directory of 4: entry 1 -> record "1" (two pages, the second marked),
// entry 3 -> record "3"; a record no entry reaches between them.
PieceMap made_up_event_text() {
    PieceMap p;
    p["event_text.directory"] = {0, word(0) + word(0x00EE) + word(0) + word(0x0104)};
    p["event_text.1.1.header"] = {0x00EE, std::string("\x2D\x03", 2)};
    p["event_text.1.2.header"] = {0x00F6, std::string("\xFF\x2E\x02", 3)};
    p["event_text.unreferenced1.1.header"] = {0x00FE, std::string("\x01\x01", 2)};
    p["event_text.3.1.header"] = {0x0104, std::string("\x04\x05", 2)};
    return p;
}

void the_event_text_bank() {
    const fs::path dir = kDir / "events";
    write(story_dir(dir) / "event_text.json", event_text_json(made_up_event_text()));
    const auto records = load_event_text(dir);
    check(records.size() == 3 && records[0].directory == 1 && !records[1].directory && records[2].directory == 3,
          "records in storage order with their directory entries (none for the unreferenced one)");
    check(records[0].pages.size() == 2 && !records[0].pages[0].marker && records[0].pages[1].marker &&
              records[0].pages[1].format == 0x2E && records[0].pages[1].rows == 2 &&
              records[0].pages[1].text == "event_text.1.2.text" && records[1].pages[0].text == "event_text.unreferenced1.1.text",
          "pages keep their marker, format, rows and text ids");

    const StoryText texts = {{"event_text.1.1.text", "Ab"}, {"event_text.1.2.text", std::string("C\xFF", 2)},
                             {"event_text.unreferenced1.1.text", "D"}, {"event_text.3.1.text", "Efg"}};
    std::vector<std::uint8_t> storage(32, 0xEE);
    std::vector<std::uint16_t> directory(4, 0xEEEE);
    build_event_text(records, texts, storage, 0x00EE, directory);
    const std::string want = std::string("\x2D\x03" "Ab\0", 5) + std::string("\xFF\x2E\x02" "C\xFF\0", 6) +
                             std::string("\0", 1) + std::string("\x01\x01" "D\0", 4) + std::string("\0", 1) +
                             std::string("\x04\x05" "Efg\0", 6) + std::string("\0", 1);
    check(std::string(storage.begin(), storage.begin() + std::ptrdiff_t(want.size())) == want,
          "the bank's bytes: pages, a 0 after each record, FF kept in the text");
    check(directory == std::vector<std::uint16_t>{0, 0x00EE, 0, 0x00EE + 17}, "the directory (0 where no record is)");
    std::vector<std::uint16_t> small_directory(2);
    refused([&] { build_event_text(records, texts, storage, 0x00EE, small_directory); }, "has 2 entries, not 4",
            "a directory entry past the game's directory");

    PieceMap broken = made_up_event_text();
    broken["event_text.directory"].bytes = word(0x00F6) + word(0) + word(0) + word(0);
    refused([&] { event_text_json(broken); }, "names no record", "a directory entry pointing mid-record");
    broken = made_up_event_text();
    broken["event_text.directory"].bytes = word(0x0104) + word(0x00EE) + word(0) + word(0);
    refused([&] { event_text_json(broken); }, "doesn't match its record's key", "a directory entry and key that differ");
    broken = made_up_event_text();
    broken["event_text.1.2.header"].offset = 0x0110;
    refused([&] { event_text_json(broken); }, "not together", "a record's pages split by another");
    broken = made_up_event_text();
    broken["event_text.3.1.header"].bytes = std::string("\x01\x02\x03", 3);
    refused([&] { event_text_json(broken); }, "not [FF] format rows", "a three-byte header without FF");

    const auto file = story_dir(dir / "bad") / "event_text.json";
    write(file, "{\"records\": [{\"directory\": 1, \"pages\": [{\"marker\": false, \"format\": 1, \"rows\": 1, \"text\": \"a\"}]},"
                " {\"directory\": 1, \"pages\": [{\"marker\": false, \"format\": 1, \"rows\": 1, \"text\": \"b\"}]}]}");
    refused([&] { load_event_text(dir / "bad"); }, "two records have the directory entry 1", "a directory entry twice");
    write(file, "{\"records\": [{\"directory\": null, \"pages\": [{\"marker\": 1, \"format\": 1, \"rows\": 1, \"text\": \"a\"}]}]}");
    refused([&] { load_event_text(dir / "bad"); }, "\"marker\" is not true or false", "a marker that isn't a boolean");
    write(file, "{\"records\": [{\"directory\": null, \"pages\": []}]}");
    refused([&] { load_event_text(dir / "bad"); }, "has no pages", "a record without pages");
}

// A made-up event region (1090:5CF4..655C). Captions fill 5CF4..5E1C; every
// list entry but the last names a record; records A (6121), B (6140), the
// fixed event C (6353) and D (6200), which only A's choice reaches.
std::string made_up_event_region() {
    std::string r(0x655C - 0x5CF4, '\0');
    const auto put = [&](std::uint16_t at, const std::string& bytes) { r.replace(at - 0x5CF4, bytes.size(), bytes); };
    put(0x5CF4, std::string("\x05Yes\0", 5));
    put(0x5CF9, std::string("\x06", 1) + std::string(0x5E1C - 0x5CF9 - 2, 'n') + std::string("\0", 1));
    put(0x6067, word(0x6073) + word(0) + word(0x6075) + word(0) + word(0) + word(0));
    for (std::uint16_t at = 0x6073; at < 0x611F; at += 2) put(at, word(at % 4 == 3 ? 0x6121 : 0x6140));
    put(0x611F, word(0));
    put(0x6121, std::string("\x01\x00" "\x00\x32" "\x04\x45\x23\x01\x00" "\x09\x02", 11) + word(0x5CF4) + word(0x5CF9) +
                    word(0x6200) + word(0) + std::string("\xFF", 1));
    put(0x6140, std::string("\x02\x03" "\x01\x01\x02\x03" "\x02\x04\x05\x06\x07\x08\x09" "\x04\x02\xFD" "\x08\xFE" "\xFF", 20));
    put(0x6200, std::string("\x04\x00" "\x02\xFF" "\x00" "\xFF", 6));
    put(0x6353, std::string("\x03\x01\xFF", 3));
    return r;
}

PieceMap made_up_events() {
    PieceMap p;
    p["events.data"] = {0x5CF4, made_up_event_region()};
    p["events.planet_pools"] = {0x8FDA, std::string("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19", 25)};
    return p;
}

void the_event_script() {
    const fs::path dir = kDir / "script";
    write(story_dir(dir) / "events.json", events_json(made_up_events()));
    const Events e = load_events(dir);
    check(e.captions.size() == 2 && e.captions[0].colour == 5 && e.captions[0].address == 0x5CF4 &&
              e.captions[1].address == 0x5CF9 && e.captions[1].text == "event.caption.2",
          "captions keep colour, address and a text id");
    check(e.events.size() == 4 && e.events[0].id == 1 && e.events[1].id == 2 && e.events[2].id == 3 && e.events[3].id == 4,
          "events in the order the original importer meets them (entries, fixed event, branches)");
    check(e.list_entries.size() == 87 && e.list_entries[0] == 1 && e.list_entries[1] == 2 && !e.list_entries[86],
          "list entries name events; the last is none");
    check(e.lists == std::vector<std::optional<int>>{1, std::nullopt, 2, std::nullopt, std::nullopt, std::nullopt},
          "lists name their first entry");
    check(e.fixed_event == 3, "the fixed event");
    const auto& a = e.events[0].script;
    check(a.size() == 3 && !a[0].action && a[0].opcode == 0 && a[0].random_threshold == 0x32 && a[1].action &&
              a[1].opcode == 4 && a[1].cash_delta == 0x00012345 && !a[2].action && a[2].opcode == 9 &&
              a[2].choices.size() == 2 && a[2].choices[0].caption == 1 && a[2].choices[0].branch == 4 &&
              a[2].choices[1].caption == 2 && !a[2].choices[1].branch,
          "a script alternates condition and action, with threshold, cash and choices");
    const auto& b = e.events[1].script;
    check(b.size() == 4 && b[0].date.month == 1 && b[0].date.year == 3 && b[1].window_start.month == 4 &&
              b[1].window_end.year == 9 && b[2].house_selector == 2 && b[2].threshold_half == -3 &&
              b[3].planet_selector == -2 && e.events[1].class_code == 3,
          "dates, windows, house standing and signed selectors");
    check(e.events[2].script.empty() && e.events[3].script[0].planet_selector == -1, "an empty script, a planet condition");
    check(e.planet_pools.size() == 6 && e.planet_pools[0].size() == 8 && e.planet_pools[0][7] == 8 &&
              e.planet_pools[5] == std::vector<std::uint8_t>{0x17, 0x18, 0x19},
          "the six planet pools keep their lengths");

    write(story_dir(kDir / "noevents") / "events.json", events_json({}));
    check(load_events(kDir / "noevents").events.empty(), "a table without the event pieces writes an empty file");

    PieceMap broken = made_up_events();
    broken["events.data"].bytes[0x6200 - 0x5CF4 + 2] = 0x0A;
    refused([&] { events_json(broken); }, "opcode is past 9", "an opcode past 9");
    broken = made_up_events();
    broken["events.data"].bytes[0x6121 - 0x5CF4 + 10] = 0x04;
    refused([&] { events_json(broken); }, "neither 2 nor 3 choices", "four choices");
    broken = made_up_events();
    broken["events.data"].bytes.replace(0x612C - 0x5CF4, 2, word(0x5CF5));
    refused([&] { events_json(broken); }, "names no caption", "a choice naming no caption");
    broken = made_up_events();
    broken["events.data"].bytes.replace(0x6075 - 0x5CF4, 2, word(0));
    refused([&] { events_json(broken); }, "don't end with their one zero", "a zero list entry before the end");
    broken = made_up_events();
    broken.erase("events.planet_pools");
    refused([&] { events_json(broken); }, "only one of", "the pools without the script");

    const auto file = story_dir(dir / "bad") / "events.json";
    const std::string head = "{\"captions\": [{\"colour\": 1, \"address\": 1, \"text\": \"c\"}], \"events\": [{\"id\": 1, \"class\": 0, \"script\": ";
    const std::string tail = "}], \"list_entries\": [1, null], \"lists\": [1], \"fixed_event\": 1, \"planet_pools\": [[1]]}";
    write(file, head + "[{\"kind\": \"action\", \"opcode\": 0}]" + tail);
    refused([&] { load_events(dir / "bad"); }, "alternate condition and action", "a script starting with an action");
    write(file, head + "[{\"kind\": \"condition\", \"opcode\": 9, \"choices\": [{\"caption\": 1, \"branch\": 2}, {\"caption\": 1, \"branch\": null}]}]" + tail);
    refused([&] { load_events(dir / "bad"); }, "branch 2 names nothing", "a branch naming no event");
    write(file, head + "[{\"kind\": \"condition\", \"opcode\": 9, \"choices\": [{\"caption\": 2, \"branch\": 1}, {\"caption\": 1, \"branch\": null}]}]" + tail);
    refused([&] { load_events(dir / "bad"); }, "names no caption", "a choice naming no caption");
    write(file, head + "[{\"kind\": \"condition\", \"opcode\": 2, \"planet_selector\": -129}]" + tail);
    refused([&] { load_events(dir / "bad"); }, "from -128 to 127", "a signed byte out of range");
}

}  // namespace

int main() {
    fs::remove_all(kDir);
    try {
        the_files_round_trip();
        the_storage_is_rebuilt_as_the_exe_lays_it_out();
        what_doesnt_fit_is_refused();
        the_event_text_bank();
        the_event_script();
    } catch (const std::exception& e) {
        check(false, std::string("unexpected: ") + e.what());
    }
    fs::remove_all(kDir);
    std::printf("%s\n", g_failures ? "FAILED" : "ok");
    return g_failures ? 1 : 0;
}
