#ifndef MECHWARRIOR_BTECH_2A36_CANVAS_H
#define MECHWARRIOR_BTECH_2A36_CANVAS_H
#include "Canvas.h"
#include "mechwarrior/combat_video.h"

/* Slices G2-G4 (docs/phase5/NO_EMULATION_PLAN.md): the 2A36 paths drawn
 * through the canvas, beside those still on the EGA register model, which goes
 * in G4. A converted path keeps its latch reads, so the latches stay as the
 * original leaves them for the others, and leaves the registers as it would. */

/* A page selector's canvas page (A000h page 0, A200h page 1, ...). */
int BtechEgaCanvasPage(uint16_t selector);
/* An aperture byte offset's first pixel on its page (it may run past row 199). */
void BtechEgaCanvasPosition(uint16_t byteOffset, int *x, int *y);
/* What a mode 2 write inherits: checked to be write mode 2 with every plane
 * enabled (the only state the converted paths are written for; anything else
 * stops the game with a note, never draws differently), and the function. */
mw1o::video::Logic BtechEgaCanvasInheritedLogic(void);

/* G3: 2A36's mode 2 byte writes, in pixels. The original reads a byte (loading
 * the latches), sets the bit mask and writes a colour; a byte written with no
 * read of its own (REP STOSB) reuses the latches of the last one read. So for
 * each of the byte's eight pixels: where the bit mask has a 1, the colour by the
 * inherited function onto the latched pixel; elsewhere the latched pixel. Read
 * just before, that is the byte's own pixels; reused, it is the earlier byte's,
 * as on the card. The register writes and latch reads still reach the register
 * model for the paths on it (until G4). The page, the function, and the bit mask
 * and latches inherited from before the path are resolved at the first byte. */
class BtechEgaCanvasWriter
{
public:
    explicit BtechEgaCanvasWriter(uint16_t selector) : selector_(selector) {}
    /* The bit mask register, written as the original writes it. */
    void setMask(uint8_t mask);
    /* The latch read at `cursor`. */
    void load(uint16_t cursor);
    /* The colour written at `cursor`. */
    void write(uint16_t cursor, uint8_t colour);

private:
    void resolve();
    uint16_t selector_;
    bool resolved_ = false, maskSet_ = false;
    int page_ = 0;
    mw1o::video::Logic logic_ = mw1o::video::Logic::Copy;
    uint8_t mask_ = 0xFF, latched_[8] = {};
};
#endif
