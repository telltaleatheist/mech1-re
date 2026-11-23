// BtechEgaCanvasWriter (src/Original/Video/BTECH_2A36_CANVAS, G3): 2A36's mode 2
// byte writes in pixels, against fake host hooks. A byte written after its own
// latch read takes its own pixels where the bit mask is 0; one written with no
// read (REP STOSB) takes the last read byte's. The register writes still reach
// the host. Plain checks: exit 0 when all hold.
#include "BTECH_2A36_CANVAS.h"

#include <cstdio>
#include <vector>

namespace {

int failures = 0;
void Check(bool ok, const char *what) {
  if (!ok) {
    std::printf("FAIL %s\n", what);
    ++failures;
  }
}

BtechEgaWriteState state;
std::vector<uint16_t> portWords;
int reads = 0;
void FakeState(BtechEgaWriteState *out) { *out = state; }
void FakePortWord(uint16_t port, uint16_t value) {
  if (port == COMBAT_EGA_GRAPHICS_PORT) portWords.push_back(value);
}
uint8_t FakeRead(uint16_t, uint16_t) {
  ++reads;
  return 0;
}

mw1o::video::Canvas &canvas = mw1o::video::GameCanvas();
bool Row(int page, int x, int y, std::vector<uint8_t> want) {
  for (size_t i = 0; i < want.size(); ++i)
    if (canvas.pixelAt(page, x + (int)i, y) != want[i]) return false;
  return true;
}

}  // namespace

int main() {
  btechHostReadEgaWriteState = FakeState;
  btechHostWritePortWord = FakePortWord;
  btechHostReadVideoByte = FakeRead;
  state = BtechEgaWriteState{2, 0, 0x0F, 0xFF, {}};
  canvas.clear();
  for (int i = 0; i < 16; ++i) canvas.pixel(1, i, 2, (uint8_t)(i & 15));  // row 2 of page 1: 0..15

  {
    BtechEgaCanvasWriter ega(COMBAT_EGA_BACK_PAGE);  // A200h: page 1
    ega.setMask(0xF0);
    Check(portWords.size() == 1 && portWords[0] == 0xF008, "the bit mask is written to the host");
    ega.load(80);  // row 2, x 0
    Check(reads == 1, "the latch read reaches the host");
    ega.write(80, 9);
    Check(Row(1, 0, 2, {9, 9, 9, 9, 4, 5, 6, 7}), "masked: the colour; unmasked: the byte's own pixels");
    ega.write(81, 9);  // no read: REP STOSB reuses the latches of byte 80
    Check(Row(1, 8, 2, {9, 9, 9, 9, 4, 5, 6, 7}), "a byte with no read takes the last read byte's pixels");
    ega.load(81);
    ega.setMask(0x0F);
    ega.write(81, 3);
    Check(Row(1, 8, 2, {9, 9, 9, 9, 3, 3, 3, 3}), "read again, its own pixels");
  }
  {
    // The function and the bit mask and latches inherited from before the path.
    state = BtechEgaWriteState{2, 3, 0x0F, 0x81, {1, 1, 1, 1, 1, 1, 1, 1}};
    BtechEgaCanvasWriter ega(COMBAT_EGA_FRONT_PAGE);
    ega.write(0, 6);  // no read in the path: the inherited latches, mask 81h, XOR
    Check(Row(0, 0, 0, {7, 1, 1, 1, 1, 1, 1, 7}), "inherited mask, latches and XOR");
    ega.load(0);
    ega.write(0, 6);
    Check(Row(0, 0, 0, {1, 1, 1, 1, 1, 1, 1, 1}), "XOR onto its own pixels");
  }
  {
    // Past row 199 into the next page, as the linear aperture runs.
    state = BtechEgaWriteState{2, 0, 0x0F, 0xFF, {}};
    BtechEgaCanvasWriter ega(COMBAT_EGA_FRONT_PAGE);
    ega.load(205 * 40);
    ega.write(205 * 40, 12);
    Check(canvas.page(1)[205 * 320 - mw1o::video::Canvas::PAGE_STRIDE] == 12, "row 205 of page 0 is page 1's");
  }
  std::printf("ega_canvas_writer: %s\n", failures ? "FAILED" : "ok");
  return failures ? 1 : 0;
}
