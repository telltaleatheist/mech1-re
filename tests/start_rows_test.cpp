// The reader of combat's start rows (btech_start_image.h): each row's bytes by its id, and every stop, by its message.
// No game file is needed. Argument: the data folder.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include "mechwarrior/btech_start_image.h"
#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/rules.h"
#include "mechwarrior/ui_layout.h"

namespace {
struct Refused {
  std::string reason;
};
int failed = 0;

template <class Call>
void Stops(Call call, const std::string &want) {
  std::string got = "nothing";
  try {
    call();
  } catch (const Refused &r) {
    got = r.reason;
  }
  const std::string whole = "Combat's start data: " + want + ". This is a fault of the program";
  std::printf("  %s\n", got.c_str());
  if (got == whole) return;
  std::printf("FAIL: wanted \"%s\"\n", whole.c_str());
  ++failed;
}
void Check(bool ok, const char *what) {
  if (ok) return;
  std::printf("FAIL: %s\n", what);
  ++failed;
}
const uint8_t kBytes[5] = {'A', 'B', 0, 'C', 'D'};
}  // namespace

[[noreturn]] void MwAbortWithReason(const char *, const char *reason, const char *, int, const char *) {
  throw Refused{reason};
}

int main(int argc, char **argv) {
  if (argc != 2) return 2;
  // Before the pieces are loaded, every row from one is missing.
  Stops([] { BtechStartRowData row("weapons.projectiles"); }, "the row \"weapons.projectiles\" has its rule piece not loaded");
  Stops([] { BtechStartRowData row("orders.map_scale_shifts"); }, "the row \"orders.map_scale_shifts\" has its layout piece not loaded");
  Stops([] { BtechStartRowData row("runtime.messages"); }, "the row \"runtime.messages\" has no story span of 210 bytes");
  Check(MwHostPrepareRules(argv[1]) && MwHostPrepareUi(argv[1]), "the data folder loads");

  // Every row that needs no story reads whole and no further, and holds its source's bytes.
  int rows = 0;
  for (size_t i = 0; i < btechStartRowCount; ++i) {
    const BtechStartRow &row = btechStartRows[i];
    if (row.source == BTECH_START_STORY) continue;
    const BtechStartRowData data(row.id);
    const uint8_t *bytes = data.bytes(0, row.size);
    if (row.source == BTECH_START_CODE) Check(!std::memcmp(bytes, row.bytes, row.size), row.id);
    if (row.source == BTECH_START_ZERO)
      for (uint16_t at = 0; at < row.size; ++at) Check(!bytes[at], row.id);
    bool stopped = false;
    try {
      data.byte(row.size);
    } catch (const Refused &) {
      stopped = true;
    }
    Check(stopped, row.id);
    ++rows;
  }
  std::printf("%d rows read\n", rows);

  // A piece shorter and longer than its row, and one that isn't there.
  Stops([] { BtechStartRowData row(BtechStartRow{"weapons.projectiles", 0x304E, 0x0E6E, 0x29, BTECH_START_RULES, nullptr, nullptr}); },
        "the row \"weapons.projectiles\" has a rule piece of 40 bytes, not 41");
  Stops([] { BtechStartRowData row(BtechStartRow{"weapons.projectiles", 0x304E, 0x0E6E, 0x27, BTECH_START_RULES, nullptr, nullptr}); },
        "the row \"weapons.projectiles\" has a rule piece of 40 bytes, not 39");
  Stops([] { BtechStartRowData row(BtechStartRow{"weapons.none", 0x304E, 0, 4, BTECH_START_RULES, nullptr, nullptr}); },
        "the row \"weapons.none\" has no rule piece");
  Stops([] { BtechStartRowData row(BtechStartRow{"orders.none", 0x304E, 0, 4, BTECH_START_UI, nullptr, nullptr}); },
        "the row \"orders.none\" has no layout piece");
  Stops([] { BtechStartRowData row("no.such_row"); }, "the row \"no.such_row\" does not exist");

  // Reads: inside, up to the end, across it and past it.
  static const BtechStartRow kRow{"test.row", 0x304E, 0x0100, sizeof(kBytes), BTECH_START_CODE, kBytes, nullptr};
  const BtechStartRowData row(kRow);
  Check(row.byte() == 'A' && row.byte(4) == 'D' && row.word(3) == ('C' | 'D' << 8) && row.word() == ('A' | 'B' << 8), "bytes and words");
  Check(row.dword(1) == ('B' | 0 << 8 | 'C' << 16 | uint32_t('D') << 24), "dword");
  uint8_t copied[5];
  row.copy(copied, 5);
  Check(!std::memcmp(copied, kBytes, 5), "copy");
  Check(row.holds(0x100) && row.holds(0x104) && !row.holds(0xFF) && !row.holds(0x105) && row.index(0x103) == 3, "near offsets");
  Check(!std::strcmp(row.text(0), "AB") && !std::strcmp(row.text(1), "B") && !*row.text(2), "text");
  Stops([&] { row.byte(5); }, "the row \"test.row\" is 5 bytes, read at 5 (1 bytes)");
  Stops([&] { row.word(4); }, "the row \"test.row\" is 5 bytes, read at 4 (2 bytes)");
  Stops([&] { row.dword(2); }, "the row \"test.row\" is 5 bytes, read at 4 (2 bytes)");
  Stops([&] { row.copy(copied, 5, 1); }, "the row \"test.row\" is 5 bytes, read at 1 (5 bytes)");
  Stops([&] { row.bytes(SIZE_MAX, 2); }, "the row \"test.row\" is 5 bytes, read at 18446744073709551615 (2 bytes)");
  Stops([&] { row.index(0x105); }, "the row \"test.row\" (304E:0100) does not hold 0105");
  Stops([&] { row.text(3); }, "the row \"test.row\" holds a text without its end");
  Stops([&] { row.text(5); }, "the row \"test.row\" is 5 bytes, read at 5 (1 bytes)");
  std::printf("start_rows: %s\n", failed ? "FAILED" : "ok");
  return failed != 0;
}
