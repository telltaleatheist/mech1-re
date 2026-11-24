#ifndef MECHWARRIOR_COMBAT_RECORD_H
#define MECHWARRIOR_COMBAT_RECORD_H
#include "mechwarrior/read_word.h"
#include <stdint.h>
#include <stdio.h>

/* Combat's file records, read to their end and never past it. A file that ends
 * early is refused by name: the original went on with whatever its memory held
 * (docs/phase5/NO_EMULATION_PLAN.md, B4e, says what that was, file by file). */

/* Stops: `file` is damaged. `what` says how, after the file's name. */
[[noreturn]] void BtechRefuseDamagedFile(const char *file, const char *what);

/* `bytes` of a record into `record`, or stops: the file ends inside `what`
 * `number`. */
void BtechReadFileRecord(void *record, uint16_t bytes, FILE *stream, const char *file, const char *what, int number);
/* The same for a list that ends with its file: false when the file ends
 * before the record's first byte. */
bool BtechReadNextFileRecord(void *record, uint16_t bytes, FILE *stream, const char *file, const char *what, int number);

/* Part of a file in memory, and the file's name for a refusal. */
class CombatRecord
{
public:
    CombatRecord(const uint8_t *bytes, uint32_t byteCount, const char *file)
        : bytes_(bytes), byteCount_(byteCount), file_(file) {}
    /* `count` bytes at `at`, or stops: `what` lies outside the file. */
    CombatRecord part(uint32_t at, uint32_t count, const char *what) const;
    /* From `at` to the end or 10000h bytes on, as far as a near offset
     * reaches; none when `at` is past the end (its parts are then refused). */
    CombatRecord window(uint32_t at) const;
    /* A list that ends with the byte `end`, or stops when none follows. */
    const uint8_t *list(uint32_t at, uint8_t end, const char *what) const;
    /* Fields of a part whose size the caller asked for, or stops. */
    uint8_t byte(uint32_t at) const;
    uint16_t word(uint32_t at) const;
    const uint8_t *bytes() const { return bytes_; }
    uint32_t byteCount() const { return byteCount_; }

private:
    const uint8_t *bytes_;
    uint32_t byteCount_;
    const char *file_;
};
#endif
