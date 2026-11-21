#ifndef MECHWARRIOR_RULES_H
#define MECHWARRIOR_RULES_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* The rules (src/Rules/Rules.h): read once from <dataDirectory>/rules/ and
 * held in memory. False (with the reason on stderr) if they can't be read. */
bool MwHostPrepareRules(const char *dataDirectory);
/* Copies rule piece `id` (src/Rules/rule_pieces.txt) as the exe lays it out,
 * rebuilt from the rules. False (with the reason on stderr) if there is no
 * such piece or `size` isn't its length. */
bool MwRulesCopyNative(const char *id, uint8_t *bytes, size_t size);
/* Copies `count` words of rule piece `id` from byte `at`, read as the exe's
 * little-endian words. False (with the reason on stderr) if there is no such
 * piece or it ends before them. */
bool MwRulesCopyWords(const char *id, uint16_t *words, size_t count, size_t at);
/* Copies `count` bytes of rule piece `id` from byte `at`. False (with the
 * reason on stderr) if there is no such piece or it ends before them. */
bool MwRulesCopyBytes(const char *id, uint8_t *bytes, size_t count, size_t at);
/* The length of rule piece `id`, or 0 if there is none. */
size_t MwRulesPieceSize(const char *id);
#endif
