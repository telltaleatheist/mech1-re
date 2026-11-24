#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/campaign_continuations.h"
#include "mechwarrior/campaign_options.h"
#include "mechwarrior/ega_campaign.h"
#include <stdlib.h>
enum { OPTIONS_SOUND_X=0x0068,OPTIONS_SOUND_Y=0x006E,OPTIONS_DETAIL_X=0x0070,OPTIONS_DETAIL_Y=0x0078,
    AUDIO_OPTION_LABEL_COUNT=2,TERRAIN_DETAIL_LABEL_COUNT=3 };

/* MW_MAIN081B:6CFA..6D4D;6E6A PUSH/JMP re-enters the same redraw.
 * The return value transports the native JMP, not an AX source result. */
uint16_t MwEnterOptionsMenuContinuation(uint16_t entryOffset)
{
    if (entryOffset==CAMPAIGN_CONTINUE_OPTIONS_REDRAW) goto publishMenu;
    if (entryOffset!=CAMPAIGN_CONTINUE_OPTIONS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: entryOffset!=CAMPAIGN_CONTINUE_OPTIONS");
    campaignUi.hubSelectedRecordIndex=OPTIONS_HUB_ROW;
    EgaSelectFont(TEXT_PANEL_MONOSPACE_FONT);
    EgaClearDrawPage(Campaign_Pal_EGA_Magenta);
    MwDrawIndexedPicture(0,0,(uint16_t)(campaignCore.currentPlanetLandscapeIndex+OPTIONS_LANDSCAPE_PICTURE_BASE));
publishMenu:
    EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
    MwUpdateAudioOptionLabel();
    MwUpdateTerrainDetailOptionLabel();
    MwDrawDescriptorTextPanel(&campaignOptions.optionsPanel);
    MwSelectUiRecordTable(campaignUi.optionsMenuSelection,&campaignOptions.optionsMenu);
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN081B:6FDE..6FF9. The NUL is copied; no clearing of the old tail. */
void MwUpdateAudioOptionLabel(void)
{
    if (campaignMission.audioOptionLabelIndex>=AUDIO_OPTION_LABEL_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: audioOptionLabelIndex>=AUDIO_OPTION_LABEL_COUNT"); /* Unrepresented out-of-table native alias. */
    uint16_t character=0;
    do
    {
        campaignOptions.optionsLabels[OPTION_SOUND].text[OPTIONS_SOUND_LABEL_COLUMN+character]=campaignOptions.audioOptionLabels[campaignMission.audioOptionLabelIndex][character];
    } while (campaignOptions.audioOptionLabels[campaignMission.audioOptionLabelIndex][character++]!='\0');
}

/* MW_MAIN081B:702B..7046. BYTE detail times5, including trailing space/NUL. */
void MwUpdateTerrainDetailOptionLabel(void)
{
    if (campaignMission.campaignTerrainDetailLevel>=TERRAIN_DETAIL_LABEL_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: campaignTerrainDetailLevel>=TERRAIN_DETAIL_LABEL_COUNT"); /* Unrepresented out-of-table native alias. */
    uint16_t character=0;
    do
    {
        campaignOptions.optionsLabels[OPTION_DETAIL].text[OPTIONS_DETAIL_LABEL_COLUMN+character]=campaignOptions.terrainDetailLabels[campaignMission.campaignTerrainDetailLevel][character];
    } while (campaignOptions.terrainDetailLabels[campaignMission.campaignTerrainDetailLevel][character++]!='\0');
}

/* MW_MAIN081B:6FB9..6FDD. Ordinary callback; the flag toggles without
 * an immediate music start/stop call. Opaque ordinal0F uses current font. */
void MwToggleAudioOption(void)
{
    campaignUi.optionsMenuSelection=OPTION_SOUND;
    campaignMission.audioOptionLabelIndex^=1;
    MwUpdateAudioOptionLabel();
    MwText text=MwArrayText(campaignOptions.optionsLabels[OPTION_SOUND].text);
    uint16_t y=OPTIONS_SOUND_Y;
    EgaDrawString(OPTIONS_SOUND_X,&y,(Campaign_Pal_EGA_DarkGrey<<EGA_BYTE_BITS)|Campaign_Pal_EGA_Yellow,text);
}

/* MW_MAIN081B:6FFA..702A. Native JL compares the incremented BYTE signed. */
void MwCycleTerrainDetailOption(void)
{
    campaignUi.optionsMenuSelection=OPTION_DETAIL;
    ++campaignMission.campaignTerrainDetailLevel;
    if ((int8_t)campaignMission.campaignTerrainDetailLevel>=TERRAIN_DETAIL_LABEL_COUNT) campaignMission.campaignTerrainDetailLevel=0;
    MwUpdateTerrainDetailOptionLabel();
    MwText text=MwArrayText(campaignOptions.optionsLabels[OPTION_DETAIL].text);
    uint16_t y=OPTIONS_DETAIL_Y;
    EgaDrawString(OPTIONS_DETAIL_X,&y,(Campaign_Pal_EGA_DarkGrey<<EGA_BYTE_BITS)|Campaign_Pal_EGA_Yellow,text);
}

/* MW_MAIN081B:7047..707E. Mutate the SAME heading and YES-record owner
 * used by restart707F; the NO record rejoins6D23 without rebuilding scenery. */
uint16_t MwConfirmQuitCampaignContinuation(void)
{
    campaignUi.optionsMenuSelection=OPTION_QUIT;
    EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
    campaignOptions.confirmationPanel.lines[0]=&campaignOptions.confirmationLabels[0].display;
    campaignOptions.confirmationMenu.records[0].activateCodeOffset=CAMPAIGN_CONTINUE_QUIT_FROM_OPTIONS;
    MwDrawDescriptorTextPanel(&campaignOptions.confirmationPanel);
    MwSelectUiRecordTable(0,&campaignOptions.confirmationMenu);
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN081B:707F..70B6, separate native callback/entry, not a new helper. */
uint16_t MwConfirmRestartCampaignContinuation(void)
{
    campaignUi.optionsMenuSelection=OPTION_RESTART;
    EgaCopyDrawPageToDisplay(0,0,EGA_COLUMNS,EGA_ROWS);
    campaignOptions.confirmationPanel.lines[0]=&campaignOptions.confirmationLabels[1].display;
    campaignOptions.confirmationMenu.records[0].activateCodeOffset=CAMPAIGN_CONTINUE_RESTART_FROM_OPTIONS;
    MwDrawDescriptorTextPanel(&campaignOptions.confirmationPanel);
    MwSelectUiRecordTable(0,&campaignOptions.confirmationMenu);
    return CAMPAIGN_CONTINUE_UI;
}

/* MW_MAIN081B:6D4E..6D51. POP/JMP cleanup; do not substitute683D. */
uint16_t MwQuitFromOptionsContinuation(void)
{
    return CAMPAIGN_CONTINUE_CLEANUP;
}

/* MW_MAIN081B:70B7..70C1. PUSH balances6855's inherited frame. */
uint16_t MwRestartFromOptionsContinuation(void)
{
    campaignUi.hubSelectedRecordIndex=0;
    return CAMPAIGN_CONTINUE_RESTART;
}
