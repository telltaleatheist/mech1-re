// The game's door to the story file: imported if needed, then held in memory
// for the importers in Original/Memory, which run again on every EXEC.
#include "mechwarrior/story.h"

#include <cstdio>
#include <cstring>

#include "Records.h"
#include "Story.h"

namespace {
mw1o::import::StoryText g_story;
std::vector<mw1o::import::PilotRecord> g_pilots;
mw1o::import::Planets g_planets;
mw1o::import::Missions g_missions;
std::vector<mw1o::import::EventTextRecord> g_event_text;

// Runs a rebuild; an error goes to stderr and returns false.
template <typename Build>
bool rebuild(Build build)
{
    try {
        build();
        return true;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "MechWarrior: %s\n", e.what());
        return false;
    }
}
}

bool MwHostPrepareStory(const char *gameDirectory, const char *writeDirectory)
{
    try {
        if (mw1o::import::ensure_story(gameDirectory, writeDirectory))
            std::fprintf(stderr, "MechWarrior: imported the story file into %s\n",
                         mw1o::import::story_dir(writeDirectory).string().c_str());
        g_story = mw1o::import::load_story_text(writeDirectory);
        g_pilots = mw1o::import::load_pilots(writeDirectory);
        g_planets = mw1o::import::load_planets(writeDirectory);
        g_missions = mw1o::import::load_missions(writeDirectory);
        g_event_text = mw1o::import::load_event_text(writeDirectory);
        return true;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "MechWarrior: %s\n", e.what());
        return false;
    }
}

bool MwStoryCopyText(const char *id, char *text, size_t capacity)
{
    const auto it = g_story.find(id);
    if (it == g_story.end()) {
        std::fprintf(stderr, "MechWarrior: the story file has no text \"%s\"\n", id);
        return false;
    }
    const std::string &found = it->second;
    if (found.find('\0') != std::string::npos || found.size() + 1 > capacity) {
        std::fprintf(stderr, "MechWarrior: the story text \"%s\" is longer than %zu characters or holds a NUL\n",
                     id, capacity - 1);
        return false;
    }
    std::memcpy(text, found.c_str(), found.size());
    std::memset(text + found.size(), 0, capacity - found.size());  // the whole buffer, as the exe holds it
    return true;
}

bool MwStoryBuildPilots(uint8_t *storage, size_t capacity, uint16_t base, uint16_t *offsets, size_t offsetCount)
{
    return rebuild([&] {
        mw1o::import::build_pilots(g_pilots, g_story, {storage, capacity}, base, {offsets, offsetCount});
    });
}

bool MwStoryBuildPlanets(uint8_t *storage, size_t capacity, uint16_t base, uint16_t *offsets, size_t offsetCount,
    uint16_t *alphabetical, size_t alphabeticalCount)
{
    return rebuild([&] {
        mw1o::import::build_planets(g_planets, g_story, {storage, capacity}, base, {offsets, offsetCount},
                                    {alphabetical, alphabeticalCount});
    });
}

bool MwStoryBuildMissions(uint8_t *storage, size_t capacity, uint16_t base, uint8_t *maximumIndices,
    uint16_t *listOffsets, size_t categoryCount, uint16_t *entries, size_t entryCount, uint16_t entryBase)
{
    return rebuild([&] {
        mw1o::import::build_missions(g_missions, g_story, {storage, capacity}, base, {maximumIndices, categoryCount},
                                     {listOffsets, categoryCount}, {entries, entryCount}, entryBase);
    });
}

bool MwStoryBuildEventText(uint8_t *storage, size_t capacity, uint16_t base, uint16_t *directory, size_t directoryCount)
{
    return rebuild([&] {
        mw1o::import::build_event_text(g_event_text, g_story, {storage, capacity}, base, {directory, directoryCount});
    });
}
