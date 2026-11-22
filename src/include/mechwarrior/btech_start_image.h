#ifndef MECHWARRIOR_BTECH_START_IMAGE_H
#define MECHWARRIOR_BTECH_START_IMAGE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* BTECH's start image: what its importers read at a combat EXEC, laid out as
 * the exe's unpacked image (BTECH_IMAGE_BYTES, loaded at 0810), built without
 * reading the exe (docs/phase2/PLAN.md 8.5, slices 6c and 6d). Every byte the
 * importers read is a row here; every other byte is zero. */
typedef enum BtechStartSource
{
    BTECH_START_CODE,      /* `bytes`: a constant, as the exe holds it */
    BTECH_START_ZERO,      /* zero in the exe */
    BTECH_START_GENERATED, /* `generate`: a table computed byte-exact */
    BTECH_START_STORY,     /* the story file's native span of this id (story/native.json) */
    BTECH_START_RULES,     /* the rule piece of this id (data/rules, src/Rules/rule_pieces.txt) */
    BTECH_START_UI         /* the layout piece of this id (data/ui, src/Ui/ui_pieces.txt) */
} BtechStartSource;
typedef struct BtechStartRow
{
    const char *id;
    uint16_t segment,offset,size;
    BtechStartSource source;
    const uint8_t *bytes;
    /* Writes the table at `at` and returns its smallest distance from an
     * integer before rounding down, over the entries it computes (the ones
     * that are not exact by construction), so a test can see the margin. */
    double (*generate)(uint8_t *at);
} BtechStartRow;
extern const BtechStartRow btechStartRows[];
extern const size_t btechStartRowCount;
/* The image offset of segment:offset. */
size_t BtechStartImageOffset(uint16_t segment,uint16_t offset);
/* Fills `image` (BTECH_IMAGE_BYTES): zero, then the rows. False if a story
 * span or a rule or layout piece is missing or of another length. */
bool BtechBuildStartImage(uint8_t *image);
#endif
