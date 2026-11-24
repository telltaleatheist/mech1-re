// Combat's file records (docs/phase5/NO_EMULATION_PLAN.md, B4e): a file that ends early is refused by name, and a whole
// one is read as before. The files are the test's own, cut where each check says. With MW1O_GAME set, the game's own
// packed banks are cut too (copies, in the work folder): a cut is refused, or leaves the bank's bytes as they were.
// Plain checks, no framework: exit 0 when all hold. The only argument is the work folder.
#include "mechwarrior/btech_start_image.h"
#include "mechwarrior/combat_heap.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_platform.h"
#include "mechwarrior/combat_resources.h"
#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_world.h"
#include "mechwarrior/port_diagnostics.h"
#include "Support/Sha256.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

// The library's stop, as an exception: the only symbol in its object, so the linker takes this one.
struct Refused {
  std::string reason;
};
[[noreturn]] void MwAbortWithReason(const char *, const char *reason, const char *, int, const char *) {
  throw Refused{reason};
}

namespace {

namespace fs = std::filesystem;

int failures = 0;
void Check(bool ok, const std::string &what) {
  if (!ok) {
    std::printf("FAIL %s\n", what.c_str());
    ++failures;
  }
}

fs::path work;
using Bytes = std::vector<uint8_t>;

void Write(const std::string &name, const Bytes &bytes) {
  std::ofstream(work / name, std::ios::binary).write(reinterpret_cast<const char *>(bytes.data()), (std::streamsize)bytes.size());
}
FILE *OpenWorkFile(const char *name, const char *mode) { return std::fopen((work / name).string().c_str(), mode); }

std::vector<uint8_t *> blocks;
uint8_t *AllocateParagraphs(uint16_t paragraphs, uint16_t *largest) {
  *largest = paragraphs;
  blocks.push_back(static_cast<uint8_t *>(std::malloc((size_t)paragraphs * 16 + 16)));
  return blocks.back();
}
void FreeAllocation(uint8_t *bytes) {
  for (uint8_t *&block : blocks)
    if (bytes && block == bytes) std::free(block), block = nullptr;
}

// A refusal leaves its files and archives open: put combat's tables back as a fresh start has them.
void Reset() {
  BtechCloseRuntimeStreamFiles();
  for (size_t i = 0; i < btechStartRowCount; ++i) {
    const BtechStartRow &row = btechStartRows[i];
    if (std::strcmp(row.id, "runtime.streams") != 0) continue;
    for (uint16_t slot = 0; slot < COMBAT_STREAM_SLOTS; ++slot)
      combatSound.runtimeStreams[slot] = BtechRuntimeStream{nullptr, row.bytes[slot * COMBAT_STREAM_STRIDE], nullptr};
  }
  BtechHostDiscardNearHeap();
  for (auto &descriptor : combatResources.archiveDescriptors) descriptor = nullptr;
  combatResources.selectedArchive = nullptr;
  combatResources.archiveIoFlags = 0;
  for (uint8_t *block : blocks) std::free(block);
  blocks.clear();
}

// What the call was refused for, without the advice; or nothing.
const std::string kAdvice =
    " Your copy of the game's files is damaged: run mw1r --setup and pick a good copy (not the one it finds in use).";
template <class Call>
std::string RefusedFor(Call call) {
  Reset();
  try {
    call();
  } catch (const Refused &refused) {
    const std::string &r = refused.reason;
    if (r.size() > kAdvice.size() && !r.compare(r.size() - kAdvice.size(), kAdvice.size(), kAdvice))
      return r.substr(0, r.size() - kAdvice.size());
    return "NO ADVICE: " + r;
  }
  return {};
}
void Expect(const std::string &got, const std::string &want) { Check(got == want, "wanted \"" + want + "\", got \"" + got + "\""); }

Bytes Filled(size_t count) {
  Bytes bytes(count);
  for (size_t i = 0; i < count; ++i) bytes[i] = (uint8_t)(i * 7 + 1);
  return bytes;
}

void Scenario() {
  Write("snario.dat", Filled(12 * 16 + 6));
  Expect(RefusedFor([] { BtechLoadScenarioAndWorld(12); }), "snario.dat ends inside the selectors of scenario 12 (6 of 16 bytes).");
  Write("snario.dat", Filled(5 * 16));
  Expect(RefusedFor([] { BtechLoadScenarioAndWorld(12); }), "snario.dat ends inside the selectors of scenario 12 (0 of 16 bytes).");

  Write("snario.dat", Filled(20));
  FILE *file = OpenWorkFile("snario.dat", "rb");
  Expect(RefusedFor([&] { BtechReadUnitFormations(file, 1); }), "snario.dat ends inside the formation of side 1 (20 of 32 bytes).");
  std::fclose(file);
  Write("snario.dat", Filled(3));
  file = OpenWorkFile("snario.dat", "rb");
  Expect(RefusedFor([&] { BtechReadSpecialTargetPosition(file, 9); }), "snario.dat ends inside the target of terrain 9 (3 of 8 bytes).");
  std::fclose(file);

  // Whole: two sides' units, then a target.
  const Bytes whole = Filled(32 + 32 + 8);
  auto dword = [&](size_t at) { return whole[at] | whole[at + 1] << 8 | whole[at + 2] << 16 | (uint32_t)whole[at + 3] << 24; };
  Write("snario.dat", whole);
  file = OpenWorkFile("snario.dat", "rb");
  BtechPosition3D target = {};
  Expect(RefusedFor([&] {
    BtechReadUnitFormations(file, 0);
    BtechReadUnitFormations(file, 1);
    target = BtechReadSpecialTargetPosition(file, 0);
  }), "");
  std::fclose(file);
  for (size_t unit = 0; unit < 8; ++unit)
    Check(combatWorld.unitFormationPositions[unit].positionA == dword(unit * 8) &&
              combatWorld.unitFormationPositions[unit].positionB == dword(unit * 8 + 4), "a unit's formation");
  Check(target.positionA == dword(64) && target.positionB == dword(68), "the target's position");
}

void Terrain() {
  enum { GRID_BYTES = TERRAIN_TILE_ROWS * TERRAIN_TILE_COLUMNS };
  combatImport.combatTerrainDetailLevel = 2;
  combatWorld.scenarioSelectors = BtechScenarioSelectors{};
  std::strcpy(combatWorld.terrainVariantFilenameSuffixes[0], "0");
  for (uint16_t tile = 0; tile < 4; ++tile) combatWorld.terrainTileGridRowBases[tile] = combatWorld.terrainTileGridColumnBases[tile] = 0;
  auto load = [](size_t world, size_t grid) {
    Write("TILE0.WLD", Filled(world));
    Write("TILE0.GRD", Filled(grid));
    combatWorld.worldObjectCount = 0;
    return RefusedFor([] { BtechLoadTerrainWorldResources(); });
  };
  Expect(load(1, GRID_BYTES), "TILE0.WLD ends inside object 0 (1 of 14 bytes).");
  Expect(load(9, GRID_BYTES), "TILE0.WLD ends inside object 0 (9 of 14 bytes).");
  Expect(load(0, 3 * TERRAIN_TILE_COLUMNS + 5), "TILE0.GRD ends inside row 3 (5 of 47 bytes).");
  Expect(load(0, GRID_BYTES - 1), "TILE0.GRD ends inside row 86 (46 of 47 bytes).");
  Expect(load(0, GRID_BYTES), "");
  const Bytes want = Filled(GRID_BYTES);
  bool same = true;
  for (size_t row = 0; row < TERRAIN_TILE_ROWS; ++row)
    same = same && !std::memcmp(combatWorld.terrainHeightGrid[row], &want[row * TERRAIN_TILE_COLUMNS], TERRAIN_TILE_COLUMNS);
  Check(same, "the whole grid");
}

// A packed file (method 0, not packed) whose header says `says` bytes.
Bytes Raw(const Bytes &body, uint32_t says) {
  Bytes file = {0, (uint8_t)says, (uint8_t)(says >> 8), (uint8_t)(says >> 16), (uint8_t)(says >> 24)};
  file.insert(file.end(), body.begin(), body.end());
  return file;
}
std::string LoadBank(const Bytes &file) {
  Write("bank.tbl", file);
  return RefusedFor([] { (void)BtechDecodeAndRelocateResourceBank("bank.tbl"); });
}
std::string LoadSurfaces(const Bytes &file) {
  Write("bank.gi", file);
  return RefusedFor([] { BtechLoadCollisionResourceBank("bank.gi"); });
}

void Banks() {
  const Bytes empty(16);  // a directory with its end only
  Expect(LoadBank(Raw(empty, 16)), "");
  Expect(LoadSurfaces(Raw(empty, 16)), "");
  Expect(LoadBank(Raw(Bytes(10), 16)), "bank.tbl unpacks to less than its header says.");
  Expect(LoadBank(Raw(Bytes(20), 16)), "bank.tbl unpacks to more than its header says.");
  for (size_t cut = 0; cut < 5; ++cut) {
    Bytes file = Raw(empty, 16);
    file.resize(cut);
    Expect(LoadBank(file), "bank.tbl ends inside its packing header.");
  }
  Bytes open(16, 1);  // a directory that never ends, its pairs far outside
  Expect(LoadBank(Raw(open, 16)), "bank.tbl has the directory's end outside the file.");
  Expect(LoadSurfaces(Raw(open, 16)), "bank.gi has the directory's end outside the file.");
  Bytes away(16);
  away[0] = 8;  // one shape at offset 8: its 14 bytes don't fit in 16
  Expect(LoadBank(Raw(away, 16)), "bank.tbl has a shape's header outside the file.");
  Expect(LoadSurfaces(Raw(away, 16)), "bank.gi has a surface's header outside the file.");
  away = Bytes(48);
  away[0] = 8;
  away[8 + 0x0A] = 1;                       // one entry
  away[8 + 0x0C] = 44;                      // at 44: its 8 bytes don't fit in 48
  Expect(LoadBank(Raw(away, 48)), "bank.tbl has a shape's entry outside the file.");
  away = Bytes(48);
  away[0] = 8, away[8 + 7] = 1, away[8 + 8] = 40;  // one patch at 40: its 16 bytes don't fit in 48
  Expect(LoadSurfaces(Raw(away, 48)), "bank.gi has a surface's patch outside the file.");
}

// The 1989 copy's PHAPCK.TBL is damaged: from 1800h to 6000h it holds MARPCK.TBL's bytes.
const char *const kDamagedPhoenixHawk = "d31595562474ed3ba6a8a4360116789f47b9a7392e8d02a527939ae57b7a4f76";

// The game's packed banks, cut: refused, or the bytes as they were. Skipped without the game.
void GameBanks() {
  const char *game = std::getenv("MW1O_GAME");
  if (!game) {
    std::printf("  MW1O_GAME not set: the game's banks not cut\n");
    return;
  }
  size_t banks = 0, cuts = 0, refused = 0, same = 0, different = 0;
  for (const auto &entry : fs::directory_iterator(game)) {
    std::string extension = entry.path().extension().string();
    for (char &c : extension) c = (char)std::toupper((unsigned char)c);
    if (extension != ".TBL" && extension != ".GI") continue;
    std::ifstream in(entry.path(), std::ios::binary);
    const Bytes whole((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    auto decode = [](const Bytes &file, Bytes *out) {
      Write("cut.tbl", file);
      return RefusedFor([out] {
        const BtechResourceAllocation allocation = BtechAllocateAndDecodeResource("cut.tbl");
        out->assign(allocation.bytes, allocation.bytes + allocation.byteCount);
      });
    };
    Bytes want, got;
    if (mw1o::support::sha256_hex(whole) == kDamagedPhoenixHawk) {
      Expect(decode(whole, &want), "cut.tbl unpacks to less than its header says.");
      std::printf("  %s: the damaged 1989 copy, refused whole\n", entry.path().filename().string().c_str());
      continue;
    }
    Expect(decode(whole, &want), "");
    ++banks;
    for (size_t size = 0; size < whole.size(); ++size) {
      if (size > 8 && size + 64 < whole.size() && size % 257) continue;
      ++cuts;
      const std::string why = decode(Bytes(whole.begin(), whole.begin() + (std::ptrdiff_t)size), &got);
      if (!why.empty()) ++refused;
      else if (got == want) ++same;
      else {
        ++different;
        std::printf("  %s cut to %zu of %zu bytes: accepted, not the same\n", entry.path().filename().string().c_str(), size, whole.size());
      }
    }
  }
  std::printf("  %zu banks, %zu cuts: %zu refused, %zu the same bytes, %zu accepted and different\n", banks, cuts, refused, same, different);
  Check(banks > 0, "banks in MW1O_GAME");
  Check(different == 0, "no cut bank is accepted with other bytes");
}

}  // namespace

int main(int argc, char **argv) {
  if (argc != 2) return std::printf("usage: combat_records_test WORK\n"), 2;
  work = argv[1];
  fs::create_directories(work);
  btechHostOpenStream = OpenWorkFile;
  btechHostAllocateParagraphs = AllocateParagraphs;
  btechHostFreeDosAllocation = FreeAllocation;
  Scenario();
  Terrain();
  Banks();
  GameBanks();
  std::printf("combat_records: %s\n", failures ? "FAILED" : "ok");
  return failures ? 1 : 0;
}
