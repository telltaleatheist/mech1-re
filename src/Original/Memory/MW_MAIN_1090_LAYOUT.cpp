/* Installation decoders shared by MW_MAIN's screen importers (see the
 * header). Boundary code, not original game helpers. */
#include "MW_MAIN_1090_LAYOUT.h"
#include "mechwarrior/ui_layout.h"
#include <string.h>

enum { UI_CAPTION_HEAD_BYTES = 5, UI_PANEL_HEADER_BYTES = 0x10, UI_MENU_MAX_RECORDS = 0x20 };

bool MwImportUiCaptions(const uint8_t *data,UiCaptionOwner *owners,size_t count)
{
    for (size_t i=0;i<count;++i)
    {
        UiCaptionOwner &owner=owners[i];
        uint8_t head[UI_CAPTION_HEAD_BYTES];
        owner.offset=MwUiCaptionOffset(owner.id);
        if (!owner.offset || (size_t)owner.offset+UI_CAPTION_HEAD_BYTES+owner.capacity>NATIVE_SEGMENT_BYTE_COUNT
            || !MwUiCopyNative(owner.id,head,sizeof(head))) return false;
        const uint8_t *text=data+owner.offset+UI_CAPTION_HEAD_BYTES;
        const uint8_t *end=owner.bytes?text+owner.bytes-1:static_cast<const uint8_t *>(memchr(text,0,owner.capacity));
        if (!end || owner.bytes>owner.capacity) return false;
        owner.display->x=MwReadNativeWord(head,0);owner.display->y=MwReadNativeWord(head,2);
        owner.display->colour=head[4];owner.display->text=owner.text;
        memcpy(owner.text,text,(size_t)(end-text)+1);
    }
    return true;
}
/* The caption whose head is at `offset`, or null. */
static const UiCaptionOwner *FindUiCaption(const UiCaptionOwner *owners,size_t count,uint16_t offset)
{
    for (size_t i=0;i<count;++i) if (owners[i].offset==offset) return &owners[i];
    return NULL;
}
static bool ImportUiPanel(const char *id,MwPanelDescriptor *panel,uint16_t lineCount,const UiCaptionOwner *owners,size_t count,bool list)
{
    uint8_t bytes[UI_PANEL_HEADER_BYTES+2*CAMPAIGN_PANEL_MAX_LINES];
    const size_t size=UI_PANEL_HEADER_BYTES+2*(size_t)lineCount;
    if (lineCount>CAMPAIGN_PANEL_MAX_LINES || MwUiPieceSize(id)!=size || !MwUiCopyNative(id,bytes,size)) return false;
    panel->x=MwReadNativeWord(bytes,0);panel->y=MwReadNativeWord(bytes,2);
    panel->width=MwReadNativeWord(bytes,4);panel->bottomInset=MwReadNativeWord(bytes,6);
    panel->lineCount=MwReadNativeWord(bytes,8);panel->borderEnabled=MwReadNativeWord(bytes,10);
    panel->lineSpacing=MwReadNativeWord(bytes,12);panel->background=MwReadNativeWord(bytes,14);
    if (list?panel->lineCount>lineCount:panel->lineCount!=lineCount) return false;
    for (uint16_t line=0;line<lineCount;++line)
    {
        const UiCaptionOwner *owner=FindUiCaption(owners,count,MwReadNativeWord(bytes,(uint16_t)(UI_PANEL_HEADER_BYTES+line*2)));
        if (!owner) return false;
        panel->lines[line]=owner->display;
    }
    return true;
}
bool MwImportUiPanel(const char *id,MwPanelDescriptor *panel,uint16_t lineCount,const UiCaptionOwner *owners,size_t count)
{
    return ImportUiPanel(id,panel,lineCount,owners,count,false);
}
bool MwImportUiListPanel(const char *id,MwPanelDescriptor *panel,uint16_t lineCount,const UiCaptionOwner *owners,size_t count)
{
    return ImportUiPanel(id,panel,lineCount,owners,count,true);
}
bool MwImportUiMenu(const char *id,MwUiMenu *menu,const UiCaptionOwner *owners,size_t count)
{
    uint8_t bytes[CAMPAIGN_UI_RECORD_BYTES*UI_MENU_MAX_RECORDS];
    const size_t size=(size_t)menu->recordCount*CAMPAIGN_UI_RECORD_BYTES;
    if (size>sizeof(bytes) || MwUiPieceSize(id)!=size || !MwUiCopyNative(id,bytes,size)) return false;
    for (uint16_t row=0;row<menu->recordCount;++row)
    {
        const uint8_t *encoded=bytes+row*CAMPAIGN_UI_RECORD_BYTES;
        MwUiRecord *record=&menu->records[row];
        record->enabled=encoded[0];record->textRecord=encoded[1];
        record->x=MwReadNativeWord(encoded,2);record->y=MwReadNativeWord(encoded,4);
        record->activateCodeOffset=MwReadNativeWord(encoded,6);record->pictureIndex=MwReadNativeWord(encoded,8);
        record->text=NULL;record->textColour=NULL;
        if (record->textRecord && record->pictureIndex)
        {
            const UiCaptionOwner *owner=FindUiCaption(owners,count,(uint16_t)(record->pictureIndex-4));
            if (!owner) return false;
            record->text=owner->display->text;record->textColour=&owner->display->colour;
        }
        record->upRecordIndex=encoded[10];record->downRecordIndex=encoded[11];
        record->leftRecordIndex=encoded[12];record->rightRecordIndex=encoded[13];
    }
    return true;
}
