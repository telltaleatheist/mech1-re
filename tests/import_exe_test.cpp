// The whole import, as the game runs it at start: the story (imported into a
// scratch write dir), the rules, the layout, then MW_MAIN's importers (from
// those alone; slice 6b) and BTECH's (from those alone; slice 6d), at the
// revision the story records. Any refusal fails, so a piece the
// importers can't take (a wrong record count, a missing name) fails here
// and not only in the traces.
//
// Needs the player's files, so ctest skips it (77) unless this is set:
//   MW1O_GAME             a game folder (MW_MAIN.EXE, BTECH.EXE)
//   MW1O_BTECH_REFERENCE  optional: a reference BTECH.EXE, imported as well (a
//                         story from a scratch folder holding it and the game
//                         folder's other exes)
//   MW1O_MAIN_1989        optional: the 1989-11-03 MW_MAIN.EXE, imported as well
//                         (the story and MW_MAIN's importers, from a scratch
//                         folder holding it and the game folder's BTECH.EXE
//                         and MW_EGA.EXE)
// Arguments: <data dir> <scratch dir>. Prints steps only, never game bytes.
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <system_error>
#include <tuple>
#include <vector>

#include "mechwarrior/campaign_events.h"
#include "mechwarrior/combat_installation.h"
#include "mechwarrior/game_files.h"
#include "mechwarrior/original_data.h"
#include "mechwarrior/rules.h"
#include "mechwarrior/problem.h"
#include "mechwarrior/story.h"
#include "mechwarrior/ui_layout.h"
#include "native_exe.h"

using native_exe::find_file;

namespace {
int g_failed = 0;

void step(bool ok, const char* what) {
    std::printf("%s: %s\n", what, ok ? "ok" : "REFUSED");
    if (!ok) ++g_failed;
}

// The revision BTECH.EXE at `path` decodes as (the reference if it doesn't).
BtechRevision revision_of(const fs::path& path) {
    const std::vector<std::uint8_t> file = native_exe::read(path);
    BtechRevision revision = BTECH_REVISION_REFERENCE;
    std::uint8_t* image = nullptr;
    if (BtechDecodeImage(file.data(), file.size(), &image, &revision)) std::free(image);
    return revision;
}
std::string read(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

void write(const fs::path& path, const std::string& bytes) { std::ofstream(path, std::ios::binary) << bytes; }

// `text` with the first `from` in it replaced.
std::string swapped(std::string text, const std::string& from, const std::string& to) {
    const auto at = text.find(from);
    if (at == std::string::npos) return {};
    return text.replace(at, from.size(), to);
}

// A story folder that can't be used is imported again at the start, once; one
// that can be used, edited or not, is left alone.
void damaged_stories(const fs::path& game, const fs::path& scratch) {
    const fs::path home = scratch / "damaged", story = home / "story", kept = home / "story.replaced", no_game = scratch / "no game";
    fs::create_directories(home);
    fs::create_directories(no_game);
    const auto start = [&](const fs::path& from) { return MwHostPrepareStory(from.string().c_str(), home.string().c_str()); };
    step(start(game), "story, to damage");
    const char* const names[] = {"text.json", "words.json", "native.json", "pilots.json", "planets.json", "missions.json",
                                 "event_text.json", "events.json", "import.json"};
    std::map<std::string, std::string> good;
    for (const char* name : names) good[name] = read(story / name);
    const auto whole = [&] {
        for (const char* name : names)
            if (read(story / name) != good[name]) return false;
        return true;
    };
    // What, the file, its bytes.
    std::vector<std::tuple<std::string, std::string, std::string>> cases;
    for (const char* name : names) {
        cases.emplace_back("cut short", name, good[name].substr(0, good[name].size() / 2));
        cases.emplace_back("empty", name, "");
        cases.emplace_back("another file's", name, "{}");
    }
    // The shapes are right; the game's importers would refuse them.
    const auto edit = [&](const char* what, const char* name, const std::string& from, const std::string& to) {
        const std::string bytes = swapped(good[name], from, to);
        step(!bytes.empty(), "an edit that finds its place");
        cases.emplace_back(what, name, bytes);
    };
    edit("a text's id changed", "text.json", "\"pilot.1.name\"", "\"pilot.1.nome\"");
    edit("a text's id changed, in no record", "text.json", "\"intro.caption.1.left\"", "\"intro.caption.1.lost\"");
    edit("a record's text id changed", "pilots.json", "\"pilot.1.name\"", "\"pilot.1.nome\"");
    edit("a planet's text id changed", "planets.json", "\"planet.1.name\"", "\"planet.1.nome\"");
    edit("a mission's text id changed", "missions.json", "\"mission.1.text\"", "\"mission.1.texd\"");
    edit("a caption's text id changed", "events.json", "\"event.caption.1\"", "\"event.caption.x\"");
    edit("a planet pool too many", "events.json", "\"planet_pools\": [", "\"planet_pools\": [[1],");
    // Inside a text: where its characters begin.
    const auto text_at = [&](const char* id) {
        return good["text.json"].find("\"text\": \"", good["text.json"].find("\"id\": \"" + std::string(id) + "\"")) + 9;
    };
    cases.emplace_back("a pilot's name too long", "text.json", std::string(good["text.json"]).insert(text_at("pilot.1.name"), 40, 'x'));
    cases.emplace_back("no pilots", "pilots.json", "{\"pilots\": []}");
    cases.emplace_back("no missions", "missions.json", "{\"missions\": [], \"categories\": []}");
    cases.emplace_back("no planets", "planets.json", "{\"planets\": [], \"alphabetical\": []}");
    cases.emplace_back("no texts", "text.json", "{\"texts\": []}");
    cases.emplace_back("no words", "words.json", "{\"words\": []}");
    cases.emplace_back("no spans", "native.json", "{\"native\": []}");
    for (const auto& [what, name, bytes] : cases) {
        write(story / name, bytes);
        step(!start(no_game), (name + ", " + what + ": refused without the copy").c_str());
        const std::string why = MwLastProblem();
        step(why.find("is missing from") != std::string::npos && why.find("good copy") != std::string::npos,
             (name + ", " + what + ": the copy is named").c_str());
        step(read(story / name) == bytes, (name + ", " + what + ": a refusal changes nothing").c_str());
        step(start(game) && whole(), (name + ", " + what + ": imported again").c_str());
        step(read(kept / name) == bytes, (name + ", " + what + ": kept aside").c_str());
    }
    for (const char* name : names) {
        fs::remove(story / name);
        step(start(game) && whole(), (std::string(name) + " deleted: imported again").c_str());
    }
    // Words changed by hand are the player's: a shorter text, and one more text.
    const std::string shorter = swapped(good["text.json"], "\"texts\": [", "\"texts\": [\n    {\"id\": \"mine\", \"text\": \"x\"},");
    write(story / "text.json", shorter);
    step(!shorter.empty() && start(no_game) && read(story / "text.json") == shorter, "an edited text stays, and needs no copy");
    const std::string cut = std::string(good["text.json"]).erase(text_at("pilot.1.dialogue"), 1);
    write(story / "text.json", cut);
    step(start(no_game) && read(story / "text.json") == cut, "a shorter text stays");
    write(story / "text.json", good["text.json"]);
    // A folder that can't be written: not the copy's fault, nothing lost.
    write(story / "pilots.json", "{}");
    fs::create_directories(story / "words.json.part" / "x");
    step(!start(game), "a blocked folder: refused");
    const std::string why = MwLastProblem();
    step(why.find("can't write") != std::string::npos && why.find("good copy") == std::string::npos, "a blocked folder is named");
    step(read(story / "pilots.json") == "{}" && read(story / "words.json") == good["words.json"], "a blocked import changes nothing");
    fs::remove_all(story / "words.json.part");
    step(start(game) && whole(), "unblocked: imported again");
}
}  // namespace

int main(int argc, char** argv) {
    const char* game = std::getenv("MW1O_GAME");
    if (!game) {
        std::printf("skipped: set MW1O_GAME (and optionally MW1O_BTECH_REFERENCE, MW1O_MAIN_1989)\n");
        return 77;
    }
    if (argc != 3) {
        std::fprintf(stderr, "usage: import_exe_test <data dir> <scratch dir>\n");
        return 2;
    }
    const fs::path data = argv[1];
    const fs::path scratch = argv[2];
    std::error_code ec;
    fs::remove_all(scratch, ec);
    fs::create_directories(scratch);

    const std::string btech_exe = find_file(game, "BTECH.EXE").string();
    mw1o::files::set_save_directory(scratch.string());
    step(MwHostPrepareStory(game, scratch.string().c_str()), "story");
    step(MwHostPrepareRules(data.string().c_str()), "rules");
    step(MwHostPrepareUi(data.string().c_str()), "ui");
    if (!g_failed) {
        step(MwImportCampaignInitialData(), "MW_MAIN's importers");
        step(MwImportCampaignEventText(), "MW_MAIN's event text");
        step(MwImportCampaignMusicData(), "MW_MAIN's music");
        step(BtechHostLoadInitialData(), "BTECH's importers");
        step(btechLoadedRevision == revision_of(btech_exe), "BTECH's revision, from the story");
    }
    if (!g_failed) damaged_stories(game, scratch);
    if (const char* reference = std::getenv("MW1O_BTECH_REFERENCE"); reference && !g_failed) {
        const fs::path folder = scratch / "game_btech_reference", write = scratch / "write_btech_reference";
        fs::create_directories(folder);
        fs::create_directories(write);
        for (const auto& entry : fs::directory_iterator(game)) {
            std::string name = entry.path().filename().string();
            for (char& c : name) c = char(std::toupper(static_cast<unsigned char>(c)));
            if (entry.is_regular_file() && name.size() > 4 && name.substr(name.size() - 4) == ".EXE" && name != "BTECH.EXE")
                fs::copy_file(entry.path(), folder / name);
        }
        fs::copy_file(reference, folder / "BTECH.EXE");
        step(MwHostPrepareStory(folder.string().c_str(), write.string().c_str()), "story, the reference BTECH");
        if (!g_failed) {
            step(BtechHostLoadInitialData(), "BTECH's importers, the reference revision");
            step(btechLoadedRevision == BTECH_REVISION_REFERENCE && revision_of(reference) == BTECH_REVISION_REFERENCE,
                 "BTECH's revision, from the story, the reference");
        }
    }
    if (const char* main1989 = std::getenv("MW1O_MAIN_1989"); main1989 && !g_failed) {
        const fs::path folder = scratch / "game_main_1989", write = scratch / "write_main_1989";
        fs::create_directories(folder);
        fs::create_directories(write);
        fs::copy_file(main1989, folder / "MW_MAIN.EXE");
        fs::copy_file(find_file(game, "MW_EGA.EXE"), folder / "MW_EGA.EXE");
        fs::copy_file(btech_exe, folder / "BTECH.EXE");
        step(MwHostPrepareStory(folder.string().c_str(), write.string().c_str()), "story, the 1989 MW_MAIN");
        if (!g_failed) {
            step(MwImportCampaignInitialData(), "MW_MAIN's importers, the 1989 MW_MAIN");
            step(MwImportCampaignEventText(), "MW_MAIN's event text, the 1989 MW_MAIN");
            step(MwImportCampaignMusicData(), "MW_MAIN's music, the 1989 MW_MAIN");
        }
    }
    fs::remove_all(scratch, ec);
    std::printf("%s\n", g_failed ? "FAILED" : "ok");
    return g_failed ? 1 : 0;
}
