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
uint8_t Combine(uint8_t old, uint8_t colour, Logic logic)
{
    switch (logic)
    {
    case Logic::And: return old & colour;
    case Logic::Or: return old | colour;
    case Logic::Xor: return old ^ colour;
    default: return colour;
    }
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
    presented_ = 0;
}

void Canvas::setUp()
{
    clear();
    for (int index = 0; index < COLOURS; ++index)
        palette_[index] = PaletteCodeColour((uint8_t)((index & 7) | ((index & 8) ? 0x10 : 0)));
    palette_[BORDER] = PaletteCodeColour(0);
    shown_ = true;
    ++setUps_;
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

uint8_t *Canvas::at(int page, int x, int y, int count)
{
    uint8_t *start = this->page(page);
    const long first = (long)y * WIDTH + x;
    if (x < 0 || y < 0 || (std::size_t)(start - memory_.data()) + first + count > MEMORY_BYTES)
        throw std::out_of_range("canvas: (" + std::to_string(x) + ", " + std::to_string(y) + ") on page "
                                + std::to_string(page) + " is past the canvas");
    return start + first;
}

void Canvas::pixel(int page, int x, int y, uint8_t colour, Logic logic)
{
    uint8_t *p = at(page, x, y, 1);
    *p = Combine(*p, colour & 15, logic);
}

uint8_t Canvas::pixelAt(int page, int x, int y) const
{
    return const_cast<Canvas *>(this)->at(page, x, y, 1)[0];
}

void Canvas::bitPattern(int page, int x, int y, uint8_t bits, uint8_t colour, Logic logic)
{
    uint8_t *p = at(page, x, y, 8);
    for (int i = 0; i < 8; ++i)
        if (bits & (0x80 >> i)) p[i] = Combine(p[i], colour & 15, logic);
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
