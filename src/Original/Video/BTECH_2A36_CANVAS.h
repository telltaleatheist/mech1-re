#ifndef MECHWARRIOR_BTECH_2A36_CANVAS_H
#define MECHWARRIOR_BTECH_2A36_CANVAS_H
#include "Canvas.h"
#include "mechwarrior/combat_video.h"

/* Combat's drawing on the canvas (docs/phase5/NO_EMULATION_PLAN.md, G2-G4).
 * 2A36 keeps its own addressing (a page selector, a byte offset of eight pixels
 * from the page's start, a mask of those eight with the leftmost at bit 7) and
 * draws through these, by pixel. Nothing here models a card.
 *
 * The game rules this rests on (G4, from both BTECH revisions and every trace):
 * combat draws only by copy, with no rotation and no set/reset, and every
 * routine that narrows the colours it writes widens them again before it
 * returns. So a byte drawn with a mask is the colour where the mask has a 1 and
 * the pixels as they were elsewhere; the order of the original's reads and
 * writes inside a routine no longer shows. */

/* A page selector's canvas page (A000h page 0, A200h page 1, ...). */
int BtechEgaCanvasPage(uint16_t selector);
/* A byte offset's first pixel on its page (it may run past row 199). */
void BtechEgaCanvasPosition(uint16_t byteOffset, int *x, int *y);

/* `colour` (its low four bits) where `mask` has a 1; the other pixels stay. */
void BtechEgaCanvasDraw(int page, uint16_t byteOffset, uint8_t mask, uint8_t colour);
/* The eight pixels' colours, leftmost first. */
void BtechEgaCanvasSetPixels(int page, uint16_t byteOffset, const uint8_t colours[8]);
/* The same eight pixels copied from one page to another. */
void BtechEgaCanvasCopy(int fromPage, int toPage, uint16_t byteOffset);

/* The game's planar images (bitmaps, saved rectangles) keep each colour bit of
 * eight pixels in its own byte, leftmost at bit 7. These read and write one bit
 * of the colours of eight canvas pixels in that form. */
uint8_t BtechEgaCanvasColourBits(int page, uint16_t byteOffset, int bit);
void BtechEgaCanvasSetColourBits(int page, uint16_t byteOffset, int bit, uint8_t bits);
/* Sets colour bit `bit` where `bits` has a 1 (the others stay). */
void BtechEgaCanvasAddColourBits(int page, uint16_t byteOffset, int bit, uint8_t bits);

/* A drawing routine's pen: the original sets a mask, then writes the colour to
 * one byte or a run of bytes with it. The page is resolved at the first byte,
 * so a routine that returns early checks nothing. */
class BtechEgaCanvasWriter
{
public:
    explicit BtechEgaCanvasWriter(uint16_t selector) : selector_(selector) {}
    void setMask(uint8_t mask) { mask_ = mask; maskSet_ = true; }
    void write(uint16_t byteOffset, uint8_t colour);

private:
    uint16_t selector_;
    bool resolved_ = false, maskSet_ = false;
    int page_ = 0;
    uint8_t mask_ = 0;
};
#endif
