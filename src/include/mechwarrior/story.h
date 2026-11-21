#ifndef MECHWARRIOR_STORY_H
#define MECHWARRIOR_STORY_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* The story file (src/Import/Story.h): the original's creative content,
 * imported once from the player's exes into <writeDirectory>/story/, then
 * read from there. False (with the reason on stderr) if it can't be imported
 * or read. */
bool MwHostPrepareStory(const char *gameDirectory, const char *writeDirectory);
/* Copies the story text `id` into `text` and zeroes the rest of the buffer.
 * False (with the reason on stderr) if there is no such text or it doesn't fit. */
bool MwStoryCopyText(const char *id, char *text, size_t capacity);
/* The original's record storage, rebuilt from story/pilots.json,
 * planets.json and missions.json (src/Import/Records.h) as the exe lays it
 * out: `base` is the storage's data-segment offset, and the offsets written
 * are data-segment offsets. The rest of each storage is zeroed. False (with
 * the reason on stderr) if the story doesn't fit the arrays given. */
bool MwStoryBuildPilots(uint8_t *storage, size_t capacity, uint16_t base, uint16_t *offsets, size_t offsetCount);
bool MwStoryBuildPlanets(uint8_t *storage, size_t capacity, uint16_t base, uint16_t *offsets, size_t offsetCount,
    uint16_t *alphabetical, size_t alphabeticalCount);
bool MwStoryBuildMissions(uint8_t *storage, size_t capacity, uint16_t base, uint8_t *maximumIndices,
    uint16_t *listOffsets, size_t categoryCount, uint16_t *entries, size_t entryCount, uint16_t entryBase);
/* The event text bank from story/event_text.json; directory[i] is 0 where no
 * record has entry i. */
bool MwStoryBuildEventText(uint8_t *storage, size_t capacity, uint16_t base, uint16_t *directory, size_t directoryCount);
/* The event script from story/events.json (src/Import/Events.h) into the
 * game's typed owners: eventChoiceCaptions, campaignEventRecords and
 * campaignEventCommands (filled in the original importer's order),
 * eventListEntries, eventLists and fixedCampaignEvent. captionOffsets[i] gets
 * caption i's original address, which the choice menus are matched by. */
bool MwStoryBuildCampaignEvents(uint16_t *captionOffsets, size_t captionCount);
/* The story-planet pools, each copied whole into pools[i] (sizes[i] bytes). */
bool MwStoryCopyPlanetPools(uint8_t *const *pools, const size_t *sizes, size_t poolCount);
#endif
