// The game's door to the story file: imported if needed, then held in memory
// for the importers in Original/Memory, which run again on every EXEC.
#include "mechwarrior/story.h"

#include <cstdio>
#include <cstring>
#include <string_view>

#include "Events.h"
#include "Music.h"
#include "Records.h"
#include "mechwarrior/campaign_events.h"
#include "Story.h"

namespace {
mw1o::import::StoryText g_story;
mw1o::import::Words g_words;
mw1o::import::Native g_native;
std::vector<mw1o::import::PilotRecord> g_pilots;
mw1o::import::Planets g_planets;
mw1o::import::Missions g_missions;
std::vector<mw1o::import::EventTextRecord> g_event_text;
mw1o::import::Events g_events;
std::vector<mw1o::import::AdlibInstrument> g_music;
std::string g_btech_revision;

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
        g_words = mw1o::import::load_words(writeDirectory);
        g_native = mw1o::import::load_native(writeDirectory);
        g_pilots = mw1o::import::load_pilots(writeDirectory);
        g_planets = mw1o::import::load_planets(writeDirectory);
        g_missions = mw1o::import::load_missions(writeDirectory);
        g_event_text = mw1o::import::load_event_text(writeDirectory);
        g_events = mw1o::import::load_events(writeDirectory);
        g_music = mw1o::import::load_music(writeDirectory);
        g_btech_revision = mw1o::import::load_btech_revision(writeDirectory);
        return true;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "MechWarrior: %s\n", e.what());
        return false;
    }
}

const char *MwStoryBtechRevision(void)
{
    return g_btech_revision.c_str();
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

// The word piece `id` (ImportTable.h), or null with the reason on stderr.
static const mw1o::import::ImportPiece *find_word(const char *id)
{
    for (const auto &piece : mw1o::import::import_table())
        if (mw1o::import::is_word(piece.kind) && piece.id == id) return &piece;
    std::fprintf(stderr, "MechWarrior: the game has no word \"%s\"\n", id);
    return nullptr;
}

bool MwStoryCopyBytes(const char *id, uint8_t *bytes, size_t size)
{
    const auto it = g_native.find(id);
    if (it == g_native.end()) {
        std::fprintf(stderr, "MechWarrior: the story file has no native span \"%s\"\n", id);
        return false;
    }
    if (it->second.size() != size) {
        std::fprintf(stderr, "MechWarrior: the native span \"%s\" is %zu bytes, the game's %zu\n", id, it->second.size(), size);
        return false;
    }
    std::memcpy(bytes, it->second.data(), size);
    return true;
}

bool MwWordsKnown(const char *id)
{
    for (const auto &piece : mw1o::import::import_table())
        if (mw1o::import::is_word(piece.kind) && piece.id == id) return true;
    return false;
}

bool MwWordsCopyText(const char *id, char *text, size_t capacity)
{
    const auto *piece = find_word(id);
    const auto it = g_words.find(id);
    if (!piece || it == g_words.end() || !mw1o::import::is_string(piece->kind)) {
        if (piece) std::fprintf(stderr, "MechWarrior: the word \"%s\" is not a string\n", id);
        return false;
    }
    const std::string &word = it->second;  // with its NUL
    if (word.size() > capacity) {
        std::fprintf(stderr, "MechWarrior: the word \"%s\" is longer than %zu characters\n", id, capacity - 1);
        return false;
    }
    std::memcpy(text, word.data(), word.size());
    std::memset(text + word.size(), 0, capacity - word.size());  // the whole buffer, as the exe holds it
    return true;
}

bool MwWordsCopyBlock(const char *id, uint8_t *bytes, size_t size)
{
    const auto *piece = find_word(id);
    const auto it = g_words.find(id);
    if (!piece || it == g_words.end()) return false;
    if (it->second.size() != size) {
        std::fprintf(stderr, "MechWarrior: the word \"%s\" is %zu bytes, the game's %zu\n", id, it->second.size(), size);
        return false;
    }
    std::memcpy(bytes, it->second.data(), size);
    return true;
}

bool MwWordsCopyInto(const char *prefix, uint8_t *view, size_t viewBytes)
{
    const std::string_view wanted(prefix);
    size_t copied = 0;
    for (const auto &piece : mw1o::import::import_table()) {
        if (!mw1o::import::is_word(piece.kind) || piece.id.substr(0, wanted.size()) != wanted) continue;
        const auto it = g_words.find(piece.id);
        if (it == g_words.end() || piece.offset > viewBytes || it->second.size() > viewBytes - piece.offset) {
            std::fprintf(stderr, "MechWarrior: the word \"%.*s\" doesn't fit its place\n", int(piece.id.size()),
                         piece.id.data());
            return false;
        }
        std::memcpy(view + piece.offset, it->second.data(), it->second.size());
        ++copied;
    }
    if (!copied) std::fprintf(stderr, "MechWarrior: the game has no words \"%s*\"\n", prefix);
    return copied != 0;
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

namespace {

void need(bool ok, const std::string &why)
{
    if (!ok) throw mw1o::import::ImportError("story/events.json: " + why);
}

MwEventDate native_date(const mw1o::import::EventDate &d)
{
    return MwEventDate{d.month, d.submonth, d.year};
}

}  // namespace

bool MwStoryBuildCampaignEvents(uint16_t *captionOffsets, size_t captionCount)
{
    return rebuild([&] {
        using mw1o::import::EventCommand;
        const auto &e = g_events;
        need(e.captions.size() == CAMPAIGN_EVENT_CAPTION_COUNT && captionCount == CAMPAIGN_EVENT_CAPTION_COUNT,
             "has " + std::to_string(e.captions.size()) + " captions, the game " + std::to_string(CAMPAIGN_EVENT_CAPTION_COUNT));
        need(e.events.size() == CAMPAIGN_EVENT_RECORD_COUNT,
             "has " + std::to_string(e.events.size()) + " events, the game " + std::to_string(CAMPAIGN_EVENT_RECORD_COUNT));
        need(e.list_entries.size() == CAMPAIGN_EVENT_LIST_ENTRIES && e.lists.size() == CAMPAIGN_EVENT_LIST_COUNT,
             "has " + std::to_string(e.list_entries.size()) + " list entries in " + std::to_string(e.lists.size()) +
                 " lists, the game " + std::to_string(CAMPAIGN_EVENT_LIST_ENTRIES) + " in " + std::to_string(CAMPAIGN_EVENT_LIST_COUNT));
        size_t commands = 0;
        for (const auto &event : e.events) commands += event.script.size();
        need(commands == CAMPAIGN_EVENT_COMMAND_COUNT,
             "has " + std::to_string(commands) + " commands, the game " + std::to_string(CAMPAIGN_EVENT_COMMAND_COUNT));

        for (size_t i = 0; i < e.captions.size(); ++i) {
            MwEventCaption &caption = eventChoiceCaptions[i];
            caption.colour = e.captions[i].colour;
            if (!MwStoryCopyText(e.captions[i].text.c_str(), caption.text, sizeof(caption.text)))
                throw mw1o::import::ImportError("story/events.json: caption " + std::to_string(i + 1) + "'s text");
            captionOffsets[i] = e.captions[i].address;
        }
        size_t next = 0;
        for (size_t r = 0; r < e.events.size(); ++r) {
            MwCampaignEvent &record = campaignEventRecords[r];
            record.eventId = e.events[r].id;
            record.classCode = e.events[r].class_code;
            MwEventCommand **destination = &record.script;
            for (const EventCommand &c : e.events[r].script) {
                MwEventCommand *command = &campaignEventCommands[next++];
                std::memset(command, 0, sizeof(*command));
                command->opcode = c.opcode;
                *destination = command;
                destination = &command->next;
                if (c.opcode == 9) {
                    command->operand.choices.count = static_cast<uint8_t>(c.choices.size());
                    for (size_t i = 0; i < c.choices.size(); ++i) {
                        command->operand.choices.captions[i] = &eventChoiceCaptions[c.choices[i].caption - 1];
                        command->operand.choices.branches[i] =
                            c.choices[i].branch ? &campaignEventRecords[*c.choices[i].branch - 1] : NULL;
                    }
                } else if (!c.action) {
                    switch (c.opcode) {
                    case 0: command->operand.randomThreshold = c.random_threshold; break;
                    case 1: command->operand.date = native_date(c.date); break;
                    case 2: case 5: command->operand.planetSelector = c.planet_selector; break;
                    case 3: command->operand.seenEventId = c.seen_event; break;
                    case 4:
                        command->operand.houseStanding.houseSelector = c.house_selector;
                        command->operand.houseStanding.thresholdHalf = c.threshold_half;
                        break;
                    default: break;
                    }
                } else {
                    switch (c.opcode) {
                    case 2:
                        command->operand.dateWindow.start = native_date(c.window_start);
                        command->operand.dateWindow.end = native_date(c.window_end);
                        break;
                    case 4: command->operand.cashDelta = c.cash_delta; break;
                    case 8: command->operand.planetSelector = c.planet_selector; break;
                    default: break;
                    }
                }
            }
            *destination = NULL;
        }
        for (size_t i = 0; i < e.list_entries.size(); ++i)
            eventListEntries[i] = e.list_entries[i] ? &campaignEventRecords[*e.list_entries[i] - 1] : NULL;
        for (size_t i = 0; i < e.lists.size(); ++i)
            eventLists[i] = e.lists[i] ? &eventListEntries[*e.lists[i] - 1] : NULL;
        fixedCampaignEvent = &campaignEventRecords[e.fixed_event - 1];
    });
}

bool MwStoryCopyPlanetPools(uint8_t *const *pools, const size_t *sizes, size_t poolCount)
{
    return rebuild([&] {
        need(g_events.planet_pools.size() == poolCount,
             "has " + std::to_string(g_events.planet_pools.size()) + " planet pools, the game " + std::to_string(poolCount));
        for (size_t i = 0; i < poolCount; ++i) {
            need(g_events.planet_pools[i].size() == sizes[i], "planet pool " + std::to_string(i + 1) + " has " +
                 std::to_string(g_events.planet_pools[i].size()) + " numbers, the game " + std::to_string(sizes[i]));
            std::memcpy(pools[i], g_events.planet_pools[i].data(), sizes[i]);
        }
    });
}

bool MwStoryCopyAdlibInstruments(uint8_t *rows, size_t rowCount, size_t rowBytes)
{
    return rebuild([&] {
        need(g_music.size() == rowCount,
             "has " + std::to_string(g_music.size()) + " AdLib instruments, the game " + std::to_string(rowCount));
        need(rowBytes >= mw1o::import::kAdlibParameters, "an AdLib instrument row is too short");
        for (size_t i = 0; i < rowCount; ++i) {
            uint8_t *row = rows + i * rowBytes;
            std::memset(row, 0, rowBytes);
            std::memcpy(row, g_music[i].data(), mw1o::import::kAdlibParameters);
        }
    });
}
