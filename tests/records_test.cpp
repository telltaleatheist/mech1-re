// src/Import/Records: the typed story files on made-up records (no game
// files). Pieces -> pilots/planets/missions.json -> load -> rebuilt storage
// laid out as the exe lays it out; the strict reader and the rebuild refuse
// what doesn't fit.
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

}  // namespace

int main() {
    fs::remove_all(kDir);
    try {
        the_files_round_trip();
        the_storage_is_rebuilt_as_the_exe_lays_it_out();
        what_doesnt_fit_is_refused();
    } catch (const std::exception& e) {
        check(false, std::string("unexpected: ") + e.what());
    }
    fs::remove_all(kDir);
    std::printf("%s\n", g_failures ? "FAILED" : "ok");
    return g_failures ? 1 : 0;
}
