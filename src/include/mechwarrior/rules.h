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
/* The length of rule piece `id`, or 0 if there is none. */
size_t MwRulesPieceSize(const char *id);
#endif
