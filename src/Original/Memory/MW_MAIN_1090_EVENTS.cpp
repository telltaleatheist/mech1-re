/* The campaign event data: the typed script from the story file
 * (src/Import/Events.*, which decodes 1090:5CF4..655C at import), the choice
 * menus and the dispatch check from the installation. No runtime address
 * registry, retained executable image or additional game procedure. */
#include "mechwarrior/campaign_events.h"
#include "mechwarrior/story.h"
#include "mechwarrior/ui_layout.h"
#include <string.h>

bool MwImportCampaignEventData(const uint8_t *data,size_t byteCount)
{
    if (!data || byteCount<NATIVE_SEGMENT_BYTE_COUNT) return false;
    /* The dispatch topology is native code, not editable scenario rules. Reject
     * an unsupported installation here rather than execute the wrong C handler. */
    const uint16_t conditions[10]={0x6425,0x643D,0x64BB,0x64ED,0x6503,0x652F,0x6561,0,0x6562,0x66CC};
    const uint16_t actions[10]={0x6587,0x6588,0x65B1,0x662C,0x662D,0x665B,0x6661,0x6667,0x6668,0x66CC};
    for (uint16_t i=0;i<10;++i)
        if (MwReadNativeWord(data,(uint16_t)(0x5E78+i*2))!=conditions[i]
            || MwReadNativeWord(data,(uint16_t)(0x5E8C+i*2))!=actions[i]) return false;
    /* The captions, records, commands, lists and fixed event come from
     * story/events.json (src/Import/Events.h), in the original importer's
     * order; captionOffsets are the captions' original addresses, which the
     * choice menus below (layout, still read here) name them by. */
    uint16_t captionOffsets[CAMPAIGN_EVENT_CAPTION_COUNT];
    if (!MwStoryBuildCampaignEvents(captionOffsets,CAMPAIGN_EVENT_CAPTION_COUNT)) return false;
    for (uint16_t menuIndex=0;menuIndex<2;++menuIndex)
    {
        MwUiMenu *menu=menuIndex?&eventThreeChoiceMenu:&eventTwoChoiceMenu;
        /* The records from the layout (data/ui/events.json, src/Ui). */
        const char *id=menuIndex?"events.menu.three_choices":"events.menu.two_choices";
        uint8_t layout[3*CAMPAIGN_UI_RECORD_BYTES];
        const size_t size=(size_t)menu->recordCount*CAMPAIGN_UI_RECORD_BYTES;
        if (size>sizeof(layout) || MwUiPieceSize(id)!=size || !MwUiCopyNative(id,layout,size)) return false;
        for (uint16_t i=0;i<menu->recordCount;++i)
        {
            uint16_t at=i*CAMPAIGN_UI_RECORD_BYTES;
            MwUiRecord *record=&menu->records[i];
            record->enabled=layout[at];record->textRecord=layout[at+1];
            record->x=MwReadNativeWord(layout,at+2);record->y=MwReadNativeWord(layout,at+4);
            record->activateCodeOffset=MwReadNativeWord(layout,at+6);
            record->pictureIndex=0;record->text=NULL;record->textColour=NULL;
            uint16_t captionOffset=MwReadNativeWord(layout,at+8);
            for (uint16_t caption=0;caption<CAMPAIGN_EVENT_CAPTION_COUNT;++caption)
                if (captionOffsets[caption]==captionOffset)
                {
                    record->text=eventChoiceCaptions[caption].text;
                    record->textColour=&eventChoiceCaptions[caption].colour;
                }
            if (captionOffset && !record->text) return false;
            record->upRecordIndex=layout[at+10];record->downRecordIndex=layout[at+11];
            record->leftRecordIndex=layout[at+12];record->rightRecordIndex=layout[at+13];
        }
    }
    for (uint16_t i=0;i<3;++i) eventChoiceBranches[i]=NULL;
    for (uint16_t i=0;i<7;++i) eventClassHandlers[i]=i==3?MwShowClassThreeEventPages:MwShowCampaignEventText;
    campaignEventChoice=MwEventOpenChoiceContinuation;
    campaignEventPlanetSubstitution=MwEventSubstitutePlanetName;
    eventOperandCursor=NULL;
    return true;
}
