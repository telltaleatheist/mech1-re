// The rule table against the player's exes: every piece's bytes hash as the
// table says (a tuned piece is only reported). BTECH's pieces are read in its
// unpacked image (src/Support/BtechImage.cpp), in both revisions when a
// reference BTECH.EXE is given.
//
// Needs the player's files, so ctest skips it (77) unless this is set:
//   MW1O_GAME             a game folder (MW_MAIN.EXE, BTECH.EXE)
//   MW1O_BTECH_REFERENCE  optional: a reference BTECH.EXE
// Prints ids and counts only, never the game's bytes.
#include "../src/Rules/RuleTable.h"
#include "native_exe.h"

using namespace mw1o::rules;
using namespace native_exe;

int main() {
    const char* game = std::getenv("MW1O_GAME");
    if (!game) {
        std::printf("skipped: set MW1O_GAME (and optionally MW1O_BTECH_REFERENCE)\n");
        return 77;
    }
    const auto main_image = load_image(find_file(game, "MW_MAIN.EXE"));
    const auto btech = btech_image(find_file(game, "BTECH.EXE"));
    check(!main_image.empty(), "MW_MAIN.EXE doesn't load");
    check(!btech.empty(), "BTECH.EXE doesn't unpack");
    const int a = check_pieces(rule_table(), "MW_MAIN.EXE", main_image, "MW_MAIN.EXE");
    const int b = check_pieces(rule_table(), "BTECH.EXE", btech, "BTECH.EXE");
    std::printf("%d MW_MAIN and %d BTECH rule tables as the table's\n", a, b);
    if (const char* reference = std::getenv("MW1O_BTECH_REFERENCE")) {
        const auto image = btech_image(reference);
        check(!image.empty(), "the reference BTECH.EXE doesn't unpack");
        std::printf("%d BTECH rule tables as the table's in the reference revision\n",
                    check_pieces(rule_table(), "BTECH.EXE", image, "the reference BTECH.EXE"));
    }
    std::printf("%s\n", g_failures ? "FAILED" : "ok");
    return g_failures ? 1 : 0;
}
