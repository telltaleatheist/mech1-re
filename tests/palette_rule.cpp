// The port's palette-code rule (src/Video/Canvas.cpp, PaletteCodeColour) for
// every code, one "code r g b" line each, for tools/trace/test_palette.py to
// check against the trace tools' own rule (G1b).
#include "Canvas.h"

#include <cstdio>

int main() {
  for (int code = 0; code < 256; ++code) {
    const mw1o::video::Rgb c = mw1o::video::PaletteCodeColour((uint8_t)code);
    std::printf("%d %d %d %d\n", code, c.r, c.g, c.b);
  }
  return 0;
}
