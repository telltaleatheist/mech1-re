#ifndef MECHWARRIOR_STORY_SPAN_H
#define MECHWARRIOR_STORY_SPAN_H
#include <initializer_list>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

/* A run of bytes of the player's story data, for an importer: UI words that
 * lie back to back, or one native span. Every read is bounded by the run. A
 * run that is missing, has a gap or another length, and a read past its end,
 * stop the program with the run's name: each is a fault of this program, as
 * the player's folders are checked at start. Offsets are the run's own. */
class MwStorySpan
{
public:
    /* The words of `ids`, in address order; an id that ends in '.' stands for
     * every word whose id starts with it. */
    static MwStorySpan words(std::initializer_list<const char *> ids);
    /* One native span, `size` bytes. */
    static MwStorySpan native(const char *id,size_t size);
    /* Bytes the caller has, at `base` (the tests'). */
    MwStorySpan(std::string name,uint16_t base,std::vector<uint8_t> bytes):name_(std::move(name)),base_(base),bytes_(std::move(bytes)) {}

    size_t size() const {return bytes_.size();}
    uint8_t byte(size_t at) const {return *bytes(at,1);}
    uint16_t word(size_t at) const {const uint8_t *from=bytes(at,2);return (uint16_t)(from[0]|from[1]<<8);}
    /* `count` bytes from `at`, all inside the run. */
    const uint8_t *bytes(size_t at,size_t count) const;
    void copy(void *to,size_t count,size_t at) const;
    /* The whole run, `count` bytes long. */
    void copyAll(void *to,size_t count) const;
    /* A near offset of the words' segment (the pointer tables hold them). */
    bool holds(uint16_t nearOffset) const {return nearOffset>=base_ && (size_t)(nearOffset-base_)<bytes_.size();}
    /* And a text that ends in the run starts there. */
    bool holdsText(uint16_t nearOffset) const;
    size_t index(uint16_t nearOffset) const;
    /* The text at `at`, its NUL inside the run. */
    const char *text(size_t at) const;
private:
    [[noreturn]] void fault(const char *what) const;
    std::string name_;
    uint16_t base_;
    std::vector<uint8_t> bytes_;
};
/* The text of the word that starts at `nearOffset`, among those whose id
 * starts with `prefix`, into `text` with the rest zeroed. False if there is
 * none or it doesn't fit (a pointer table's word of another kind). */
bool MwWordsCopyTextAt(const char *prefix,uint16_t nearOffset,char *text,size_t capacity);
#endif
