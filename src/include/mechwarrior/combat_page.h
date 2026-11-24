#ifndef MECHWARRIOR_COMBAT_PAGE_H
#define MECHWARRIOR_COMBAT_PAGE_H
#include <stdint.h>

/* A page combat draws on: one of the canvas's eight (src/Video/Canvas.h). The
 * original named a page by its segment, A000h for the first and 200h more for
 * each one after it; the drawing never needed the number, only which page it
 * is. None is the start state, before the video starts (the original's zero):
 * a drawing on it is refused when a byte is drawn. The front and back page
 * swap at each flip, so these name the pages, not their use. */
enum class CombatPage : uint8_t
{
    None,
    First,          /* A000h */
    Second,         /* A200h */
    BitmapScratch=4 /* A600h */
};
#endif
