#ifndef MECHWARRIOR_BTECH_START_IMAGE_H
#define MECHWARRIOR_BTECH_START_IMAGE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <vector>

/* BTECH's start rows: what its importers read at a combat EXEC, without
 * reading the exe (docs/phase2/PLAN.md 8.5, slices 6c and 6d). A row keeps the
 * address its bytes have in the exe's unpacked image (loaded at 0810), so
 * start_exe_test can check it there; the importers read it by id. */
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
/* One row's bytes, for an importer: from the code, a generated table, the
 * story, or a rule or layout piece, as the row says. A piece that is missing
 * or of another length, and a read past the row's end, stop the program with
 * the row's id. Each is a fault of this program: the player's folders are
 * checked at start, before any of this runs. Offsets are the row's own. */
class BtechStartRowData
{
public:
    explicit BtechStartRowData(const char *id);
    explicit BtechStartRowData(const BtechStartRow &row);
    uint8_t byte(size_t at=0) const {return *bytes(at,1);}
    uint16_t word(size_t at=0) const {const uint8_t *from=bytes(at,2);return (uint16_t)(from[0]|from[1]<<8);}
    uint32_t dword(size_t at=0) const {return word(at)|(uint32_t)word(at+2)<<16;}
    /* `count` bytes from `at`, all inside the row. */
    const uint8_t *bytes(size_t at,size_t count) const;
    void copy(void *to,size_t count,size_t at=0) const;
    /* A near offset of the row's segment (the exe's directories hold them). */
    bool holds(uint16_t nearOffset) const;
    size_t index(uint16_t nearOffset) const;
    /* The text at `at`, its NUL inside the row. */
    const char *text(size_t at) const;
private:
    [[noreturn]] void fault(const char *what) const;
    const BtechStartRow &row_;
    std::vector<uint8_t> bytes_;
};
/* The image offset of segment:offset. */
size_t BtechStartImageOffset(uint16_t segment,uint16_t offset);
#endif
