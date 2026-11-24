#ifndef MECHWARRIOR_CAMPAIGN_UI_H
#define MECHWARRIOR_CAMPAIGN_UI_H
#include "mechwarrior/ega_campaign.h"
#include "mechwarrior/campaign.h"
#include "mechwarrior/campaign_continuations.h"
#include "mechwarrior/native_address.h"

enum { CAMPAIGN_UI_RECORD_BYTES = 0x000E, HUB_UI_RECORD_COUNT = 0x0006,
    CONTRACT_HUB_UI_RECORD_COUNT = 0x0003, UI_HIGHLIGHT_WIDTH = 0x0022,
    UI_HIGHLIGHT_HEIGHT = 0x0023, NOTIFICATION_TEXT_START = 0x5EDB,
    NOTIFICATION_TEXT_END = 0x6067 };
enum { UI_SELECTED_TEXT_COLOUR = Campaign_Pal_EGA_Yellow, UI_ACTIVATION_FLASH_COUNT = 2,UI_NO_NEIGHBOUR=0xFF,
    UI_FLASH_FIRST_WAIT_TICKS = 2 };
enum { PANEL_HORIZONTAL_TILE_PIXELS = 0x0040, PANEL_VERTICAL_TILE_PIXELS = 0x0020,
    PANEL_BOTTOM_EDGE_INSET = 4, PANEL_RIGHT_EDGE_INSET = 6,
    PANEL_BOTTOM_CORNER_INSET = 0x000C, PANEL_RIGHT_CORNER_INSET = 0x000F,
    PANEL_TOP_LEFT = 0x0012, PANEL_TOP_RIGHT, PANEL_BOTTOM_LEFT, PANEL_BOTTOM_RIGHT,
    PANEL_TOP_TILE, PANEL_BOTTOM_TILE, PANEL_LEFT_TILE, PANEL_RIGHT_TILE,
    PANEL_ALTERNATE_PICTURE_BASE = 0x0012 };
void MwDrawPicturePanelBorder(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void MwDrawStoredPicturePanelBorder(void);
void MwDrawAlternatePicturePanelBorder(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

enum { TEXT_PANEL_MONOSPACE_FONT = 3, TEXT_PANEL_COMPACT_FONT = 0,
    TEXT_PANEL_FOREGROUND = Campaign_Pal_EGA_LightGrey, TEXT_PANEL_BACKGROUND = Campaign_Pal_EGA_DarkGrey,
    TEXT_PANEL_CAPTURE_MARGIN = 8, TEXT_PANEL_MODE_DISCARD_RETURN = 1,
    TEXT_PANEL_MODE_LEAVE_VISIBLE = 0x0063 };
/* Register/stack transport, NOT a newly discovered native source return struct.
 * discardReturnWord means the100C RET has skipped the entry's return word.
 * A caller must honor its own native continuation/frame, never ignore this. */
typedef struct MwTextPanelReturn
{
    const uint8_t *textEnd; /* The NUL the text ended on. */
    uint16_t textOriginX, textOriginY;
    bool discardReturnWord;
} MwTextPanelReturn;
/* TEXT_PANEL_FOREGROUND/BACKGROUND select the panel colours. */
MwTextPanelReturn MwShowMonospaceTextPanel(uint16_t insetX, uint16_t insetY,
    uint16_t columns, uint16_t rows, MwText text);
MwTextPanelReturn MwShowCompactTextPanel(uint16_t insetX, uint16_t insetY,
    uint16_t columns, uint16_t rows, MwText text);
MwTextPanelReturn MwCenterAndShowTextPanel(MwText text);
void MwRestoreSavedTextPanelRegion(void);

/* Coordinates and text are ordinary C objects. The suffixes identify the
 * native fields; host pointers are not serialized 16-bit near addresses. */
typedef struct MwPanelText
{
    uint16_t x, y;
    uint8_t colour;
    char *text;
} MwPanelText;

typedef struct MwUiRecord
{
    uint8_t enabled, textRecord;
    uint16_t x, y, activateCodeOffset;
    uint16_t pictureIndex;
    char *text;
    uint8_t *textColour;
    uint8_t upRecordIndex, downRecordIndex;
    uint8_t leftRecordIndex, rightRecordIndex;
} MwUiRecord;

typedef struct MwUiMenu
{
    MwUiRecord *records;
    uint16_t recordCount;
    /* Native0960 reads this BYTE before089B rejects an FF navigation link. */
    const uint8_t *sentinelEnabled;
    const uint16_t *sentinelEnabledWord;
    uint8_t sentinelEnabledShift;
} MwUiMenu;
void MwShowPendingNotifications(void);
/* Original CALL AX/BX transport. Bind native offsets to actual converted
 * callees. Unbound nonzero offsets fail; they are never success stubs. */
extern void (*campaignNearCallback)(uint16_t codeOffset);
/* Return transports the native shared exit JMP, not an AX/native RET result. */
uint16_t MwCampaignUiEventLoop(void);
/* Shared1909 entry or its1A0D post-event label. Host JMP transport only. */
uint16_t MwEnterCampaignPlanetHub(uint16_t entryOffset);
/* Campaign state owned by campaign_ui.h (plan, slice B2). */
struct CampaignUiState
{
    /* Original UI tables occupy1083..1154 without an overlapping raw DS image.
     * Authored records/text still require native-image initialization. */
    MwUiRecord planetHubRecords[HUB_UI_RECORD_COUNT];
    MwUiRecord contractHubRecords[CONTRACT_HUB_UI_RECORD_COUNT];
    MwUiRecord remotePlanetHubRecords[HUB_UI_RECORD_COUNT];
    MwUiMenu planetHubMenu, contractHubMenu, remotePlanetHubMenu;
    MwUiMenu *uiMenu;
    uint16_t selectedUiRecordIndex;
    uint16_t uiSelectionChangedCodeOffset, uiIdleCodeOffset;
    uint16_t panelOriginX, panelOriginY, panelWidth, panelHeight;
    uint16_t panelBorderPictureBase;
    uint16_t textPanelInsetX, textPanelInsetY;
    uint16_t textPanelOriginX, textPanelOriginY;
    uint16_t textPanelWidthPixels, textPanelHeightPixels;
    uint16_t textPanelPreviousFont, textPanelFont;
    uint16_t textPanelContinuationMode, textPanelYOverride;
    uint16_t notificationTextInsetX[4], notificationTextInsetY[4];
    uint16_t notificationColumns[4], notificationRows[4];
    uint16_t notificationTextOffsets[4];
    uint8_t notificationTextStorage[NOTIFICATION_TEXT_END - NOTIFICATION_TEXT_START];
    /* FF link probes are read BEFORE sentinel rejection. No invented early guard. */
    uint8_t hubFirstNavigationProbe, hubSecondNavigationProbe, hubThirdNavigationProbe;
    uint16_t companyMenuSelectedRow, barMenuSelectedRow, optionsMenuSelection;
    uint16_t hubSelectedRecordIndex;
};
extern CampaignUiState campaignUi;
/* Suspended6236 CALL return, not a new1090 game variable. */
extern uint16_t campaignEventDispatchReturnContinuation;
uint16_t MwSelectUiRecordTable(uint16_t selectedIndex, MwUiMenu *menu);
uint16_t MwSelectUiRecord(uint16_t selectedIndex);
void MwCaptureAndDrawUiRecord(uint16_t x, uint16_t y, uint16_t pictureIndex, const MwUiRecord *record);
void MwPresentUiRecord(uint16_t x, uint16_t y, const MwUiRecord *record);
MwUiRecord *MwGetSelectedUiRecord(void);
bool MwIsUiRecordDisabled(uint16_t recordIndex);
void MwFlashActivatedUiRecord(const MwUiRecord *record);
#endif
