#ifndef MECHWARRIOR_VIDEO_MODE_H
#define MECHWARRIOR_VIDEO_MODE_H
#include <stdint.h>

/* The games' video mode calls (INT 10h AH=00h and 0Fh in the originals) as the
 * canvas's set-up (docs/phase5/NO_EMULATION_PLAN.md, G5). Mode 0Dh, the
 * 320x200 mode both games draw in, sets up the canvas; mode 3, the text mode
 * they leave in, closes it. No other mode is used. */
enum { MW_VIDEO_MODE_TEXT=0x03, MW_VIDEO_MODE_320X200=0x0D };
void MwSetVideoMode(uint8_t mode);
/* The mode last set: the campaign saves it at start and sets it again on exit. */
uint8_t MwGetVideoMode(void);
#endif
