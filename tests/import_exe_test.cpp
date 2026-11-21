// The whole import, as the game runs it at start: the story (imported into a
// scratch write dir), the rules, the layout, then MW_MAIN's and BTECH's
// importers over the player's exes. Any refusal fails, so a piece the
// importers can't take (a wrong record count, a missing name) fails here
// and not only in the traces.
//
// Needs the player's files, so ctest skips it (77) unless this is set:
//   MW1O_GAME             a game folder (MW_MAIN.EXE, BTECH.EXE)
//   MW1O_BTECH_REFERENCE  optional: a reference BTECH.EXE, imported as well
// Arguments: <data dir> <scratch dir>. Prints steps only, never game bytes.
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>

#include "mechwarrior/campaign_events.h"
#include "mechwarrior/combat_installation.h"
#include "mechwarrior/dos_files.h"
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
}  // namespace

int main(int argc, char** argv) {
    const char* game = std::getenv("MW1O_GAME");
    if (!game) {
        std::printf("skipped: set MW1O_GAME (and optionally MW1O_BTECH_REFERENCE)\n");
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

    const std::string main_exe = find_file(game, "MW_MAIN.EXE").string();
    const std::string btech_exe = find_file(game, "BTECH.EXE").string();
    step(MwHostSetCampaignSaveDirectory(scratch.string().c_str()), "save directory");
    step(MwHostPrepareStory(game, scratch.string().c_str()), "story");
    step(MwHostPrepareRules(data.string().c_str()), "rules");
    step(MwHostPrepareUi(data.string().c_str()), "ui");
    if (!g_failed) {
        step(MwHostLoadCampaignInitialData(main_exe.c_str()), "MW_MAIN's importers");
        step(MwImportCampaignEventText(), "MW_MAIN's event text");
        step(MwHostLoadCampaignMusicData(main_exe.c_str()), "MW_MAIN's music");
        step(BtechHostLoadInitialData(btech_exe.c_str()), "BTECH's importers");
        if (const char* reference = std::getenv("MW1O_BTECH_REFERENCE"))
            step(BtechHostLoadInitialData(reference), "BTECH's importers, the reference revision");
    }
    fs::remove_all(scratch, ec);
    std::printf("%s\n", g_failed ? "FAILED" : "ok");
    return g_failed ? 1 : 0;
}
