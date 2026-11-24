/* Native1090 options, persistence UI and News Net data owners. The installation
 * decoder is boundary code, not a memory registry or an original game helper. */
#include "mechwarrior/campaign_options.h"
#include "mechwarrior/campaign_pilots.h"
#include "MW_MAIN_1090_LAYOUT.h"
#include "mechwarrior/story.h"
#include "mechwarrior/native_start.h"
#include <string.h>
static MwUiRecord optionsRecords[CAMPAIGN_OPTIONS_ROWS],restoreRecords[CAMPAIGN_SAVE_ROWS+1];
static MwUiRecord confirmationRecords[2],newsRecords[CAMPAIGN_NEWS_ROWS];
/* No converted writer owns these out-of-menu native FF-link probes. */
static uint8_t optionsNavigationProbe,restoreNavigationProbe,confirmationNavigationProbe,newsNavigationProbe;
CampaignOptionsState campaignOptions={
    .optionsMenu={optionsRecords,CAMPAIGN_OPTIONS_ROWS,&optionsNavigationProbe},
    .restoreMenu={restoreRecords,CAMPAIGN_SAVE_ROWS+1,&restoreNavigationProbe},
    .confirmationMenu={confirmationRecords,2,&confirmationNavigationProbe},
    .newsMenu={newsRecords,CAMPAIGN_NEWS_ROWS,&newsNavigationProbe}
};

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
        owners[count++]={optionIds[row],&campaignOptions.optionsLabels[row].display,campaignOptions.optionsLabels[row].text,sizeof(campaignOptions.optionsLabels[row].text),0};
    for (uint16_t row=0;row<4;++row)
        owners[count++]={confirmationIds[row],&campaignOptions.confirmationLabels[row].display,campaignOptions.confirmationLabels[row].text,sizeof(campaignOptions.confirmationLabels[row].text),0};
    for (uint16_t row=0;row<3;++row)
        owners[count++]={newsIds[row],&campaignOptions.newsMenuLabels[row].display,campaignOptions.newsMenuLabels[row].text,sizeof(campaignOptions.newsMenuLabels[row].text),0};
    owners[count++]={"options.caption.restore_heading",&campaignOptions.restoreHeading.display,campaignOptions.restoreHeading.text,sizeof(campaignOptions.restoreHeading.text),0};
    owners[count++]={"options.caption.restore_cancel",&campaignOptions.restoreCancel.display,campaignOptions.restoreCancel.text,sizeof(campaignOptions.restoreCancel.text),0};
    for (uint16_t row=0;row<CAMPAIGN_SAVE_ROWS;++row)
        owners[count++]={saveIds[row],&campaignOptions.saveListRows[row].display,campaignOptions.saveListRows[row].caption.text,sizeof(campaignOptions.saveListRows[row].caption.text),0};
    if (!MwImportUiCaptions(owners,count)
        || !MwImportUiPanel("options.panel.options",&campaignOptions.optionsPanel,7,owners,count)
        || !MwImportUiPanel("options.panel.restore",&campaignOptions.restorePanel,14,owners,count)
        || !MwImportUiPanel("options.panel.confirmation",&campaignOptions.confirmationPanel,3,owners,count)
        || !MwImportUiPanel("options.panel.news",&campaignOptions.newsMenuPanel,3,owners,count)
        || !MwImportUiMenu("options.menu.options",&campaignOptions.optionsMenu,owners,count)
        || !MwImportUiMenu("options.menu.restore",&campaignOptions.restoreMenu,owners,count)
        || !MwImportUiMenu("options.menu.confirmation",&campaignOptions.confirmationMenu,owners,count)
        || !MwImportUiMenu("options.menu.news",&campaignOptions.newsMenu,owners,count)) return false;
    campaignPilots.barMenu.sentinelEnabled=&campaignOptions.optionsMenu.records[5].rightRecordIndex; /*1090:9E3E*/
    /* The menus' past-end probes (story/native.json, src/Import/pieces.txt). */
    if (!MwStoryCopyBytes("probe.options",&optionsNavigationProbe,1)
        || !MwStoryCopyBytes("probe.restore",&restoreNavigationProbe,1)
        || !MwStoryCopyBytes("probe.confirmation",&confirmationNavigationProbe,1)
        || !MwStoryCopyBytes("probe.news",&newsNavigationProbe,1)) return false;
    campaignOptions.saveFileHandle=0; /*A04C*/campaignOptions.saveFileCount=0; /*A04E*/
    memset(campaignOptions.selectedSaveFilename,0,sizeof(campaignOptions.selectedSaveFilename)); /*A064*/
    memcpy(campaignOptions.saveFindPattern,mwSaveFindPattern,sizeof(campaignOptions.saveFindPattern)); /*A056*/
    /* The screen's words, from story/words.json (options.word.*), as the exe holds them. */
    if (!MwWordsCopyBlock("options.word.save_prompt",campaignOptions.saveNamePrompt,sizeof(campaignOptions.saveNamePrompt))
        || !MwWordsCopyBlock("options.word.audio_labels",reinterpret_cast<uint8_t *>(campaignOptions.audioOptionLabels),sizeof(campaignOptions.audioOptionLabels))
        || !MwWordsCopyBlock("options.word.terrain_labels",reinterpret_cast<uint8_t *>(campaignOptions.terrainDetailLabels),sizeof(campaignOptions.terrainDetailLabels))
        || !MwWordsCopyBlock("options.word.disk_full",campaignOptions.saveDiskFullText,sizeof(campaignOptions.saveDiskFullText))
        || !MwWordsCopyBlock("options.word.save_disk_error",campaignOptions.saveDiskErrorText,sizeof(campaignOptions.saveDiskErrorText))
        || !MwWordsCopyBlock("options.word.load_disk_error",campaignOptions.loadDiskErrorText,sizeof(campaignOptions.loadDiskErrorText))
        || !MwWordsCopyBlock("options.word.news_boundary",reinterpret_cast<uint8_t *>(campaignOptions.newsBoundaryText),sizeof(campaignOptions.newsBoundaryText)))
        return false;
    campaignOptions.newsAtStart=0; /*257F*/campaignOptions.newsAtEnd=0; /*2580*/campaignOptions.newsLastArticleId=0; /*2562*/
    campaignOptions.newsArticleCursor=NULL;campaignOptions.newsArticleListStart=NULL;
    campaignOptions.saveSearchFilename=NULL;campaignOptions.saveListWriteRow=NULL;
    return true;
}
