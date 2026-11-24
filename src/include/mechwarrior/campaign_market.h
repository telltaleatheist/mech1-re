#ifndef MECHWARRIOR_CAMPAIGN_MARKET_H
#define MECHWARRIOR_CAMPAIGN_MARKET_H
#include "mechwarrior/campaign_service.h"

enum { RESERVE_AMMO_MENU_ROWS=13,RESERVE_AMMO_DONE_ROW=12,RESERVE_AMMO_MACHINE_GUN_FAMILY=5,
    RESERVE_AMMO_MACHINE_GUN_BATCH=10,RESERVE_AMMO_ROW_Y=0x82,RESERVE_AMMO_ROW_HEIGHT=6,
    RESERVE_AMMO_PRICE_X=0x98,RESERVE_AMMO_QUANTITY_X=0xD7,
    RESERVE_AMMO_PREVIEW_WIDTH=0x6C,RESERVE_AMMO_PREVIEW_HEIGHT=0x3E };
typedef struct MwReserveAmmoQuantityRow {MwPanelText display;char text[6];} MwReserveAmmoQuantityRow;
uint16_t MwOpenReserveAmmunitionShop(void);
void MwFormatReserveAmmunitionQuantity(uint16_t family,uint16_t quantity);
void MwDrawReserveAmmunitionPrices(bool selling);
void MwDrawAmmunitionShopCompanyFunds(void);
void MwRefreshReserveAmmunitionSelection(void);
void MwShowReserveAmmunitionCompatibility(uint16_t family);
void MwDrawReserveAmmunitionCompatibleMechs(uint16_t family);
void MwMarkOwnedAmmunitionCompatibleMechs(void);
void MwBuyReserveAmmunition(void);
uint16_t MwSellReserveAmmunition(void);

enum { MECH_SERVICE_ROW_REARM=2, MECH_SERVICE_ROW_SELL=3,
    REARM_DIALOG_X=0x10, REARM_DIALOG_Y=0x8E, REARM_DIALOG_WIDTH=0xB2, REARM_DIALOG_HEIGHT=0x37,
    SALE_DIALOG_X=0x0A, SALE_DIALOG_Y=0x72, SALE_DIALOG_WIDTH=0xC8, SALE_DIALOG_HEIGHT=0x50,
    DEALER_LIST_NAME_COLUMNS=0x0C, DEALER_AMMO_ROW_HEIGHT=6,
    MECH_COMPLEX_ANIMATION_FRAMES=0x17, MECH_COMPLEX_ANIMATION_END=0xFFFF,
    MECH_COMPLEX_ANIMATION_PAUSE_MAXIMUM=0x1E, MECH_COMPLEX_ANIMATION_PAUSE_MINIMUM=6 };
enum { REARM_QUOTE_X=0x6A, REARM_QUOTE_Y=0x96, REARM_FUNDS_X=0x4C, REARM_FUNDS_Y=0xA2,
    SALE_QUOTE_X=0x1A, SALE_QUOTE_Y=0x8A, DEALER_PRICE_X=0x32, DEALER_PRICE_Y=0x84,
    DEALER_FUNDS_X=0x38, DEALER_FUNDS_Y=0x90, MARKET_PRICE_FIELD_DIGITS=8,
    MARKET_MESSAGE_X=0x10, MARKET_MESSAGE_Y=0x10,
    REARM_FULL_MESSAGE_X=0x12, REARM_FULL_MESSAGE_COLUMNS=0x15, REARM_FULL_MESSAGE_ROWS=5,
    DEALER_QUOTA_MESSAGE_COLUMNS=0x20, DEALER_QUOTA_MESSAGE_ROWS=6,
    PURCHASE_FUNDS_MESSAGE_COLUMNS=0x1B, PURCHASE_FUNDS_MESSAGE_ROWS=5,
    OWNERSHIP_LIMIT_MESSAGE_X=0x0C, OWNERSHIP_LIMIT_MESSAGE_Y=8,
    OWNERSHIP_LIMIT_MESSAGE_COLUMNS=0x1E, OWNERSHIP_LIMIT_MESSAGE_ROWS=4,
    DEALER_EMPTY_MESSAGE_Y=0x0C, DEALER_EMPTY_MESSAGE_COLUMNS=0x1C, DEALER_EMPTY_MESSAGE_ROWS=5,
    DEALER_COMPONENT_DONE_Y=0xB8, DEALER_COMPONENT_FIXED_LINES=0x0D,
    MECH_COMPLEX_ANIMATION_X=0x60, MECH_COMPLEX_ANIMATION_Y=0x40,
    MECH_COMPLEX_HUB_ROW=1, MECH_COMPLEX_FIRST_VISIT=0xFFFF,
    MECH_COMPLEX_REVIEW_CONTEXT=0, MECH_COMPLEX_DEALER_CONTEXT=2, MECH_COMPLEX_EVENT_CLASS=4,
    REARM_QUOTE_TIME_UNITS=1, SALE_QUOTE_TIME_UNITS=2, DEALER_QUOTA_RESTRICTED_TIER=2 };
typedef struct MwMarketUnitListRow
{
    MwPanelText display;
    char text[DEALER_LIST_NAME_COLUMNS+1];
} MwMarketUnitListRow;
/* The existing repair menu's FF probe is inside this very message. */
#define marketCapacityNavigationProbe (*(uint8_t *)&campaignMarket.ownedMechCapacityText[3])
/* Campaign state owned by campaign_market.h (plan, slice B2). */
struct CampaignMarketState
{
    /* The after-action and settlement directories (1090:1491, 166A, 1786), whose
     * entries the C references above stand for, are checked at import: the
     * import table's check pieces (src/Import/pieces.txt) refuse an installation
     * whose directories differ. */
    /* Native reserve-ammunition shop state and shared caption/menu owners. */
    uint16_t reserveAmmoShopSelectedRow, reserveAmmoPreviewSelection;
    uint8_t reserveAmmoSelling, reserveAmmoMechColours[11];
    char reserveAmmoBuyLabel[5], reserveAmmoSellLabel[5];
    MwServiceCaption reserveAmmoShopHeading, reserveAmmoFamilyLabels[6], reserveAmmoDoneLabel;
    MwReserveAmmoQuantityRow reserveAmmoQuantityRows[6];
    MwPanelDescriptor reserveAmmoShopPanel, reserveAmmoCompatibilityPanels[6];
    MwServiceCaption reserveAmmoCompatibilityHeadings[6];
    uint16_t reserveAmmoCompatibleMechs[6][6];
    MwUiRecord reserveAmmoShopRecords[RESERVE_AMMO_MENU_ROWS];
    MwUiMenu reserveAmmoShopMenu;
    /* Rearm/dealer objects are ordinary C data, independent of their disk encoding. */
    MwPanelDescriptor rearmConfirmationPanel, saleConfirmationPanel;
    MwPanelDescriptor marketUnitListPanel, marketMechStatusPanel, marketDropShipStatusPanel;
    MwPanelDescriptor marketComponentPanel, mechComplexPanel;
    MwServiceCaption rearmConfirmationLabels[3], saleConfirmationLabels[3];
    MwServiceCaption marketListHeading, marketMechStatusLabels[4], marketDropShipStatusLabels[3];
    MwServiceCaption mechComplexLabels[5], marketComponentTitle;
    MwMarketUnitListRow marketUnitListRows[0x0D];
    MwComponentConditionTextRow marketSystemConditionRows[0x0A], marketWeaponConditionRows[0x0A];
    MwServiceCaption marketComponentDone;
    MwServiceCaption marketWeaponHeading;
    MwServiceCaption positionedText;
    MwBodyConditionText marketBodyCondition;
    MwUiMenu rearmConfirmationMenu, saleConfirmationMenu, marketUnitListMenu;
    MwUiMenu marketMechMenu, marketDropShipMenu, marketComponentMenu, mechComplexMenu;
    char ammoBayFullText[0x12], noMarketMechsText[0x2E];
    char dealerPurchaseLimitText[0x33], purchaseInsufficientFundsText[0x17], ownedMechCapacityText[0x37];
    char marketListDoneText[DEALER_LIST_NAME_COLUMNS];
    uint16_t selectedMarketUnitSlot, selectedMarketUnitByteOffset;
    uint16_t selectedMarketAmmoFamilies[CAMPAIGN_AMMO_GROUPS], selectedMarketAmmoQuantities[CAMPAIGN_AMMO_GROUPS];
    uint16_t marketSystemRowIndex, purchaseOwnedSlotByteOffset;
    uint16_t mechComplexAnimationFrames[MECH_COMPLEX_ANIMATION_FRAMES];
};
extern CampaignMarketState campaignMarket;
/* First-visit6236 is a native event call with AX4, not a no-argument callback. */
uint16_t MwQuoteSelectedMechRearm(void);
uint16_t MwRearmSelectedMech(void);
uint16_t MwCancelSelectedMechRearm(void);
void MwRestoreMechServiceDialogRegion(void);
void MwCalculateSelectedUnitNetPrice(void);
uint16_t MwBeginOwnedUnitSaleCallback(void);
uint16_t MwSaleContinuation(void);
uint16_t MwCancelOwnedUnitSale(void);
uint16_t MwOpenMarketMechList(void);
uint16_t MwSelectMarketUnitListRowCallback(void);
void MwInitializeMechServiceIdle(void);
/* entryOffset is shared-entry/JMP transport, not a native source argument. */
uint16_t MwDrawMarketMechDetailContinuation(uint16_t entryOffset);
uint16_t MwOpenMarketMechComponentInspectionCallback(void);
uint16_t MwPurchaseSelectedMarketUnit(uint16_t entryOffset);
/*2DE2 resumes after6236; entryOffset is host transport, not a native argument. */
uint16_t MwEnterMechServiceMenuContinuation(uint16_t entryOffset);
void MwAnimateMechServiceIdle(void);

#endif
