// The program's start constants and file names (mechwarrior/native_start.h,
// docs/phase2/PLAN.md 8.3) against the player's exes: every row's bytes, as
// the code sets them, must be the exe's at the row's address. A row that
// differs is named by id; the game's bytes are never printed.
//
// Needs the player's files, so ctest skips it (77) unless this is set:
//   MW1O_GAME   a game folder (MW_MAIN.EXE, MW.EXE)
#include "native_exe.h"
#include "mechwarrior/native_start.h"

#include <cstring>
#include <map>

using namespace native_exe;

int main() {
    const char* game = std::getenv("MW1O_GAME");
    if (!game) {
        std::printf("skipped: set MW1O_GAME\n");
        return 77;
    }
    std::map<std::string, std::vector<std::uint8_t>> images;
    int same = 0;
    for (std::size_t i = 0; i < mwNativeStartRowCount; ++i) {
        const MwNativeStartRow& row = mwNativeStartRows[i];
        auto& image = images[row.exe];
        if (image.empty()) image = load_image(find_file(game, row.exe));
        check(!image.empty(), std::string(row.exe) + " doesn't load");
        const std::size_t at = std::size_t(row.segment - kLoadSegment) * 16 + row.offset;
        if (image.empty() || at + row.size > image.size()) {
            check(false, std::string(row.id) + " runs past " + row.exe);
            continue;
        }
        std::vector<std::uint8_t> bytes(row.size);
        row.fill(bytes.data());
        const bool ok = std::memcmp(bytes.data(), image.data() + at, row.size) == 0;
        check(ok, std::string(row.id) + " differs from " + row.exe);
        same += ok;
    }
    std::printf("%d of %zu start constants as the exes'\n%s\n", same, mwNativeStartRowCount, g_failures ? "FAILED" : "ok");
    return g_failures ? 1 : 0;
}
