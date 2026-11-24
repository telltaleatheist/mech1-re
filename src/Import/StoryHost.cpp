// The game's door to the story file: imported if needed, then held in memory
// for the importers in Original/Memory, which run again on every EXEC.
#include "mechwarrior/problem.h"
#include "mechwarrior/story.h"

#include <cstdio>
#include <cstring>

#include "Events.h"
#include "Records.h"
#include "mechwarrior/campaign.h"
#include "mechwarrior/campaign_economy.h"
#include "mechwarrior/campaign_events.h"
#include "mechwarrior/campaign_mechanics.h"
#include "mechwarrior/campaign_startup.h"
#include "Story.h"

namespace {
// Everything the story file holds.
struct Story {
    mw1o::import::StoryText texts;
    mw1o::import::Words words;
    mw1o::import::Native native;
    std::vector<mw1o::import::PilotRecord> pilots;
    mw1o::import::Planets planets;
    mw1o::import::Missions missions;
    std::vector<mw1o::import::EventTextRecord> event_text;
    mw1o::import::Events events;
    std::string btech_revision;
};
Story g_story;

void need(bool ok, const std::string &why)
{
    if (!ok) throw mw1o::import::ImportError("story/events.json: " + why);
}

void need_events(const mw1o::import::Events &e)
{
    need(e.captions.size() == CAMPAIGN_EVENT_CAPTION_COUNT,
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
}

void need_pools(const mw1o::import::Events &e, const size_t *sizes, size_t count)
{
    need(e.planet_pools.size() == count,
         "has " + std::to_string(e.planet_pools.size()) + " planet pools, the game " + std::to_string(count));
    for (size_t i = 0; i < count; ++i)
        need(e.planet_pools[i].size() == sizes[i], "planet pool " + std::to_string(i + 1) + " has " +
             std::to_string(e.planet_pools[i].size()) + " numbers, the game " + std::to_string(sizes[i]));
}

// What the campaign's importers would refuse, before they run: the records are
// rebuilt into scratch storage of the game's sizes. Throws.
void check_story(const Story &s)
{
    mw1o::import::check_texts(s.texts);
    std::vector<uint8_t> storage;
    std::vector<uint16_t> offsets, more;
    const auto room = [&](size_t size) -> std::span<uint8_t> { storage.assign(size, 0); return storage; };
    const auto words = [](std::vector<uint16_t> &v, size_t count) -> std::span<uint16_t> { v.assign(count, 0); return v; };
    mw1o::import::build_pilots(s.pilots, s.texts, room(sizeof(campaignMechanics.pilotDefinitionStorage)),
                               PILOT_DEFINITIONS_START, words(offsets, CAMPAIGN_PILOT_ID_COUNT));
    mw1o::import::build_planets(s.planets, s.texts, room(sizeof(campaignCore.planetRecordStorage)), PLANET_RECORDS_START,
                                words(offsets, CAMPAIGN_PLANET_COUNT), words(more, TRAVEL_PLANET_DIRECTORY_COUNT));
    std::vector<uint8_t> maximum(MISSION_CATEGORY_COUNT);
    mw1o::import::build_missions(s.missions, s.texts, room(sizeof(campaignEconomy.missionDescriptionStorage)),
                                 MISSION_DESCRIPTION_STORAGE_START, maximum, words(offsets, MISSION_CATEGORY_COUNT),
                                 words(more, MISSION_DESCRIPTION_LIST_WORDS), MISSION_DESCRIPTION_LIST_START);
    mw1o::import::build_event_text(s.event_text, s.texts, room(sizeof(campaignEventText.eventTextStorage)),
                                   EVENT_TEXT_STORAGE_START, words(offsets, EVENT_TEXT_DIRECTORY_COUNT));
    need_events(s.events);
    for (const auto &caption : s.events.captions) {
        const auto it = s.texts.find(caption.text);
        need(it != s.texts.end() && it->second.find('\0') == std::string::npos &&
                 it->second.size() < sizeof(MwEventCaption::text), "a caption has no text that fits");
    }
    const size_t pools[6] = {sizeof(campaignStartup.eventPlanetChoicePool0), sizeof(campaignStartup.eventPlanetChoicePool1),
        sizeof(campaignStartup.eventPlanetChoicePool2), sizeof(campaignStartup.eventPlanetChoicePool3),
        sizeof(campaignStartup.eventPlanetChoicePool4), sizeof(campaignStartup.eventPlanetChoicePool5)};
    need_pools(s.events, pools, 6);
}

Story load_story(const std::filesystem::path &writeDirectory)
{
    Story s;
    s.texts = mw1o::import::load_story_text(writeDirectory);
    s.words = mw1o::import::load_words(writeDirectory);
    s.native = mw1o::import::load_native(writeDirectory);
    s.pilots = mw1o::import::load_pilots(writeDirectory);
    s.planets = mw1o::import::load_planets(writeDirectory);
    s.missions = mw1o::import::load_missions(writeDirectory);
    s.event_text = mw1o::import::load_event_text(writeDirectory);
    s.events = mw1o::import::load_events(writeDirectory);
    s.btech_revision = mw1o::import::load_btech_revision(writeDirectory);
    check_story(s);
    return s;
}

// Runs a rebuild; an error goes to stderr and returns false.
template <typename Build>
bool rebuild(Build build)
{
    try {
        build();
        return true;
    } catch (const std::exception &e) {
        MwReportProblem(e.what());
        return false;
    }
}
}

bool MwHostPrepareStory(const char *gameDirectory, const char *writeDirectory)
{
    using namespace mw1o::import;
    try {
        const std::filesystem::path home = writeDirectory;
        const bool imported = ensure_story(gameDirectory, home);
        if (imported)
            std::fprintf(stderr, "MechWarrior: imported the story file into %s\n", utf8_path(story_dir(home)).c_str());
        std::string why;
        try {
            g_story = load_story(home);
            return true;
        } catch (const std::exception &e) {
            why = e.what();
        }
        // Once in a start: what was just imported is not imported again.
        if (imported) throw ImportError("the story file just imported can't be used: " + why);
        repair_story(why, gameDirectory, home);
        try {
            g_story = load_story(home);
        } catch (const std::exception &e) {
            throw ImportError("the story file was damaged (" + why + "), and imported again it can't be used: " + e.what());
        }
        std::fprintf(stderr, "MechWarrior: the story file was damaged (%s): imported again into %s, the old files are in %s\n",
                     why.c_str(), utf8_path(story_dir(home)).c_str(), utf8_path(replaced_dir(home)).c_str());
        return true;
    } catch (const std::exception &e) {
        MwReportProblem(e.what());
        return false;
    }
}

const char *MwStoryBtechRevision(void)
{
    return g_story.btech_revision.c_str();
}

bool MwStoryCopyText(const char *id, char *text, size_t capacity)
{
    const auto it = g_story.texts.find(id);
    if (it == g_story.texts.end()) {
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
    const auto it = g_story.native.find(id);
    if (it == g_story.native.end()) {
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
    const auto it = g_story.words.find(id);
    if (!piece || it == g_story.words.end() || !mw1o::import::is_string(piece->kind)) {
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
    const auto it = g_story.words.find(id);
    if (!piece || it == g_story.words.end()) return false;
    if (it->second.size() != size) {
        std::fprintf(stderr, "MechWarrior: the word \"%s\" is %zu bytes, the game's %zu\n", id, it->second.size(), size);
        return false;
    }
    std::memcpy(bytes, it->second.data(), size);
    return true;
}

bool MwStoryBuildPilots(uint8_t *storage, size_t capacity, uint16_t base, uint16_t *offsets, size_t offsetCount)
{
    return rebuild([&] {
        mw1o::import::build_pilots(g_story.pilots, g_story.texts, {storage, capacity}, base, {offsets, offsetCount});
    });
}

bool MwStoryBuildPlanets(uint8_t *storage, size_t capacity, uint16_t base, uint16_t *offsets, size_t offsetCount,
    uint16_t *alphabetical, size_t alphabeticalCount)
{
    return rebuild([&] {
        mw1o::import::build_planets(g_story.planets, g_story.texts, {storage, capacity}, base, {offsets, offsetCount},
                                    {alphabetical, alphabeticalCount});
    });
}

bool MwStoryBuildMissions(uint8_t *storage, size_t capacity, uint16_t base, uint8_t *maximumIndices,
    uint16_t *listOffsets, size_t categoryCount, uint16_t *entries, size_t entryCount, uint16_t entryBase)
{
    return rebuild([&] {
        mw1o::import::build_missions(g_story.missions, g_story.texts, {storage, capacity}, base, {maximumIndices, categoryCount},
                                     {listOffsets, categoryCount}, {entries, entryCount}, entryBase);
    });
}

bool MwStoryBuildEventText(uint8_t *storage, size_t capacity, uint16_t base, uint16_t *directory, size_t directoryCount)
{
    return rebuild([&] {
        mw1o::import::build_event_text(g_story.event_text, g_story.texts, {storage, capacity}, base, {directory, directoryCount});
    });
}

namespace {

MwEventDate native_date(const mw1o::import::EventDate &d)
{
    return MwEventDate{d.month, d.submonth, d.year};
}

}  // namespace

bool MwStoryBuildCampaignEvents(uint16_t *captionOffsets, size_t captionCount)
{
    return rebuild([&] {
        using mw1o::import::EventCommand;
        const auto &e = g_story.events;
        need_events(e);
        need(captionCount == CAMPAIGN_EVENT_CAPTION_COUNT, "the game asks for " + std::to_string(captionCount) + " captions");

        for (size_t i = 0; i < e.captions.size(); ++i) {
            MwEventCaption &caption = campaignEvents.eventChoiceCaptions[i];
            caption.colour = e.captions[i].colour;
            if (!MwStoryCopyText(e.captions[i].text.c_str(), caption.text, sizeof(caption.text)))
                throw mw1o::import::ImportError("story/events.json: caption " + std::to_string(i + 1) + "'s text");
            captionOffsets[i] = e.captions[i].address;
        }
        size_t next = 0;
        for (size_t r = 0; r < e.events.size(); ++r) {
            MwCampaignEvent &record = campaignEvents.campaignEventRecords[r];
            record.eventId = e.events[r].id;
            record.classCode = e.events[r].class_code;
            MwEventCommand **destination = &record.script;
            for (const EventCommand &c : e.events[r].script) {
                MwEventCommand *command = &campaignEvents.campaignEventCommands[next++];
                std::memset(command, 0, sizeof(*command));
                command->opcode = c.opcode;
                *destination = command;
                destination = &command->next;
                if (c.opcode == 9) {
                    command->operand.choices.count = static_cast<uint8_t>(c.choices.size());
                    for (size_t i = 0; i < c.choices.size(); ++i) {
                        command->operand.choices.captions[i] = &campaignEvents.eventChoiceCaptions[c.choices[i].caption - 1];
                        command->operand.choices.branches[i] =
                            c.choices[i].branch ? &campaignEvents.campaignEventRecords[*c.choices[i].branch - 1] : NULL;
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
            campaignEvents.eventListEntries[i] = e.list_entries[i] ? &campaignEvents.campaignEventRecords[*e.list_entries[i] - 1] : NULL;
        for (size_t i = 0; i < e.lists.size(); ++i)
            campaignEvents.eventLists[i] = e.lists[i] ? &campaignEvents.eventListEntries[*e.lists[i] - 1] : NULL;
        campaignEvents.fixedCampaignEvent = &campaignEvents.campaignEventRecords[e.fixed_event - 1];
    });
}

bool MwStoryCopyPlanetPools(uint8_t *const *pools, const size_t *sizes, size_t poolCount)
{
    return rebuild([&] {
        need_pools(g_story.events, sizes, poolCount);
        for (size_t i = 0; i < poolCount; ++i)
            std::memcpy(pools[i], g_story.events.planet_pools[i].data(), sizes[i]);
    });
}
