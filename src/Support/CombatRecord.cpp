#include "mechwarrior/combat_record.h"
#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/port_diagnostics.h"
#include <string.h>

[[noreturn]] void BtechRefuseDamagedFile(const char *file, const char *what)
{
    static char reason[400];
    snprintf(reason, sizeof(reason), "%s %s. Your copy of the game's files is damaged: run mw1r --setup and pick a "
        "good copy.", file, what);
    MwAbortWithReason("port failsafe", reason, __FILE__, __LINE__, __func__);
}

static size_t ReadOrRefuse(void *record, uint16_t bytes, FILE *stream, const char *file, const char *what, int number,
    bool mayEnd)
{
    const size_t read = BtechReadStreamElements(record, 1, bytes, stream);
    if (read == bytes || (mayEnd && !read)) return read;
    char ends[160];
    snprintf(ends, sizeof(ends), "ends inside %s %d (%u of %u bytes)", what, number, (unsigned)read, (unsigned)bytes);
    BtechRefuseDamagedFile(file, ends);
}

void BtechReadFileRecord(void *record, uint16_t bytes, FILE *stream, const char *file, const char *what, int number)
{
    ReadOrRefuse(record, bytes, stream, file, what, number, false);
}

bool BtechReadNextFileRecord(void *record, uint16_t bytes, FILE *stream, const char *file, const char *what, int number)
{
    return ReadOrRefuse(record, bytes, stream, file, what, number, true) != 0;
}

CombatRecord CombatRecord::part(uint32_t at, uint32_t count, const char *what) const
{
    if (at > byteCount_ || count > byteCount_ - at)
    {
        char outside[160];
        snprintf(outside, sizeof(outside), "has %s outside the file", what);
        BtechRefuseDamagedFile(file_, outside);
    }
    return CombatRecord(bytes_ + at, count, file_);
}

CombatRecord CombatRecord::window(uint32_t at) const
{
    if (at > byteCount_) return CombatRecord(bytes_, 0, file_);
    const uint32_t count = byteCount_ - at;
    return CombatRecord(bytes_ + at, count < 0x10000 ? count : 0x10000, file_);
}

const uint8_t *CombatRecord::list(uint32_t at, uint8_t end, const char *what) const
{
    if (at >= byteCount_ || !memchr(bytes_ + at, end, byteCount_ - at))
    {
        char outside[160];
        snprintf(outside, sizeof(outside), "has %s without an end", what);
        BtechRefuseDamagedFile(file_, outside);
    }
    return bytes_ + at;
}

uint8_t CombatRecord::byte(uint32_t at) const { return part(at, 1, "a field").bytes_[0]; }
uint16_t CombatRecord::word(uint32_t at) const { return MwReadWord(part(at, 2, "a field").bytes_); }
