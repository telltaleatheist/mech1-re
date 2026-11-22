/* Native1090 options, persistence UI and News Net data owners. The installation
 * decoder is boundary code, not a memory registry or an original game helper. */
#include "mechwarrior/campaign_options.h"
#include "mechwarrior/campaign_pilots.h"
#include "MW_MAIN_1090_LAYOUT.h"
#include "mechwarrior/story.h"
#include "mechwarrior/native_start.h"
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

bool MwImportCampaignOptionsData(void)
{
    static const char *const optionIds[CAMPAIGN_OPTIONS_ROWS]={"options.caption.save","options.caption.restore",
        "options.caption.sound","options.caption.detail","options.caption.restart","options.caption.quit","options.caption.continue"};
    static const char *const confirmationIds[4]={"options.caption.confirm_quit","options.caption.confirm_restart",
        "options.caption.yes","options.caption.no"};
    static const char *const newsIds[3]={"options.caption.news_previous","options.caption.news_next","options.caption.news_done"};
    static const char *const saveIds[CAMPAIGN_SAVE_ROWS]={"options.caption.save_1","options.caption.save_2",
        "options.caption.save_3","options.caption.save_4","options.caption.save_5","options.caption.save_6",
        "options.caption.save_7","options.caption.save_8","options.caption.save_9","options.caption.save_10",
        "options.caption.save_11","options.caption.save_12"};
    UiCaptionOwner owners[CAMPAIGN_OPTIONS_ROWS+4+3+2+CAMPAIGN_SAVE_ROWS];
    size_t count=0;
    for (uint16_t row=0;row<CAMPAIGN_OPTIONS_ROWS;++row)
        owners[count++]={optionIds[row],&optionsLabels[row].display,optionsLabels[row].text,sizeof(optionsLabels[row].text),0};
    for (uint16_t row=0;row<4;++row)
        owners[count++]={confirmationIds[row],&confirmationLabels[row].display,confirmationLabels[row].text,sizeof(confirmationLabels[row].text),0};
    for (uint16_t row=0;row<3;++row)
        owners[count++]={newsIds[row],&newsMenuLabels[row].display,newsMenuLabels[row].text,sizeof(newsMenuLabels[row].text),0};
    owners[count++]={"options.caption.restore_heading",&restoreHeading.display,restoreHeading.text,sizeof(restoreHeading.text),0};
    owners[count++]={"options.caption.restore_cancel",&restoreCancel.display,restoreCancel.text,sizeof(restoreCancel.text),0};
    for (uint16_t row=0;row<CAMPAIGN_SAVE_ROWS;++row)
        owners[count++]={saveIds[row],&saveListRows[row].display,saveListRows[row].caption.text,sizeof(saveListRows[row].caption.text),0};
    if (!MwImportUiCaptions(owners,count)
        || !MwImportUiPanel("options.panel.options",&optionsPanel,7,owners,count)
        || !MwImportUiPanel("options.panel.restore",&restorePanel,14,owners,count)
        || !MwImportUiPanel("options.panel.confirmation",&confirmationPanel,3,owners,count)
        || !MwImportUiPanel("options.panel.news",&newsMenuPanel,3,owners,count)
        || !MwImportUiMenu("options.menu.options",&optionsMenu,owners,count)
        || !MwImportUiMenu("options.menu.restore",&restoreMenu,owners,count)
        || !MwImportUiMenu("options.menu.confirmation",&confirmationMenu,owners,count)
        || !MwImportUiMenu("options.menu.news",&newsMenu,owners,count)) return false;
    barMenu.sentinelEnabled=&optionsMenu.records[5].rightRecordIndex; /*1090:9E3E*/
    /* The menus' past-end probes (story/native.json, src/Import/pieces.txt). */
    if (!MwStoryCopyBytes("probe.options",&optionsNavigationProbe,1)
        || !MwStoryCopyBytes("probe.restore",&restoreNavigationProbe,1)
        || !MwStoryCopyBytes("probe.confirmation",&confirmationNavigationProbe,1)
        || !MwStoryCopyBytes("probe.news",&newsNavigationProbe,1)) return false;
    saveFileHandle=0; /*A04C*/saveFileCount=0; /*A04E*/
    memset(selectedSaveFilename,0,sizeof(selectedSaveFilename)); /*A064*/
    memcpy(saveFindPattern,mwSaveFindPattern,sizeof(saveFindPattern)); /*A056*/
    /* The screen's words, from story/words.json (options.word.*), as the exe holds them. */
    if (!MwWordsCopyBlock("options.word.save_prompt",saveNamePrompt,sizeof(saveNamePrompt))
        || !MwWordsCopyBlock("options.word.audio_labels",reinterpret_cast<uint8_t *>(audioOptionLabels),sizeof(audioOptionLabels))
        || !MwWordsCopyBlock("options.word.terrain_labels",reinterpret_cast<uint8_t *>(terrainDetailLabels),sizeof(terrainDetailLabels))
        || !MwWordsCopyBlock("options.word.disk_full",saveDiskFullText,sizeof(saveDiskFullText))
        || !MwWordsCopyBlock("options.word.save_disk_error",saveDiskErrorText,sizeof(saveDiskErrorText))
        || !MwWordsCopyBlock("options.word.load_disk_error",loadDiskErrorText,sizeof(loadDiskErrorText))
        || !MwWordsCopyBlock("options.word.news_boundary",reinterpret_cast<uint8_t *>(newsBoundaryText),sizeof(newsBoundaryText)))
        return false;
    newsAtStart=0; /*257F*/newsAtEnd=0; /*2580*/newsLastArticleId=0; /*2562*/
    newsArticleCursor=NULL;newsArticleListStart=NULL;
    saveSearchFilename=NULL;saveListWriteRow=NULL;
    return true;
}
