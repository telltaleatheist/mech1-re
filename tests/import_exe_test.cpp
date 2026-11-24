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
#include <vector>
#include <filesystem>
#include <string>
#include <system_error>

#include "mechwarrior/campaign_events.h"
#include "mechwarrior/combat_installation.h"
#include "mechwarrior/game_files.h"
#include "mechwarrior/original_data.h"
#include "mechwarrior/rules.h"
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
