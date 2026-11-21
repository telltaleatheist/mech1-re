/* Native1090 options, persistence UI and News Net data owners. The installation
 * decoder is boundary code, not a memory registry or an original game helper. */
#include "mechwarrior/campaign_options.h"
#include "mechwarrior/campaign_pilots.h"
#include "mechwarrior/ui_layout.h"
#include <string.h>

MwPanelDescriptor optionsPanel,restorePanel,confirmationPanel,newsMenuPanel;
MwServiceCaption optionsLabels[CAMPAIGN_OPTIONS_ROWS],confirmationLabels[4],newsMenuLabels[3];
MwServiceCaption restoreHeading,restoreCancel;
MwSaveListRow saveListRows[CAMPAIGN_SAVE_ROWS];
uint16_t saveFileHandle,saveFileCount;
const char *saveSearchFilename;
MwSaveListRow *saveListWriteRow;
char selectedSaveFilename[CAMPAIGN_SAVE_FILENAME_BYTES],saveFindPattern[6];
uint8_t saveNamePrompt[0x1B];
char audioOptionLabels[2][4],terrainDetailLabels[3][5];
uint8_t saveDiskFullText[0x2B],saveDiskErrorText[0x30],loadDiskErrorText[0x31];
uint8_t newsAtStart,newsAtEnd;
uint16_t newsLastArticleId;
char newsBoundaryText[0x1B];
MwCampaignEvent **newsArticleCursor,**newsArticleListStart;
static MwUiRecord optionsRecords[CAMPAIGN_OPTIONS_ROWS],restoreRecords[CAMPAIGN_SAVE_ROWS+1];
static MwUiRecord confirmationRecords[2],newsRecords[CAMPAIGN_NEWS_ROWS];
/* No converted writer owns these out-of-menu native FF-link probes. */
static uint8_t optionsNavigationProbe,restoreNavigationProbe,confirmationNavigationProbe,newsNavigationProbe;
MwUiMenu optionsMenu={optionsRecords,CAMPAIGN_OPTIONS_ROWS,&optionsNavigationProbe};
MwUiMenu restoreMenu={restoreRecords,CAMPAIGN_SAVE_ROWS+1,&restoreNavigationProbe};
MwUiMenu confirmationMenu={confirmationRecords,2,&confirmationNavigationProbe};
MwUiMenu newsMenu={newsRecords,CAMPAIGN_NEWS_ROWS,&newsNavigationProbe};

/* A caption: its head (x, y, colour) from the layout (data/ui/options.json,
 * src/Ui), its text from the exe after the head, until slice 5. */
struct UiCaptionOwner
{
    const char *id;
    uint16_t offset;
    MwPanelText *display;
    char *text;
    size_t capacity;
};
static bool ImportUiCaption(const uint8_t *data,const UiCaptionOwner &owner)
{
    uint8_t head[5];
    const uint8_t *end=static_cast<const uint8_t *>(memchr(data+owner.offset+5,0,owner.capacity));
    if (!end || !MwUiCopyNative(owner.id,head,sizeof(head))) return false;
    owner.display->x=MwReadNativeWord(head,0);owner.display->y=MwReadNativeWord(head,2);
    owner.display->colour=head[4];owner.display->text=owner.text;
    memcpy(owner.text,data+owner.offset+5,(size_t)(end-(data+owner.offset+5))+1);return true;
}
/* The caption whose head is at `offset`, or null. */
static const UiCaptionOwner *FindUiCaption(const UiCaptionOwner *owners,size_t count,uint16_t offset)
{
    for (size_t i=0;i<count;++i) if (owners[i].offset==offset) return &owners[i];
    return NULL;
}
/* A panel from the layout: its header, and its lines through the native line
 * pointers the layout rebuilds from its caption names. */
static bool ImportUiPanel(const char *id,MwPanelDescriptor *panel,uint16_t lineCount,const UiCaptionOwner *owners,size_t count)
{
    uint8_t bytes[0x10+2*CAMPAIGN_PANEL_MAX_LINES];
    const size_t size=0x10+2*(size_t)lineCount;
    if (lineCount>CAMPAIGN_PANEL_MAX_LINES || MwUiPieceSize(id)!=size || !MwUiCopyNative(id,bytes,size)) return false;
    panel->x=MwReadNativeWord(bytes,0);panel->y=MwReadNativeWord(bytes,2);
    panel->width=MwReadNativeWord(bytes,4);panel->bottomInset=MwReadNativeWord(bytes,6);
    panel->lineCount=MwReadNativeWord(bytes,8);panel->borderEnabled=MwReadNativeWord(bytes,10);
    panel->lineSpacing=MwReadNativeWord(bytes,12);panel->background=MwReadNativeWord(bytes,14);
    if (panel->lineCount!=lineCount) return false;
    for (uint16_t line=0;line<lineCount;++line)
    {
        const UiCaptionOwner *owner=FindUiCaption(owners,count,MwReadNativeWord(bytes,(uint16_t)(0x10+line*2)));
        if (!owner) return false;
        panel->lines[line]=owner->display;
    }
    return true;
}
/* A menu from the layout. A text record's word is the address of its
 * caption's colour byte (the head's last), so it names the caption. */
static bool ImportUiMenu(const char *id,MwUiMenu *menu,const UiCaptionOwner *owners,size_t count)
{
    uint8_t bytes[CAMPAIGN_UI_RECORD_BYTES*CAMPAIGN_SAVE_ROWS+CAMPAIGN_UI_RECORD_BYTES];
    const size_t size=(size_t)menu->recordCount*CAMPAIGN_UI_RECORD_BYTES;
    if (size>sizeof(bytes) || MwUiPieceSize(id)!=size || !MwUiCopyNative(id,bytes,size)) return false;
    for (uint16_t row=0;row<menu->recordCount;++row)
    {
        const uint8_t *encoded=bytes+row*CAMPAIGN_UI_RECORD_BYTES;
        MwUiRecord *record=&menu->records[row];
        record->enabled=encoded[0];record->textRecord=encoded[1];
        record->x=MwReadNativeWord(encoded,2);record->y=MwReadNativeWord(encoded,4);
        record->activateCodeOffset=MwReadNativeWord(encoded,6);record->pictureIndex=MwReadNativeWord(encoded,8);
        const UiCaptionOwner *owner=record->textRecord?FindUiCaption(owners,count,(uint16_t)(record->pictureIndex-4)):NULL;
        if (!owner) return false;
        record->text=owner->display->text;record->textColour=&owner->display->colour;
        record->upRecordIndex=encoded[10];record->downRecordIndex=encoded[11];
        record->leftRecordIndex=encoded[12];record->rightRecordIndex=encoded[13];
    }
    return true;
}
bool MwImportCampaignOptionsData(const uint8_t *data,size_t byteCount)
{
    if (!data || byteCount<NATIVE_SEGMENT_BYTE_COUNT) return false;
    /* Each caption's place in the exe, where its text still comes from. */
    static const char *const optionIds[CAMPAIGN_OPTIONS_ROWS]={"options.caption.save","options.caption.restore",
        "options.caption.sound","options.caption.detail","options.caption.restart","options.caption.quit","options.caption.continue"};
    static const uint16_t optionOffsets[CAMPAIGN_OPTIONS_ROWS]={0x9D70,0x9D80,0x9D92,0x9DA6,0x9DB8,0x9DCA,0x9DDB};
    static const char *const confirmationIds[4]={"options.caption.confirm_quit","options.caption.confirm_restart",
        "options.caption.yes","options.caption.no"};
    static const uint16_t confirmationOffsets[4]={0x9CD4,0x9CFA,0x9D23,0x9D2C};
    static const char *const newsIds[3]={"options.caption.news_previous","options.caption.news_next","options.caption.news_done"};
    static const uint16_t newsOffsets[3]={0x2516,0x2524,0x252E};
    static const char *const saveIds[CAMPAIGN_SAVE_ROWS]={"options.caption.save_1","options.caption.save_2",
        "options.caption.save_3","options.caption.save_4","options.caption.save_5","options.caption.save_6",
        "options.caption.save_7","options.caption.save_8","options.caption.save_9","options.caption.save_10",
        "options.caption.save_11","options.caption.save_12"};
    UiCaptionOwner owners[CAMPAIGN_OPTIONS_ROWS+4+3+2+CAMPAIGN_SAVE_ROWS];
    size_t count=0;
    for (uint16_t row=0;row<CAMPAIGN_OPTIONS_ROWS;++row)
        owners[count++]={optionIds[row],optionOffsets[row],&optionsLabels[row].display,optionsLabels[row].text,sizeof(optionsLabels[row].text)};
    for (uint16_t row=0;row<4;++row)
        owners[count++]={confirmationIds[row],confirmationOffsets[row],&confirmationLabels[row].display,confirmationLabels[row].text,sizeof(confirmationLabels[row].text)};
    for (uint16_t row=0;row<3;++row)
        owners[count++]={newsIds[row],newsOffsets[row],&newsMenuLabels[row].display,newsMenuLabels[row].text,sizeof(newsMenuLabels[row].text)};
    owners[count++]={"options.caption.restore_heading",0x9E79,&restoreHeading.display,restoreHeading.text,sizeof(restoreHeading.text)};
    owners[count++]={"options.caption.restore_cancel",0x9F6C,&restoreCancel.display,restoreCancel.text,sizeof(restoreCancel.text)};
    for (uint16_t row=0;row<CAMPAIGN_SAVE_ROWS;++row)
        owners[count++]={saveIds[row],(uint16_t)(0x9E94+row*0x12),&saveListRows[row].display,saveListRows[row].caption.text,sizeof(saveListRows[row].caption.text)};
    for (size_t i=0;i<count;++i) if (!ImportUiCaption(data,owners[i])) return false;
    if (!ImportUiPanel("options.panel.options",&optionsPanel,7,owners,count)
        || !ImportUiPanel("options.panel.restore",&restorePanel,14,owners,count)
        || !ImportUiPanel("options.panel.confirmation",&confirmationPanel,3,owners,count)
        || !ImportUiPanel("options.panel.news",&newsMenuPanel,3,owners,count)
        || !ImportUiMenu("options.menu.options",&optionsMenu,owners,count)
        || !ImportUiMenu("options.menu.restore",&restoreMenu,owners,count)
        || !ImportUiMenu("options.menu.confirmation",&confirmationMenu,owners,count)
        || !ImportUiMenu("options.menu.news",&newsMenu,owners,count)) return false;
    barMenu.sentinelEnabled=&optionsMenu.records[5].rightRecordIndex; /*1090:9E3E*/
    optionsNavigationProbe=data[0xABDD];restoreNavigationProbe=data[0xAD6D];
    confirmationNavigationProbe=data[0xAB26];newsNavigationProbe=data[0x332A];
    saveFileHandle=MwReadNativeWord(data,0xA04C);saveFileCount=MwReadNativeWord(data,0xA04E);
    memcpy(selectedSaveFilename,data+0xA064,sizeof(selectedSaveFilename));
    memcpy(saveFindPattern,data+0xA056,sizeof(saveFindPattern));
    memcpy(saveNamePrompt,data+0xA031,sizeof(saveNamePrompt));
    memcpy(audioOptionLabels,data+0xA071,sizeof(audioOptionLabels));
    memcpy(terrainDetailLabels,data+0xA079,sizeof(terrainDetailLabels));
    memcpy(saveDiskFullText,data+0xA088,sizeof(saveDiskFullText));
    memcpy(saveDiskErrorText,data+0xA0B3,sizeof(saveDiskErrorText));
    memcpy(loadDiskErrorText,data+0xA0E3,sizeof(loadDiskErrorText));
    newsAtStart=data[0x257F];newsAtEnd=data[0x2580];newsLastArticleId=MwReadNativeWord(data,0x2562);
    memcpy(newsBoundaryText,data+0x2564,sizeof(newsBoundaryText));
    newsArticleCursor=NULL;newsArticleListStart=NULL;
    saveSearchFilename=NULL;saveListWriteRow=NULL;
    return true;
}
