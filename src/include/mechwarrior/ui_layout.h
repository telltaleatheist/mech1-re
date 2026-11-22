#ifndef MECHWARRIOR_UI_LAYOUT_H
#define MECHWARRIOR_UI_LAYOUT_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* The layout (src/Ui/Ui.h): read once from <dataDirectory>/ui/ and held in
 * memory. False (with the reason on stderr) if it can't be read. */
bool MwHostPrepareUi(const char *dataDirectory);
/* Copies layout piece `id` (src/Ui/ui_pieces.txt) as the exe lays it out,
 * rebuilt from data/ui. False (with the reason on stderr) if there is no such
 * piece or `size` isn't its length. */
bool MwUiCopyNative(const char *id, uint8_t *bytes, size_t size);
/* Copies `count` little-endian words, or bytes, of layout piece `id` from
 * byte `at`. False (with the reason on stderr) if there is no such piece or
 * it ends before them. */
bool MwUiCopyWords(const char *id, uint16_t *words, size_t count, size_t at);
bool MwUiCopyBytes(const char *id, uint8_t *bytes, size_t count, size_t at);
/* The length of layout piece `id`, or 0 if there is none. */
size_t MwUiPieceSize(const char *id);
/* Where caption piece `id` (its head) sits in MW_MAIN's data segment, or 0 if
 * there is no such caption: what the exe's panels, menus and pointer tables
 * hold, which the importers match to rebuild them from names. The caption's
 * text is a word (story/words.json), not read here. */
uint16_t MwUiCaptionOffset(const char *id);
/* Where layout piece `id` sits in its exe's data segment, or 0 if there is
 * none: what the exe's pointer tables hold, which the layout rebuilds from
 * names (a panel, a diagram). */
uint16_t MwUiPieceOffset(const char *id);
#endif
