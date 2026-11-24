#ifndef MECHWARRIOR_CAMPAIGN_OPTIONS_H
#define MECHWARRIOR_CAMPAIGN_OPTIONS_H
#include "mechwarrior/ega_campaign.h"
#include "mechwarrior/campaign_service.h"
#include "mechwarrior/campaign_events.h"
#include "mechwarrior/campaign_exchange.h"

enum { CAMPAIGN_OPTIONS_ROWS=7, CAMPAIGN_SAVE_ROWS=12, CAMPAIGN_SAVE_STEM_BYTES=8,
    CAMPAIGN_SAVE_FILENAME_BYTES=13, CAMPAIGN_SAVE_BYTES=CAMPAIGN_EXCHANGE_BYTES,
    CAMPAIGN_NEWS_ROWS=3,
    OPTION_SAVE=0, OPTION_RESTORE, OPTION_SOUND, OPTION_DETAIL, OPTION_RESTART,
    OPTION_QUIT, OPTION_CONTINUE };
enum { OPTIONS_HUB_ROW=5,OPTIONS_LANDSCAPE_PICTURE_BASE=3,
    OPTIONS_SOUND_LABEL_COLUMN=11,OPTIONS_DETAIL_LABEL_COLUMN=8,
    OPTIONS_VALUE_FOREGROUND=Campaign_Pal_EGA_Yellow,OPTIONS_VALUE_BACKGROUND=Campaign_Pal_EGA_DarkGrey };
typedef struct MwSaveListRow
{
    MwPanelText display;
    union
    {
        struct { char rowNumber[4],filename[9]; } columns;
        char text[13];
    } caption;
} MwSaveListRow;
/* Campaign state owned by campaign_options.h (plan, slice B2). */
struct CampaignOptionsState
{
    MwPanelDescriptor optionsPanel, restorePanel, confirmationPanel, newsMenuPanel;
    MwServiceCaption optionsLabels[CAMPAIGN_OPTIONS_ROWS], confirmationLabels[4], newsMenuLabels[3];
    MwServiceCaption restoreHeading, restoreCancel;
    MwSaveListRow saveListRows[CAMPAIGN_SAVE_ROWS];
    MwUiMenu optionsMenu, restoreMenu, confirmationMenu, newsMenu;
    uint16_t saveFileCount;
    MwSaveListRow *saveListWriteRow;
    char selectedSaveFilename[CAMPAIGN_SAVE_FILENAME_BYTES], saveFindPattern[6];
    uint8_t saveNamePrompt[0x1B];
    char audioOptionLabels[2][4], terrainDetailLabels[3][5];
    uint8_t saveDiskFullText[0x2B], saveDiskErrorText[0x30], loadDiskErrorText[0x31];
    uint8_t newsAtStart, newsAtEnd;
    uint16_t newsLastArticleId;
    char newsBoundaryText[0x1B];
    MwCampaignEvent **newsArticleCursor, **newsArticleListStart;
};
extern CampaignOptionsState campaignOptions;
bool MwImportCampaignOptionsData(void);
uint16_t MwEnterOptionsMenuContinuation(uint16_t entryOffset);
void MwUpdateAudioOptionLabel(void);
void MwUpdateTerrainDetailOptionLabel(void);
void MwToggleAudioOption(void);
void MwCycleTerrainDetailOption(void);
uint16_t MwConfirmQuitCampaignContinuation(void);
uint16_t MwConfirmRestartCampaignContinuation(void);
uint16_t MwQuitFromOptionsContinuation(void);
uint16_t MwRestartFromOptionsContinuation(void);
enum class CampaignSaveWrite { Written, NotCreated, DiskFull };
CampaignSaveWrite MwWriteCampaignSaveFile(const char *filename);
bool MwReadCampaignSaveFile(const char *filename);
uint16_t MwSaveCampaignCallback(void);
uint16_t MwShowOptionsMessageAndReturnContinuation(const MwNativeByteView *text,uint16_t textOffset);
uint16_t MwShowLoadCampaignCallback(void);
bool MwEnumerateCampaignSaves(void);
uint16_t MwLoadSelectedCampaignCallback(void);
void MwInitializeNewsArticleCursor(void);
bool MwFindNextNewsArticle(uint16_t *articleId);
bool MwFindPreviousNewsArticle(uint16_t *articleId);
uint16_t MwOpenNewsNetContinuation(void);
uint16_t MwNextNewsArticleContinuation(void);
uint16_t MwPreviousNewsArticleContinuation(void);
void MwDrawNewsBoundaryMessage(void);
void MwRestoreNewsArticleRegion(void);
#endif
