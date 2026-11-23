#ifndef MW1O_VIDEO_CANVAS_H
#define MW1O_VIDEO_CANVAS_H
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace mw1o::video
{
/* One colour of the frame. */
struct Rgb
{
    uint8_t r = 0, g = 0, b = 0;
    friend bool operator==(const Rgb &, const Rgb &) = default;
};

/* What the game's palette codes show in its 320x200 mode (BIOS mode 0Dh): bits
 * 2, 1 and 0 are red, green and blue at two thirds, bit 4 adds a third to all
 * three, and code 6 is brown (green at a third). The other bits are not shown
 * in this mode. The game's palette tables are written in these codes. */
Rgb PaletteCodeColour(uint8_t code);

/* How a drawn colour combines with the pixel already there. */
enum class Logic : uint8_t
{
    Copy,
    And,
    Or,
    Xor
};

/* The game's screen (docs/phase5/NO_EMULATION_PLAN.md, "Graphics: a canvas,
 * not an EGA card"): 320x200 pixels, one byte each (a 16-colour index), on as
 * many pages as the game uses, with a 16-entry RGB palette and a border colour.
 * Nothing in it models a card.
 *
 * A page starts every PAGE_STRIDE pixels, where the game's page selectors put
 * it, and the rest of each stride past row 199 is the page's off-screen tail,
 * which the game's documented unchecked drawing and reads (a row past 200 and
 * so on) still reach. Combat and the campaign overlay both draw through the
 * drawing operations (G2-G5).
 *
 * The game's mode sets are the canvas's set-up: setUp() for its 320x200 mode,
 * close() for the text mode it leaves in. A closed canvas is not the screen. */
class Canvas
{
public:
    static constexpr int WIDTH = 320, HEIGHT = 200, PAGES = 8, COLOURS = 16;
    static constexpr std::size_t PAGE_STRIDE = 0x10000, MEMORY_BYTES = PAGE_STRIDE * PAGES;
    /* The border's entry, after the 16 colours. */
    static constexpr int BORDER = COLOURS;
    using Palette = std::array<Rgb, COLOURS + 1>;
    /* Shows a page: the canvas and the page number. */
    using Presenter = std::function<void(const Canvas &, int page)>;

    /* Every page's pixels to index 0, the whole memory with them, and page 0
     * the presented page (a mode set: the screen shows the start of memory). */
    void clear();
    /* The game's 320x200 mode set: clear(), the mode's 16 default colours
     * (codes 0-7 and 10h-17h) and a black border, and the canvas is the
     * screen. Counted, so a presenter can tell a new screen from the last. */
    void setUp();
    /* The game's text mode set: the canvas is no longer the screen. */
    void close() { shown_ = false; }
    bool shown() const { return shown_; }
    unsigned long setUps() const { return setUps_; }
    uint8_t *page(int index);
    const uint8_t *page(int index) const;
    /* All pages, PAGE_STRIDE apart. */
    const uint8_t *memory() const { return memory_.data(); }

    /* Drawing, in pixel coordinates on a page. A position may run on past the
     * page (y*WIDTH+x beyond row 199, into its off-screen tail and the pages
     * after it), as the game's unchecked drawing can in its linear page
     * aperture; past the canvas's memory is refused. Colours are 4-bit indices. */
    void pixel(int page, int x, int y, uint8_t colour, Logic logic = Logic::Copy);
    uint8_t pixelAt(int page, int x, int y) const;
    /* Eight pixels from (x, y) rightwards, leftmost at bit 7: where `bits` has a
     * 1, `colour` is drawn by `logic`; the others are left as they are. */
    void bitPattern(int page, int x, int y, uint8_t bits, uint8_t colour, Logic logic = Logic::Copy);

    void setPaletteEntry(int index, Rgb colour);
    Rgb paletteEntry(int index) const;
    const Palette &palette() const { return palette_; }

    void setPresenter(Presenter presenter) { presenter_ = std::move(presenter); }
    /* Shows `index` through the presenter, if one is set. */
    void present(int index);
    /* The last page present() showed; 0 before any. */
    int presentedPage() const { return presented_; }

private:
    uint8_t *at(int page, int x, int y, int count);
    std::array<uint8_t, MEMORY_BYTES> memory_{};
    Palette palette_{};
    Presenter presenter_;
    int presented_ = 0;
    bool shown_ = false;
    unsigned long setUps_ = 0;
};

/* The game's one canvas, shared by the game's drawing and the host's presenter. */
Canvas &GameCanvas();
}
#endif
