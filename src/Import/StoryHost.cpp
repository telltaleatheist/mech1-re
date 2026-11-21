// The game's door to the story file: imported if needed, then held in memory
// for the importers in Original/Memory, which run again on every EXEC.
#include "mechwarrior/story.h"

#include <cstdio>
#include <cstring>

#include "Story.h"

namespace {
mw1o::import::StoryText g_story;
}

bool MwHostPrepareStory(const char *gameDirectory, const char *writeDirectory)
{
    try {
        if (mw1o::import::ensure_story(gameDirectory, writeDirectory))
            std::fprintf(stderr, "MechWarrior: imported the story file into %s\n",
                         mw1o::import::story_dir(writeDirectory).string().c_str());
        g_story = mw1o::import::load_story_text(writeDirectory);
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
    std::memcpy(text, found.c_str(), found.size() + 1);
    return true;
}
