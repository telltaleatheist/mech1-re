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
    /* The dispatch topology (1090:5E78 conditions, 5E8C actions) is native
     * code, not editable scenario rules: the import table's check pieces
     * (src/Import/pieces.txt) refuse an installation whose tables differ. */
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
