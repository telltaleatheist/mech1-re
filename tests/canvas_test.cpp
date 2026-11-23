// src/Video/Canvas (docs/phase5/NO_EMULATION_PLAN.md, G1, G2): the pages where
// the EGA model's selectors put them, the palette codes' colours, the palette's
// entries, present(), and drawing. Plain checks, no framework: exit 0 when all hold.
#include "Canvas.h"

#include <cstdio>
#include <stdexcept>

using mw1o::video::Canvas;
using mw1o::video::PaletteCodeColour;
using mw1o::video::Rgb;

namespace {

int failures = 0;

void Check(bool ok, const char *what) {
  if (!ok) {
    std::printf("FAIL %s\n", what);
    ++failures;
  }
}

template <typename F>
bool Throws(F f) {
  try {
    f();
  } catch (const std::out_of_range &) {
    return true;
  }
  return false;
}

}  // namespace

int main() {
  static Canvas canvas;  // 512 KB: not on the stack

  // Pages: PAGE_STRIDE apart in memory(), where the EGA model's selectors
  // (A000h, A200h, ...; 2000h bytes of 8 pixels each) put them.
  Check(canvas.page(0) == canvas.memory(), "page 0 starts the memory");
  Check(canvas.page(1) == canvas.memory() + 0x2000 * 8, "page 1 is selector A200h");
  Check(canvas.page(7) + Canvas::PAGE_STRIDE == canvas.memory() + Canvas::MEMORY_BYTES, "page 7 is the last");
  Check(Canvas::PAGE_STRIDE >= (std::size_t)Canvas::WIDTH * Canvas::HEIGHT, "a page holds a frame");
  Check(Throws([&] { canvas.page(8); }) && Throws([&] { canvas.page(-1); }), "no page 8 or -1");
  canvas.page(3)[Canvas::WIDTH * 199 + 319] = 15;
  canvas.page(3)[Canvas::WIDTH * Canvas::HEIGHT] = 9;  // the page's off-screen tail
  Check(canvas.memory()[3 * Canvas::PAGE_STRIDE + 63999] == 15, "the last pixel of page 3");
  canvas.clear();
  bool zero = true;
  for (std::size_t i = 0; i < Canvas::MEMORY_BYTES; ++i) zero = zero && canvas.memory()[i] == 0;
  Check(zero, "clear() zeroes every page and tail");

  // The palette codes' colours in the game's 320x200 mode.
  Check(PaletteCodeColour(0x00) == Rgb{0, 0, 0}, "code 0 is black");
  Check(PaletteCodeColour(0x01) == Rgb{0, 0, 170}, "code 1 is blue");
  Check(PaletteCodeColour(0x04) == Rgb{170, 0, 0}, "code 4 is red");
  Check(PaletteCodeColour(0x06) == Rgb{170, 85, 0}, "code 6 is brown");
  Check(PaletteCodeColour(0x07) == Rgb{170, 170, 170}, "code 7 is light grey");
  Check(PaletteCodeColour(0x10) == Rgb{85, 85, 85}, "code 10h is dark grey");
  Check(PaletteCodeColour(0x16) == Rgb{255, 255, 85}, "code 16h is yellow");
  Check(PaletteCodeColour(0x17) == Rgb{255, 255, 255}, "code 17h is white");
  // Bits 3, 5, 6 and 7 aren't shown in this mode.
  bool ignored = true;
  for (int code = 0; code < 256; ++code) ignored = ignored && PaletteCodeColour((uint8_t)code) == PaletteCodeColour((uint8_t)(code & 0x17));
  Check(ignored, "only bits 0, 1, 2 and 4 show");
  Check(PaletteCodeColour(0x0E) == Rgb{170, 85, 0}, "code 0Eh is brown too");

  // Palette entries: 16 colours and the border.
  canvas.setPaletteEntry(5, Rgb{1, 2, 3});
  canvas.setPaletteEntry(Canvas::BORDER, Rgb{4, 5, 6});
  Check(canvas.paletteEntry(5) == Rgb{1, 2, 3} && canvas.palette()[5] == Rgb{1, 2, 3}, "entry 5 set");
  Check(canvas.paletteEntry(Canvas::BORDER) == Rgb{4, 5, 6}, "the border set");
  Check(Throws([&] { canvas.setPaletteEntry(17, Rgb{}); }) && Throws([&] { canvas.paletteEntry(-1); }),
        "no entry 17 or -1");

  // present(): the presenter sees the canvas and the page.
  int shown = -1;
  const Canvas *seen = nullptr;
  canvas.present(2);
  Check(canvas.presentedPage() == 2, "present without a presenter records the page");
  canvas.setPresenter([&](const Canvas &c, int page) {
    seen = &c;
    shown = page;
  });
  canvas.present(1);
  Check(seen == &canvas && shown == 1 && canvas.presentedPage() == 1, "the presenter shows page 1");
  Check(Throws([&] { canvas.present(8); }) && shown == 1, "no page 8 presented");

  // Drawing (G2): pixels and eight-pixel bit patterns, by each function.
  using mw1o::video::Logic;
  canvas.clear();
  canvas.pixel(1, 10, 20, 0x1C);
  Check(canvas.pixelAt(1, 10, 20) == 0x0C && canvas.page(1)[20 * 320 + 10] == 0x0C, "a pixel, its colour's low nibble");
  canvas.pixel(1, 10, 20, 0x06, Logic::And);
  Check(canvas.pixelAt(1, 10, 20) == 0x04, "AND");
  canvas.pixel(1, 10, 20, 0x03, Logic::Or);
  Check(canvas.pixelAt(1, 10, 20) == 0x07, "OR");
  canvas.pixel(1, 10, 20, 0x05, Logic::Xor);
  Check(canvas.pixelAt(1, 10, 20) == 0x02, "XOR");
  canvas.bitPattern(0, 16, 3, 0xA1, 9);
  const uint8_t want[8] = {9, 0, 9, 0, 0, 0, 0, 9};
  bool pattern = true;
  for (int i = 0; i < 8; ++i) pattern = pattern && canvas.pixelAt(0, 16 + i, 3) == want[i];
  Check(pattern && canvas.pixelAt(0, 15, 3) == 0 && canvas.pixelAt(0, 24, 3) == 0, "a bit pattern, leftmost at bit 7");
  canvas.bitPattern(0, 16, 3, 0xFF, 3, Logic::Xor);
  Check(canvas.pixelAt(0, 16, 3) == (9 ^ 3) && canvas.pixelAt(0, 17, 3) == 3, "a bit pattern by XOR");
  // Past row 199 into the page's tail and the page after, as the EGA model's
  // linear aperture lets the game draw; past the canvas's memory is refused.
  canvas.pixel(0, 0, 205, 7);
  Check(canvas.page(1)[(205 * 320) - Canvas::PAGE_STRIDE] == 7, "row 205 of page 0 is in page 1");
  Check(Throws([&] { canvas.pixel(7, 0, 205, 1); }) && Throws([&] { canvas.bitPattern(7, 319, 204, 1, 1); }),
        "past the last page refused");
  Check(Throws([&] { canvas.pixel(0, -1, 0, 1); }), "negative x refused");

  Check(&mw1o::video::GameCanvas() == &mw1o::video::GameCanvas(), "one game canvas");

  std::printf("canvas: %s\n", failures ? "FAILED" : "ok");
  return failures ? 1 : 0;
}
