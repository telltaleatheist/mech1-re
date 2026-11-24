#include "mechwarrior/combat_orders.h"
#include "mechwarrior/story.h"
#include "mechwarrior/native_address.h"
#include "mechwarrior/ui_layout.h"
#include <string.h>

CombatOrderState combatOrders;
static char orderPanelText[ORDER_PANEL_TEXT_END-ORDER_PANEL_TEXT_START];

/* Actual fresh image/BSS boundary.0040 itself does not blanket-clear state. */
/* A layout piece (data/ui, src/Ui) copied into `view` at its native offset,
 * so the reads below keep the exe's addresses. */
static bool CopyLayoutPiece(uint8_t *view,size_t viewBytes,const char *id,uint16_t offset)
{
    const size_t size=MwUiPieceSize(id);
    return size && offset<=viewBytes && size<=viewBytes-offset && MwUiCopyNative(id,view+offset,size);
}
bool BtechImportOrderInitialData(const uint8_t *data,size_t bytes)
{
    /* The layout's pieces, at their native offsets (304E:008C..032E), and the
     * order panel's words (story/words.json, orders.word.*) at theirs
     * (304E:010C..02B6), so the reads below keep the exe's addresses. */
    uint8_t ui[0x032E],words[ORDER_PANEL_TEXT_END];
    memset(ui,0,sizeof(ui));memset(words,0,sizeof(words));
    if (!MwWordsCopyInto("orders.word.",words,sizeof(words))) return false;
    if (!CopyLayoutPiece(ui,sizeof(ui),"orders.start.lance_panel_text_x",0x008C)
        || !CopyLayoutPiece(ui,sizeof(ui),"orders.selection_first_rows",0x0092)
        || !CopyLayoutPiece(ui,sizeof(ui),"orders.selection_last_rows",0x0096)
        || !CopyLayoutPiece(ui,sizeof(ui),"orders.navigation_panel_deltas",0x009A)
        || !CopyLayoutPiece(ui,sizeof(ui),"orders.navigation_row_deltas",0x00B0)
        || !CopyLayoutPiece(ui,sizeof(ui),"orders.engine_condition_colours",0x030E)
        || !CopyLayoutPiece(ui,sizeof(ui),"orders.system_condition_colours",0x031E)
        || !CopyLayoutPiece(ui,sizeof(ui),"orders.word_refs.orders",0x02B6)
        || !CopyLayoutPiece(ui,sizeof(ui),"orders.display_indices",0x02C4)
        || !CopyLayoutPiece(ui,sizeof(ui),"orders.word_refs.tactical",0x02DA)
        || !CopyLayoutPiece(ui,sizeof(ui),"orders.word_refs.damage",0x02EC)
        || !CopyLayoutPiece(ui,sizeof(ui),"orders.word_refs.weapons",0x02FA)) return false;
    if (!data || bytes<0x4F5A || MwReadNativeWord(data,0x4F2E) || MwReadNativeWord(data,0x4F30)) return false;
    memcpy(orderPanelText,words+ORDER_PANEL_TEXT_START,sizeof(orderPanelText));
    /* Authored string directories are decoded once at executable IO. Runtime
     * panels use the references below, never native-address accessors. */
    for (uint16_t directory=0;directory<4;++directory)
    {
        const uint16_t bases[]={0x02B6,0x02DA,0x02EC,0x02FA};
        const uint16_t counts[]={ORDER_LABELS,ORDER_TACTICAL_LABELS,ORDER_DAMAGE_LABELS,COMBAT_WEAPON_KINDS};
        const char **destinations[]={combatOrders.orderLabels,combatOrders.tacticalOrderLabels,combatOrders.orderDamageLabels,combatOrders.orderEquipmentWeaponLabels};
        for (uint16_t entry=0;entry<counts[directory];++entry)
        {
            uint16_t offset=MwReadNativeWord(ui,bases[directory]+entry*2);
            if (offset<ORDER_PANEL_TEXT_START || offset>=ORDER_PANEL_TEXT_END || !memchr(words+offset,0,ORDER_PANEL_TEXT_END-offset)) return false;
            destinations[directory][entry]=orderPanelText+offset-ORDER_PANEL_TEXT_START;
        }
    }
    combatOrders.orderWeaponHeading=orderPanelText+0x026C-ORDER_PANEL_TEXT_START;
    combatOrders.orderOfText=orderPanelText+0x027A-ORDER_PANEL_TEXT_START;
    combatOrders.orderColon=orderPanelText+0x027D-ORDER_PANEL_TEXT_START;
    combatOrders.orderSelectionHeading=orderPanelText+0x027F-ORDER_PANEL_TEXT_START;
    combatOrders.orderLocationText=orderPanelText+0x028D-ORDER_PANEL_TEXT_START;
    combatOrders.orderBaseText=orderPanelText+0x029A-ORDER_PANEL_TEXT_START;
    combatOrders.orderDamageText=orderPanelText+0x02A7-ORDER_PANEL_TEXT_START;
    combatOrders.orderWeaponsText=orderPanelText+0x02AE -ORDER_PANEL_TEXT_START;
    for (uint16_t code=0;code<ORDER_DISPLAY_CODES;++code) combatOrders.orderDisplayIndices[code]=MwReadNativeWord(ui,0x02C4+code*2);
    for (uint16_t scheme=0;scheme<2;++scheme)
        for (uint16_t condition=0;condition<4;++condition)
        {
            combatOrders.orderEngineConditionColours[scheme][condition]=MwReadNativeWord(ui,0x030E +scheme*8+condition*2);
            combatOrders.orderSystemConditionColours[scheme][condition]=MwReadNativeWord(ui,0x031E +scheme*8+condition*2);
        }
    /* 4F32 and 4F46 lie past BTECH's file data: zero where the exe reloaded them. */
    memset(combatOrders.mountedWeaponCountsByKind,0,sizeof(combatOrders.mountedWeaponCountsByKind));
    memset(combatOrders.operationalWeaponCountsByKind,0,sizeof(combatOrders.operationalWeaponCountsByKind));
    combatOrders.orderPanelCaptureBuffer=BtechResourceAllocation{};
    for (uint16_t selection=0;selection<ORDER_SELECTION_CLASSES;++selection)
    {
        combatOrders.orderSelectionFirstRows[selection]=(int16_t)MwReadNativeWord(ui,0x0092+selection*2);
        combatOrders.orderSelectionLastRows[selection]=(int16_t)MwReadNativeWord(ui,0x0096+selection*2);
    }
    for (uint16_t direction=0;direction<ORDER_NAVIGATION_CODES;++direction)
    {
        combatOrders.orderNavigationPanelDeltas[direction]=(int16_t)MwReadNativeWord(ui,0x009A+direction*2);
        combatOrders.orderNavigationRowDeltas[direction]=(int16_t)MwReadNativeWord(ui,0x00B0+direction*2);
    }
    for (uint16_t scale=0;scale<ORDER_MAP_SCALE_COUNT;++scale) combatOrders.orderMapScaleShifts[scale]=data[0x0088+scale];
    for (uint16_t style=0;style<ORDER_MAP_MARKER_STYLES;++style)
        for (uint16_t x=0;x<ORDER_MAP_MARKER_WIDTH;++x)
            for (uint16_t y=0;y<ORDER_MAP_MARKER_WIDTH;++y) combatOrders.orderMapMarkerMasks[style][x][y]=data[0x00E8+style*9+x*3+y];
    for (uint16_t corner=0;corner<2;++corner)
    {
        uint32_t *position[]={&combatOrders.combatWorldBoundaryCorners[corner].positionA,&combatOrders.combatWorldBoundaryCorners[corner].positionB,&combatOrders.combatWorldBoundaryCorners[corner].positionC};
        for (uint16_t axis=0;axis<3;++axis) *position[axis]=MwReadNativeWord(data,0x0336+corner*12+axis*4)|((uint32_t)MwReadNativeWord(data,0x0338+corner*12+axis*4)<<16);
    }
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {combatOrders.unitOrderMapScreenX[slot]=0; /*4F08*/combatOrders.unitOrderMapScreenY[slot]=0; /*4F18*/}
    for (uint16_t panel=0;panel<3;++panel) combatOrders.lanceOrderPanelTextX[panel]=MwReadNativeWord(ui,0x008C+panel*2); /* orders.json "start" */
    combatOrders.orderScreenFullRedrawPending=data[0x4EEC];
    combatOrders.previousOrderHighlightPanel=0; /*4EEE*/
    combatOrders.previousOrderHighlightRow=0; /*4EF4*/
    combatOrders.orderMapPositionMask=0; /* 4EF0, past BTECH's file data: written before it's read, zero where the exe reloaded it. */
    for (uint16_t panel=0;panel<ORDER_WINGMATE_PANELS;++panel)
    {
        if (MwReadNativeWord(data,0x4EFC+panel*2)) return false; /* Fresh EXEC has no live near control references. */
        combatOrders.lanceOrderControls[panel]=NULL;
    }
    combatOrders.orderMapScaleIndex=0; /*4EF6*/
    combatOrders.selectedLanceOrderPanel=0; /*4EF8*/combatOrders.selectedOrderPanelRow=0; /*4EFA*/
    combatOrders.orderPanelMode=0; /*4F02*/combatOrders.orderActionSelection=0; /*4F04*/
    combatOrders.pendingOrderCommand=0; /*4F06*/
    combatOrders.orderDestinationRepeatCountdown=0; /*4F28*/
    combatOrders.previousOrderMapScaleIndex=0; /*4F2A*/combatOrders.previousOrderNavigationKey=0; /*4F2C*/
    return true;
}
