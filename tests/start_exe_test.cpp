// The program's start constants and file names (mechwarrior/native_start.h,
// docs/phase2/PLAN.md 8.3) and BTECH's start image (btech_start_image.h,
// 8.5, slice 6c) against the player's exes: every code, zero and generated
// row's bytes, as the code makes them, must be the exe's at the row's
// address, in each BTECH revision given. A generated table must also keep
// every computed entry clear of an integer before rounding down, so another
// libm cannot round it differently. MW_EGA's start state (ega_campaign.h,
// slice 6e) is compared byte by byte with MW_EGA.EXE, but for the ranges it
// leaves zero on purpose. A row that differs is named by id; the game's bytes
// are never printed.
//
// Needs the player's files, so ctest skips it (77) unless this is set:
//   MW1O_GAME             a game folder (MW_MAIN.EXE, MW.EXE, BTECH.EXE, MW_EGA.EXE)
//   MW1O_BTECH_REFERENCE  optional: another BTECH.EXE revision, checked too
//   MW1O_BTECH_1990       optional: the same
#include "native_exe.h"
#include "mechwarrior/btech_start_image.h"
#include "mechwarrior/combat_installation.h"
#include "mechwarrior/ega_campaign.h"
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

// The words past BTECH's file data that the importers take as zero: they are in no row, as the exe's file ends before
// them (304E:4EE0) and its startup clears them. The addresses are the reference's: nothing proves the 1990 exe keeps
// each variable at the same one, only that its clear covers them all.
struct ClearedWord {
    std::uint16_t at;
    const char* field;
};
const ClearedWord kClearedWords[] = {
    {0x4EEC, "orderScreenFullRedrawPending"}, {0x4EFC, "lanceOrderControls[0]"}, {0x4EFE, "lanceOrderControls[1]"},
    {0x4F00, "lanceOrderControls[2]"},        {0x4F2E, "orders 4F2E"},           {0x4F30, "orders 4F30"},
    {0x7750, "pose 7750"},                    {0x7754, "cockpitFullRedrawPending"}, {0x798C, "objectNeedsScreenClip"},
    {0x798D, "mirrorProjectionA"},            {0xC332, "render C332"},           {0xC34A, "render C34A"},
    {0xC358, "projectionMode10"},             {0xC35C, "pose C35C"},             {0xC8F8, "render C8F8"},
    {0xC944, "render C944"},                  {0xC946, "pose C946"},             {0xC94A, "useViewBasisDirectly"},
    {0xC953, "forceObjectScreenClip"},        {0xD1EE, "pose D1EE"},
};

// Whether the startup's clear (mov di,first / mov cx,end / sub cx,di / xor ax,ax / rep stosb) is in `image` once and
// clears every word above; what is wrong, or nothing.
std::string cleared_words_fault(const std::vector<std::uint8_t>& image) {
    int found = 0;
    std::uint16_t first = 0, end = 0;
    for (std::size_t i = 0; i + 12 <= image.size(); ++i) {
        const std::uint8_t* p = image.data() + i;
        if (p[0] != 0xBF || p[3] != 0xB9 || p[6] != 0x2B || p[7] != 0xCF || p[8] != 0x33 || p[9] != 0xC0 || p[10] != 0xF3 ||
            p[11] != 0xAA)
            continue;
        ++found;
        first = std::uint16_t(p[1] | p[2] << 8), end = std::uint16_t(p[4] | p[5] << 8);
    }
    if (found != 1) return "the startup's clear is there " + std::to_string(found) + " times";
    for (const ClearedWord& word : kClearedWords)
        if (word.at < first || word.at + 2 > end) return std::string(word.field) + " is outside the startup's clear";
    return {};
}

// Every code, zero and generated row of BTECH's start image against one revision.
int check_btech(const fs::path& path) {
    BtechRevision revision{};
    const std::vector<std::uint8_t> image = btech_image(path, &revision);
    check(!image.empty(), path.string() + " is not a known BTECH.EXE");
    if (image.empty()) return 0;
    const char* name = revision == BTECH_REVISION_REFERENCE ? "the reference BTECH" : "the 1990 BTECH";
    const std::string fault = cleared_words_fault(image);
    check(fault.empty(), fault + " in " + name);
    // The check's own check: a clear that ends before the last word, starts after the first, or isn't there.
    for (std::size_t i = 0; i + 12 <= image.size(); ++i) {
        if (image[i] != 0xBF || image[i + 3] != 0xB9 || image[i + 6] != 0x2B || image[i + 10] != 0xF3 || image[i + 11] != 0xAA)
            continue;
        auto changed = image;
        changed[i + 4] = 0xEF, changed[i + 5] = 0xD1;
        check(!cleared_words_fault(changed).empty(), "a clear up to D1EF passes");
        changed[i + 4] = 0xF0;
        check(cleared_words_fault(changed).empty(), "a clear up to D1F0 fails");
        changed = image;
        changed[i + 1] = 0xED, changed[i + 2] = 0x4E;
        check(!cleared_words_fault(changed).empty(), "a clear from 4EED passes");
        changed = image;
        changed[i + 11] = 0x90;
        check(!cleared_words_fault(changed).empty(), "an exe without the clear passes");
    }
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

    // MW_EGA's start state against the exe's image; the rest of the overlay's
    // 64K, past the file, starts zero (the exe's loader zero-filled it).
    const std::vector<std::uint8_t> ega = load_image(find_file(game, "MW_EGA.EXE"));
    check(!ega.empty() && ega.size() <= 0x10000, "MW_EGA.EXE is not a DOS program");
    if (!ega.empty() && ega.size() <= 0x10000) {
        EgaLoadStartState();
        std::size_t same = 0, zeroed = 0;
        for (std::uint32_t at = 0; at < 0x10000; ++at) {
            bool skipped = false;
            for (std::size_t i = 0; i < egaStartZeroedRangeCount; ++i)
                skipped |= at >= egaStartZeroedRanges[i].offset &&
                           at < std::uint32_t(egaStartZeroedRanges[i].offset) + egaStartZeroedRanges[i].byteCount;
            const std::uint8_t expected = skipped || at >= ega.size() ? 0 : ega[at];
            if (EgaReadOverlayByte(std::uint16_t(at)) == expected) {
                ++same;
                zeroed += skipped;
            } else {
                char what[64];
                std::snprintf(what, sizeof what, "MW_EGA overlay byte %04X differs", unsigned(at));
                check(false, what);
            }
        }
        std::printf("%zu of 65536 MW_EGA overlay bytes as the exe's (%zu left zero on purpose)\n", same, zeroed);
    }
    std::printf("%s\n", g_failures ? "FAILED" : "ok");
    return g_failures ? 1 : 0;
}
