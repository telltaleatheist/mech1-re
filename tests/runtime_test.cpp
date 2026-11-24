// The C runtime's rules kept by B5 (docs/phase5/NO_EMULATION_PLAN.md): the combat near heap's exhaustion, decided by
// 0810's allocator over its native sizes (not a byte budget: fragmentation can decide a failure), and the 1989 signed
// long divide (its rounding is game maths; it equals C's truncating division, and the minimum dword over -1 wraps).
// Combat's open files: which slot of the table each takes (its address goes into the checksum that seeds the combat
// random mix), and the 512-byte buffer charged to the near heap at the first read, never at the open.
// Plain checks, no framework: exit 0 when all hold.
#include "mechwarrior/btech_start_image.h"
#include "mechwarrior/combat_heap.h"
#include "mechwarrior/combat_platform.h"
#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_runtime.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <random>

namespace {

int failures = 0;
void Check(bool ok, const char *what) {
  if (!ok) {
    std::printf("FAIL %s\n", what);
    ++failures;
  }
}

void *Near(uint16_t bytes) { return BtechAllocateNearBytes(bytes, bytes ? bytes : 1, BTECH_NEAR_BYTES); }

// The heap's room: from the program break (BTECH_NEAR_INITIAL_BREAK) to the segment's end, as the allocator grows it.
void Exhaustion() {
  BtechHostDiscardNearHeap();
  // The bitmap workspace (0x3AC4 bytes) never fits: every combat's screens ask for it and fall back (step 0).
  Check(!Near(0x3AC4), "the bitmap workspace fails on a fresh heap");
  // The most a fresh heap grants in one block, and one byte more.
  uint16_t most = 0;
  for (uint32_t bytes = 1; bytes < 0x2000; ++bytes) {
    BtechHostDiscardNearHeap();
    if (Near((uint16_t)bytes)) most = (uint16_t)bytes;
  }
  std::printf("  the largest block on a fresh heap: %u bytes\n", most);
  BtechHostDiscardNearHeap();
  Check(most > 7000 && Near(most) != nullptr, "the largest block granted");
  BtechHostDiscardNearHeap();
  Check(Near((uint16_t)(most + 1)) == nullptr, "a byte more refused");
  // E9C6: a count times stride past a word is refused before the heap.
  BtechHostDiscardNearHeap();
  Check(BtechAllocateZeroedNearElements(0x100, 0x101, 16, BTECH_NEAR_BYTES) == nullptr, "a zeroed request past a word");
  uint8_t *zeroed = static_cast<uint8_t *>(BtechAllocateZeroedNearElements(4, 8, 32, BTECH_NEAR_BYTES));
  bool allZero = zeroed != nullptr;
  for (int i = 0; zeroed && i < 32; ++i) allZero = allZero && zeroed[i] == 0;
  Check(allZero, "a zeroed request is zeroed");
}

// Fragmentation decides: seven blocks, every other one released. The free bytes and the room left would hold a request
// a byte budget grants, but no free run is long enough; releasing the block between two free ones joins them (on demand,
// when the search walks them) and the request fits.
void Fragmentation() {
  BtechHostDiscardNearHeap();
  void *blocks[7] = {};
  for (void *&b : blocks) b = Near(1000);
  bool all = true;
  for (void *b : blocks) all = all && b != nullptr;
  Check(all, "seven blocks of 1000");
  BtechReleaseNearHeapBlock(blocks[1]);
  BtechReleaseNearHeapBlock(blocks[3]);
  BtechReleaseNearHeapBlock(blocks[5]);
  Check(Near(2000) == nullptr, "2000 bytes refused: three free runs of 1000, none long enough");
  BtechReleaseNearBlockAndProbeAllocation(blocks[2]);  // the probe resets the search cursor to the first block
  Check(Near(2000) != nullptr, "2000 bytes granted once the runs either side of a released block join");
}

// A released block's object stays readable until the heap is discarded (the sound code may still walk a released node;
// it keeps its own last bytes).
void ReleasedStaysReadable() {
  BtechHostDiscardNearHeap();
  uint8_t *node = static_cast<uint8_t *>(Near(16));
  for (int i = 0; i < 16; ++i) node[i] = (uint8_t)(0xA0 + i);
  BtechReleaseNearHeapBlock(node);
  void *reused = Near(16);  // the same place in the heap: a new object
  Check(reused != node, "a new allocation is a new object");
  bool kept = true;
  for (int i = 0; i < 16; ++i) kept = kept && node[i] == (uint8_t)(0xA0 + i);
  Check(kept, "the released object keeps its bytes");
}

struct DivideCase {
  uint32_t numerator, denominator;
  int32_t quotient, remainder;
};
// Pinned from the original's algorithm (main before B5 gives the same on two million random cases and these).
const DivideCase kDivides[] = {
    {0x80000000u, 0x00000001u, -2147483647 - 1, 0}, {0x80000000u, 0xFFFFFFFFu, -2147483647 - 1, 0},
    {0x80000000u, 0x00010000u, -32768, 0},           {0x7FFFFFFFu, 0x00010000u, 32767, 65535},
    {0x80000000u, 0x80000000u, 1, 0},                {0x00000001u, 0x80000000u, 0, 1},
    {0xFFFFFFFFu, 0x80000000u, 0, -1},               {0x7FFFFFFFu, 0xFFFF0001u, -32768, 32767},
    {0x00000000u, 0x12345678u, 0, 0},                {0x00000064u, 0x00000007u, 14, 2},
    {0xFFFFFF9Cu, 0x00000007u, -14, -2},             {0x00000064u, 0xFFFFFFF9u, -14, 2},
    {0xFFFFFF9Cu, 0xFFFFFFF9u, 14, -2},              {0x000F4240u, 0x00011170u, 14, 20000},
    {0xFFF0BDC0u, 0x00011170u, -14, -20000},
};

void LongDivide() {
  for (const DivideCase &c : kDivides) {
    runtime_1989::LongDivision r = runtime_1989::signed_long_divide(c.numerator, c.denominator);
    if (r.quotient != c.quotient || r.remainder != c.remainder) {
      std::printf("  %08X / %08X: %d r %d, expected %d r %d\n", c.numerator, c.denominator, r.quotient, r.remainder,
                  c.quotient, c.remainder);
      Check(false, "a pinned long divide");
    }
  }
  // Narrow (a 16-bit divisor) and wide: C's truncating division, quotient and remainder.
  std::mt19937_64 rng(5);
  int differ = 0;
  for (int i = 0; i < 200000; ++i) {
    int32_t n = (int32_t)rng(), d = (int32_t)(i % 2 ? rng() : (rng() & 0xFFFF));
    if (!d || n == INT32_MIN) continue;
    runtime_1989::LongDivision r = runtime_1989::signed_long_divide((uint32_t)n, (uint32_t)d);
    if (r.quotient != n / d || r.remainder != n % d) ++differ;
  }
  Check(differ == 0, "the long divide truncates as C does");
}

const char *const kStreamFile = "runtime_test_stream.bin";
FILE *OpenTestFile(const char *, const char *mode) { return std::fopen(kStreamFile, mode); }

// The table as the start image has it: its "runtime.streams" row begins at slot 0's flags (304E:4BC6).
bool StartImageStreams() {
  for (size_t i = 0; i < btechStartRowCount; ++i) {
    const BtechStartRow &row = btechStartRows[i];
    if (std::strcmp(row.id, "runtime.streams") != 0) continue;
    for (uint16_t slot = 0; slot < COMBAT_STREAM_SLOTS; ++slot)
      runtimeStreams[slot] = BtechRuntimeStream{nullptr, row.bytes[slot * COMBAT_STREAM_STRIDE], nullptr};
    return true;
  }
  return false;
}

void Streams() {
  btechHostOpenStream = OpenTestFile;
  if (FILE *f = std::fopen(kStreamFile, "wb")) std::fputs("combat stream", f), std::fclose(f);
  Check(StartImageStreams(), "the start image's streams row");
  BtechHostDiscardNearHeap();

  // Slot order: the image's five standard streams stay, a file takes the lowest free slot, a closed one's is reused.
  BtechRuntimeStream *first = BtechRuntimeStreamOpen("A", "rb"), *second = BtechRuntimeStreamOpen("B", "rb");
  Check(first && BtechStreamIdentity(first) == 0x4BE8, "the first file opened is at 4BE8 (slot 5)");
  Check(second && BtechStreamIdentity(second) == 0x4BF0, "the second at 4BF0");
  if (!first || !second) return;
  Check(BtechRuntimeStreamClose(first) == 0, "a file closes");
  BtechRuntimeStream *again = BtechRuntimeStreamOpen("C", "rb");
  Check(again && BtechStreamIdentity(again) == 0x4BE8, "the next open takes the closed file's slot");

  // The table's fifteen free slots, then a refusal; a file that can't be opened takes none.
  for (int i = 2; i < 15; ++i) Check(BtechRuntimeStreamOpen("D", "rb") != nullptr, "fifteen files open");
  Check(BtechRuntimeStreamOpen("E", "rb") == nullptr, "a sixteenth is refused");
  Check(BtechStreamIdentity(&runtimeStreams[COMBAT_STREAM_SLOTS - 1]) == 0x4C58, "the last slot is at 4C58");
  BtechCloseRuntimeStreamFiles();
  Check(StartImageStreams(), "the table again");

  // The buffer: not at the open, at the first read; released at the close. The largest block a fresh heap grants
  // tells them apart.
  const uint16_t kMost = 7774;
  BtechHostDiscardNearHeap();
  BtechRuntimeStream *stream = BtechRuntimeStreamOpen("F", "rb");
  if (!stream) { Check(false, "a file opens"); return; }
  Check(!stream->buffer && Near(kMost) != nullptr, "an open charges nothing");
  BtechHostDiscardNearHeap();
  Check(BtechReadStreamByte(stream->file) == 'c' && stream->buffer, "the first read takes the buffer");
  Check(Near(kMost) == nullptr, "which is charged to the near heap");
  uint8_t *buffer = stream->buffer;
  Check(BtechReadStreamByte(stream->file) == 'o' && stream->buffer == buffer, "the next read keeps it");
  Check(BtechRuntimeStreamClose(stream) == 0 && !stream->buffer && Near(500) != nullptr, "the close releases it");

  // A full heap: the read still works, unbuffered.
  BtechHostDiscardNearHeap();
  Check(Near(kMost) != nullptr, "the heap filled");
  stream = BtechRuntimeStreamOpen("G", "rb");
  Check(stream && BtechReadStreamByte(stream->file) == 'c' && !stream->buffer && (stream->flags & COMBAT_STREAM_UNBUFFERED),
        "a read on a full heap is unbuffered");
  BtechCloseRuntimeStreamFiles();
  std::remove(kStreamFile);
}

}  // namespace

int main() {
  Exhaustion();
  Fragmentation();
  ReleasedStaysReadable();
  LongDivide();
  Streams();
  BtechHostDiscardNearHeap();
  if (failures) std::printf("%d failed\n", failures);
  else std::printf("runtime: all hold\n");
  return failures ? 1 : 0;
}
