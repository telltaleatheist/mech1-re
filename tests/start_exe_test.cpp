// The program's start constants and file names (mechwarrior/native_start.h,
// docs/phase2/PLAN.md 8.3) and BTECH's start image (btech_start_image.h,
// 8.5, slice 6c) against the player's exes: every code, zero and generated
// row's bytes, as the code makes them, must be the exe's at the row's
// address, in each BTECH revision given. A generated table must also keep
// every computed entry clear of an integer before rounding down, so another
// libm cannot round it differently. A row that differs is named by id; the
// game's bytes are never printed.
//
// Needs the player's files, so ctest skips it (77) unless this is set:
//   MW1O_GAME             a game folder (MW_MAIN.EXE, MW.EXE, BTECH.EXE)
//   MW1O_BTECH_REFERENCE  optional: another BTECH.EXE revision, checked too
//   MW1O_BTECH_1990       optional: the same
#include "native_exe.h"
#include "mechwarrior/btech_start_image.h"
#include "mechwarrior/combat_installation.h"
#include "mechwarrior/native_start.h"

#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <set>

using namespace native_exe;

namespace {
// BTECH.EXE unpacked by the game's decoder; empty if it isn't a known revision.
std::vector<std::uint8_t> btech_image(const fs::path& path, BtechRevision* revision) {
    std::ifstream in(path, std::ios::binary);
    std::vector<std::uint8_t> file((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    const std::size_t packed = BtechInstallationPackedBytes(file.size());
    std::uint8_t* image = nullptr;
    if (!packed || packed > file.size() || !BtechDecodeImage(file.data(), packed, &image, revision)) return {};
    std::vector<std::uint8_t> out(image, image + BTECH_IMAGE_BYTES);
    std::free(image);
    return out;
}

// Every code, zero and generated row of BTECH's start image against one revision.
int check_btech(const fs::path& path) {
    BtechRevision revision{};
    const std::vector<std::uint8_t> image = btech_image(path, &revision);
    check(!image.empty(), path.string() + " is not a known BTECH.EXE");
    if (image.empty()) return 0;
    const char* name = revision == BTECH_REVISION_REFERENCE ? "the reference BTECH" : "the 1990 BTECH";
    int same = 0;
    for (std::size_t i = 0; i < btechStartRowCount; ++i) {
        const BtechStartRow& row = btechStartRows[i];
        // Story spans are keyed and hashed by the import; rule and layout
        // pieces are checked by rules_exe_test and ui_exe_test.
        if (row.source == BTECH_START_STORY || row.source == BTECH_START_RULES || row.source == BTECH_START_UI) continue;
        const std::size_t at = BtechStartImageOffset(row.segment, row.offset);
        if (at + row.size > image.size()) {
            check(false, std::string(row.id) + " runs past the image");
            continue;
        }
        std::vector<std::uint8_t> bytes(row.size, 0);
        if (row.source == BTECH_START_CODE) std::memcpy(bytes.data(), row.bytes, row.size);
        if (row.source == BTECH_START_GENERATED) {
            const double margin = row.generate(bytes.data());
            check(margin > 1e-6, std::string(row.id) + ": an entry lies within 1e-6 of an integer");
        }
        const bool ok = std::memcmp(bytes.data(), image.data() + at, row.size) == 0;
        check(ok, std::string(row.id) + " differs from " + name);
        same += ok;
    }
    std::printf("%d BTECH start rows as %s's\n", same, name);
    return same;
}
}  // namespace

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
    std::printf("%d of %zu start constants as the exes'\n", same, mwNativeStartRowCount);

    // BTECH's start image: no two rows overlap, then every row against each
    // revision given.
    std::vector<std::pair<std::size_t, std::size_t>> spans;
    for (std::size_t i = 0; i < btechStartRowCount; ++i)
        spans.push_back({BtechStartImageOffset(btechStartRows[i].segment, btechStartRows[i].offset), btechStartRows[i].size});
    std::sort(spans.begin(), spans.end());
    for (std::size_t i = 1; i < spans.size(); ++i)
        check(spans[i - 1].first + spans[i - 1].second <= spans[i].first, "two BTECH start rows overlap");
    std::set<fs::path> btechs = {fs::weakly_canonical(find_file(game, "BTECH.EXE"))};
    for (const char* other : {"MW1O_BTECH_REFERENCE", "MW1O_BTECH_1990"})
        if (const char* path = std::getenv(other)) btechs.insert(fs::weakly_canonical(path));
    for (const fs::path& path : btechs) check_btech(path);
    std::printf("%s\n", g_failures ? "FAILED" : "ok");
    return g_failures ? 1 : 0;
}
