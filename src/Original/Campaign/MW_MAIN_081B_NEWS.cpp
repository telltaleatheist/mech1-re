#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/campaign_continuations.h"
#include "mechwarrior/campaign_options.h"
#include "mechwarrior/campaign_contracts.h"
#include "mechwarrior/ega_campaign.h"
#include <stdlib.h>

enum { NEWS_COMPANY_ROW=1,NEWS_PREVIOUS_ROW=0,NEWS_NEXT_ROW=1,
    NEWS_ARTICLE_TOP=0x17,NEWS_ARTICLE_HEIGHT=0x9A,NEWS_TEXT_X=0x1D,
    NEWS_TEXT_WIDTH=0x106,NEWS_BACKGROUND_COLOUR=Campaign_Pal_EGA_Black,NEWS_MESSAGE_COLOUR=Campaign_Pal_EGA_Green,
    NEWS_MESSAGE_X=0x0038,NEWS_MESSAGE_Y=0x0061 };

/* MW_MAIN081B:6314..6324. Stored directory references, not new indexes. */
void MwInitializeNewsArticleCursor(void)
{
    if (eventDirectoryIndex>=CAMPAIGN_EVENT_LIST_COUNT || !eventLists[eventDirectoryIndex]) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: eventDirectoryIndex>=CAMPAIGN_EVENT_LIST_COUNT || !eventLists[eventDirectoryIndex]");
    campaignOptions.newsArticleCursor=eventLists[eventDirectoryIndex];
    campaignOptions.newsArticleListStart=campaignOptions.newsArticleCursor;
}

/* MW_MAIN081B:6325..6362. CF true is exhausted. Advance BEFORE inspecting
 * the next candidate; no class3 seen filter and no reset of dispatch count. */
bool MwFindNextNewsArticle(uint16_t *articleId)
{
    campaignEventClass=EVENT_CLASS_REPEATABLE;
    MwCampaignEvent **cursor=campaignOptions.newsArticleCursor;
    while (*cursor)
    {
        const MwCampaignEvent *event=*++cursor;
        if (!event) return true;
        if (event->classCode!=EVENT_CLASS_REPEATABLE) continue;
        currentEventId=event->eventId;
        bool carry;
        uint16_t transfer=MwInterpretCampaignEvent(event,&carry);
        if (transfer) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: transfer"); /* No class3 installation script contains a choice/nonlocal transfer. */
        if (!carry) continue;
        campaignOptions.newsArticleCursor=cursor;
        *articleId=currentEventId;
        return false;
    }
    return true;
}

/* MW_MAIN081B:6363..639A. Compare current/start references BEFORE decrement.
 * Failure leaves current5E6A unchanged, though interpreter side effects remain. */
bool MwFindPreviousNewsArticle(uint16_t *articleId)
{
    campaignEventClass=EVENT_CLASS_REPEATABLE;
    MwCampaignEvent **cursor=campaignOptions.newsArticleCursor;
    while (cursor!=campaignOptions.newsArticleListStart)
    {
        const MwCampaignEvent *event=*--cursor;
        if (!event) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !event"); /* Unrepresented corrupted list reference, not exhaustion. */
        if (event->classCode!=EVENT_CLASS_REPEATABLE) continue;
        currentEventId=event->eventId;
        bool carry;
        uint16_t transfer=MwInterpretCampaignEvent(event,&carry);
        if (transfer) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: transfer"); /* Shipped class3 scripts return normally. */
        if (!carry) continue;
        campaignOptions.newsArticleCursor=cursor;
        *articleId=currentEventId;
        return false;
    }
    return true;
}

/* MW_MAIN081B:2C13..2C76. Entry charges time BEFORE selecting the article
 * directory, so month progression may change which list is browsed. */
uint16_t MwOpenNewsNetContinuation(void)
{
    campaignUi.companyMenuSelectedRow=NEWS_COMPANY_ROW;
    campaignOptions.newsAtStart=0;campaignOptions.newsAtEnd=0;
    MwAdvanceCampaignTimeAndChargeWages(1);
    MwDrawCampaignFramedBackground();
    EgaFlipPages();
    EgaSelectFont(TEXT_PANEL_COMPACT_FONT);
    campaignOptions.newsLastArticleId=UINT16_MAX;
    MwInitializeNewsArticleCursor();
    uint16_t articleId;
    bool exhausted=MwFindNextNewsArticle(&articleId);
    EgaSelectFont(TEXT_PANEL_MONOSPACE_FONT); /* Native preserves CF across this call. */
    if (exhausted)
    {
        MwDrawNewsBoundaryMessage();
        campaignOptions.newsAtEnd=1;campaignOptions.newsAtStart=1;
    }
    MwDrawDescriptorTextPanel(&campaignOptions.newsMenuPanel);
    MwSelectUiRecordTable(NEWS_PREVIOUS_ROW,&campaignOptions.newsMenu);
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN081B:2C77..2CC3. At the opposite boundary, the first click only
 * restores the last article; it does not advance the list in the same click. */
uint16_t MwNextNewsArticleContinuation(void)
{
    if (campaignOptions.newsAtEnd==1) goto selectNext;
    if (campaignOptions.newsAtStart==1)
    {
        MwRestoreNewsArticleRegion();campaignOptions.newsAtStart=0;campaignOptions.newsAtEnd=0;
        goto selectNext;
    }
    uint16_t articleId;
    if (MwFindNextNewsArticle(&articleId) || articleId==campaignOptions.newsLastArticleId)
    {
        MwDrawNewsBoundaryMessage();campaignOptions.newsAtEnd=1;
    }
    else
    {
        campaignOptions.newsLastArticleId=articleId;EgaSelectFont(TEXT_PANEL_MONOSPACE_FONT);
    }
selectNext:
    MwSelectUiRecordTable(NEWS_NEXT_ROW,&campaignOptions.newsMenu);
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN081B:2D16..2D62. Separate previous callback, native flag/write order. */
uint16_t MwPreviousNewsArticleContinuation(void)
{
    if (campaignOptions.newsAtStart==1) goto selectPrevious;
    if (campaignOptions.newsAtEnd==1)
    {
        MwRestoreNewsArticleRegion();campaignOptions.newsAtStart=0;campaignOptions.newsAtEnd=0;
        goto selectPrevious;
    }
    uint16_t articleId;
    if (MwFindPreviousNewsArticle(&articleId) || articleId==campaignOptions.newsLastArticleId)
    {
        MwDrawNewsBoundaryMessage();campaignOptions.newsAtStart=1;
    }
    else
    {
        campaignOptions.newsLastArticleId=articleId;EgaSelectFont(TEXT_PANEL_MONOSPACE_FONT);
    }
selectPrevious:
    MwSelectUiRecordTable(NEWS_PREVIOUS_ROW,&campaignOptions.newsMenu);
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN081B:2CC4..2D01. Save the visible article BEFORE drawing the message. */
void MwDrawNewsBoundaryMessage(void)
{
    EgaCopyDisplayToDrawPage(0,NEWS_ARTICLE_TOP,EGA_COLUMNS,NEWS_ARTICLE_HEIGHT);
    EgaFillDisplayPageRectangle(NEWS_TEXT_X,NEWS_ARTICLE_TOP,NEWS_TEXT_WIDTH,NEWS_ARTICLE_HEIGHT,NEWS_BACKGROUND_COLOUR);
    MwNativeByteView text={(const uint8_t *)campaignOptions.newsBoundaryText,0x2564,sizeof(campaignOptions.newsBoundaryText)};
    EgaDrawTransparentText(NEWS_MESSAGE_X,NEWS_MESSAGE_Y,NEWS_MESSAGE_COLOUR,&text,0x2564);
}
/* MW_MAIN081B:2D02..2D15. Restore, do not regenerate/re-evaluate the article. */
void MwRestoreNewsArticleRegion(void)
{
    EgaCopyDrawPageToDisplay(0,NEWS_ARTICLE_TOP,EGA_COLUMNS,NEWS_ARTICLE_HEIGHT);
}
