#ifndef MECHWARRIOR_STORY_H
#define MECHWARRIOR_STORY_H
#include <stdbool.h>
#include <stddef.h>

/* The story file (src/Import/Story.h): the original's creative content,
 * imported once from the player's exes into <writeDirectory>/story/, then
 * read from there. False (with the reason on stderr) if it can't be imported
 * or read. */
bool MwHostPrepareStory(const char *gameDirectory, const char *writeDirectory);
/* Copies the story text `id`, NUL-terminated, into `text`. False (with the
 * reason on stderr) if there is no such text or it doesn't fit. */
bool MwStoryCopyText(const char *id, char *text, size_t capacity);
#endif
