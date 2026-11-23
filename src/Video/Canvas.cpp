#include "Canvas.h"

#include <stdexcept>
#include <string>

namespace mw1o::video
{
namespace
{
void CheckPage(int index)
{
    if (index < 0 || index >= Canvas::PAGES) throw std::out_of_range("canvas: no page " + std::to_string(index));
}
void CheckEntry(int index)
{
    if (index < 0 || index > Canvas::BORDER) throw std::out_of_range("canvas: no palette entry " + std::to_string(index));
}
}

Rgb PaletteCodeColour(uint8_t code)
{
    const uint8_t intensity = (code & 0x10) ? 85 : 0;
    Rgb colour{(uint8_t)(((code & 4) ? 170 : 0) + intensity), (uint8_t)(((code & 2) ? 170 : 0) + intensity),
               (uint8_t)(((code & 1) ? 170 : 0) + intensity)};
    if ((code & 0x17) == 6) colour.g = 85;
    return colour;
}

void Canvas::clear()
{
    memory_.fill(0);
}

uint8_t *Canvas::page(int index)
{
    CheckPage(index);
    return memory_.data() + index * PAGE_STRIDE;
}

const uint8_t *Canvas::page(int index) const
{
    CheckPage(index);
    return memory_.data() + index * PAGE_STRIDE;
}

void Canvas::setPaletteEntry(int index, Rgb colour)
{
    CheckEntry(index);
    palette_[index] = colour;
}

Rgb Canvas::paletteEntry(int index) const
{
    CheckEntry(index);
    return palette_[index];
}

void Canvas::present(int index)
{
    CheckPage(index);
    presented_ = index;
    if (presenter_) presenter_(*this, index);
}

Canvas &GameCanvas()
{
    static Canvas canvas;
    return canvas;
}
}
