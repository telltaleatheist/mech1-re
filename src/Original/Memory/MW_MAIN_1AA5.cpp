#include "mechwarrior/campaign_events.h"
#include "mechwarrior/story.h"
#include <string.h>
CampaignEventTextState campaignEventText;

/* The text bank (1AA5) from story/event_text.json, laid out as the exe holds
 * it: the storage 00EE..E089 and the directory at 1AA5:0000. Only the
 * authored writable text bytes survive; the directory becomes references into
 * their ONE owner. Runs at start and on every EXEC, which restores the planet
 * names the events wrote into the text. */
bool MwImportCampaignEventText(void)
{
    uint16_t directory[EVENT_TEXT_DIRECTORY_COUNT];
    if (!MwStoryBuildEventText(campaignEventText.eventTextStorage,sizeof(campaignEventText.eventTextStorage),EVENT_TEXT_STORAGE_START,
        directory,EVENT_TEXT_DIRECTORY_COUNT)) return false;
    for (uint16_t i=0;i<EVENT_TEXT_DIRECTORY_COUNT;++i)
    {
        uint16_t offset=directory[i];
        if (!offset) {campaignEventText.eventTextRecords[i]=NULL;continue;}
        if (offset<EVENT_TEXT_STORAGE_START || offset>=EVENT_TEXT_STORAGE_END) return false;
        campaignEventText.eventTextRecords[i]=&campaignEventText.eventTextStorage[offset-EVENT_TEXT_STORAGE_START];
    }
    return true;
}
