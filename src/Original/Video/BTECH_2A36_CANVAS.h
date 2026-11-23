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
#endif
