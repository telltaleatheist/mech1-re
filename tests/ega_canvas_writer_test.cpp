// Combat's drawing on the canvas (src/Original/Video/BTECH_2A36_CANVAS, G3-G4):
// the pen (colour where the mask has a 1), eight pixels set or copied, and the
// game's planar form (one colour bit of eight pixels per byte). Plain checks:
// exit 0 when all hold.
#include "BTECH_2A36_CANVAS.h"
#include "mechwarrior/port_diagnostics.h"

#include <cstdio>
#include <string>
#include <vector>

// The library's stop, as an exception: it is the only symbol in its object, so
// the linker takes this one and a refusal can be checked.
struct Refused {
  std::string reason;
};
[[noreturn]] void MwAbortWithReason(const char *, const char *reason, const char *, int, const char *) {
  throw Refused{reason};
}

namespace {

int failures = 0;
void Check(bool ok, const char *what) {
  if (!ok) {
    std::printf("FAIL %s\n", what);
    ++failures;
  }
}

// What the call was refused for, or nothing.
template <class Call>
std::string RefusedFor(Call call) {
  try {
    call();
  } catch (const Refused &refused) {
    return refused.reason;
  }
  return {};
}

mw1o::video::Canvas &canvas = mw1o::video::GameCanvas();
bool Row(int page, int x, int y, std::vector<uint8_t> want) {
  for (size_t i = 0; i < want.size(); ++i)
    if (canvas.pixelAt(page, x + (int)i, y) != want[i]) return false;
  return true;
}

}  // namespace

int main() {
  canvas.clear();
  for (int i = 0; i < 16; ++i) canvas.pixel(1, i, 2, (uint8_t)i);  // row 2 of page 1: 0..15

  // The pen: a byte offset of eight pixels from the page's start, row 2 at 80.
  {
    BtechEgaCanvasWriter ega(CombatPage::Second);  // A200h: page 1
    ega.setMask(0xF0);
    ega.write(80, 9);
    Check(Row(1, 0, 2, {9, 9, 9, 9, 4, 5, 6, 7}), "the colour where the mask has a 1, the rest as it was");
    ega.write(81, 0x1C);  // the same mask again; a colour's low four bits
    Check(Row(1, 8, 2, {12, 12, 12, 12, 12, 13, 14, 15}), "the mask kept between writes");
    ega.setMask(0x55);
    ega.write(80, 3);
    Check(Row(1, 0, 2, {9, 3, 9, 3, 4, 3, 6, 3}), "a dithered mask");
  }
  // Eight pixels set, copied between pages; the scratch and the pages.
  {
    const uint8_t colours[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    BtechEgaCanvasSetPixels(BtechEgaCanvasPage(CombatPage::BitmapScratch), 41, colours);
    Check(BtechEgaCanvasPage(CombatPage::BitmapScratch) == 3 && Row(3, 8, 1, {1, 2, 3, 4, 5, 6, 7, 8}),
          "eight colours on the scratch page (A600h, page 3), byte 41 at (8, 1)");
    BtechEgaCanvasCopy(3, 0, 41);
    Check(Row(0, 8, 1, {1, 2, 3, 4, 5, 6, 7, 8}), "copied to page 0");
  }
  // The planar form: colours 1..8 are bits 0..3 of each pixel.
  {
    Check(BtechEgaCanvasColourBits(0, 41, 0) == 0xAA, "bit 0 of 1..8");    // 1,3,5,7
    Check(BtechEgaCanvasColourBits(0, 41, 1) == 0x66, "bit 1 of 1..8");    // 2,3,6,7
    Check(BtechEgaCanvasColourBits(0, 41, 2) == 0x1E, "bit 2 of 1..8");    // 4,5,6,7
    Check(BtechEgaCanvasColourBits(0, 41, 3) == 0x01, "bit 3 of 1..8");    // 8
    // Writing every bit of a byte back reproduces it (capture then restore).
    uint8_t planes[4];
    for (int bit = 0; bit < 4; ++bit) planes[bit] = BtechEgaCanvasColourBits(0, 41, bit);
    canvas.clear();
    for (int bit = 3; bit >= 0; --bit) BtechEgaCanvasSetColourBits(0, 41, bit, planes[bit]);
    Check(Row(0, 8, 1, {1, 2, 3, 4, 5, 6, 7, 8}), "the four colour bits written back");
    BtechEgaCanvasSetColourBits(0, 41, 0, 0x00);
    Check(Row(0, 8, 1, {0, 2, 2, 4, 4, 6, 6, 8}), "one colour bit cleared, the others kept");
    BtechEgaCanvasAddColourBits(0, 41, 3, 0x81);
    Check(Row(0, 8, 1, {8, 2, 2, 4, 4, 6, 6, 8}), "a colour bit added where the bits have a 1");
  }
  // Past row 199 into the next page, as the game's unchecked drawing runs.
  {
    BtechEgaCanvasWriter ega(CombatPage::First);
    ega.setMask(0xFF);
    ega.write(205 * 40, 12);
    Check(canvas.page(1)[205 * 320 - mw1o::video::Canvas::PAGE_STRIDE] == 12, "row 205 of page 0 is page 1's");
  }
  // No page: refused at the first byte drawn, not before.
  {
    BtechEgaCanvasWriter ega(CombatPage::None);
    ega.setMask(0xFF);
    Check(RefusedFor([&] { ega.write(0, 1); }) == "Rejected condition: a drawing on no page", "no page refused");
    Check(RefusedFor([] { BtechEgaCanvasPage(CombatPage::None); }) == "Rejected condition: a drawing on no page",
          "no page has no number");
  }
  // A captured or restored word at the segment's last byte carries past the pages.
  {
    const std::string carried = "Rejected condition: a captured word carries past the canvas's pages";
    uint8_t bytes[8] = {};
    BtechResourceAllocation buffer = {bytes, sizeof(bytes)};
    combatVideo.copySourcePage = combatVideo.drawPage = CombatPage::First;
    for (const uint16_t start : {(uint16_t)0xFFFE, (uint16_t)0xFFFF}) {
      combatVideo.graphicsRowOffsets[0] = start;
      const std::string want = start == 0xFFFF ? carried : std::string();
      Check(RefusedFor([&] { BtechEgaCaptureScreenRectangle(buffer, 0, 0, 8, 1); }) == want, "the capture's last word");
      Check(RefusedFor([&] { BtechEgaRestoreScreenRectangle(buffer, 0, 0, 8, 1); }) == want, "the restore's last word");
    }
  }
  std::printf("ega_canvas_writer: %s\n", failures ? "FAILED" : "ok");
  return failures ? 1 : 0;
}
