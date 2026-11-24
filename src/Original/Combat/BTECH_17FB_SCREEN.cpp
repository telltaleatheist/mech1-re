#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_orders.h"
#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_input.h"
#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_polygons.h"
#include <stdlib.h>

enum { ORDER_REDUCED_PANEL_X=0x6D,ORDER_REDUCED_PANEL_Y=0x7A,
    ORDER_REDUCED_PANEL_WIDTH=0x67,ORDER_REDUCED_PANEL_HEIGHT=0x4A,
    ORDER_REDUCED_BORDER_LEFT=0x70,ORDER_REDUCED_BORDER_RIGHT=0xD2,
    ORDER_REDUCED_BORDER_TOP=0x78,ORDER_REDUCED_BORDER_BOTTOM=0xC4,
    ORDER_PANEL_BACKGROUND=Combat_Pal_EGA_LightGrey,ORDER_PANEL_SHADOW=Combat_Pal_EGA_DarkGrey,ORDER_PANEL_HIGHLIGHT=Combat_Pal_EGA_White,
    ORDER_DESTINATION_BLINK_MASK=2,ORDER_DESTINATION_MARKER_INSET=2,ORDER_DESTINATION_MARKER_SIZE=5 };

/*17FB:00DF. Actual FAR command-screen refresh. The full-map capture excludes
 * markers/tracking/panels; cached frames restore it before drawing overlays.
 * An editor screen change returns without presenting another screen here. */
void BtechRefreshLanceOrderScreen(void)
{
    uint16_t redrawRequired=0;
    combatVideo.drawPageSelector=combatVideo.backPageSelector;
    if (combatOrders.orderScreenFullRedrawPending)
    {
        combatCockpit.cockpitFullRedrawPending=0;
        combatVideo.screenPolygonNeedsClip=false;
        BtechLoadScreenImage("map.scr",NULL);
        if (combatImport.reducedMemoryMode)
        {
            combatVideo.drawLineColour=ORDER_PANEL_BACKGROUND;combatVideo.drawFillColour=ORDER_PANEL_BACKGROUND;
            combatVideo.drawPolygonFilled=true;
            BtechDrawStyledRectangle(ORDER_REDUCED_PANEL_X,ORDER_REDUCED_PANEL_Y,ORDER_REDUCED_PANEL_WIDTH,ORDER_REDUCED_PANEL_HEIGHT);
            combatVideo.drawFillColour=ORDER_PANEL_SHADOW;
            BtechDrawClippedLine(ORDER_REDUCED_BORDER_LEFT,ORDER_REDUCED_BORDER_TOP,ORDER_REDUCED_BORDER_RIGHT,ORDER_REDUCED_BORDER_TOP);
            BtechDrawClippedLine(ORDER_REDUCED_BORDER_LEFT,ORDER_REDUCED_BORDER_BOTTOM,ORDER_REDUCED_BORDER_RIGHT,ORDER_REDUCED_BORDER_BOTTOM);
            combatVideo.drawFillColour=ORDER_PANEL_HIGHLIGHT;
            BtechDrawClippedLine(ORDER_REDUCED_BORDER_LEFT,ORDER_REDUCED_BORDER_TOP+1,ORDER_REDUCED_BORDER_RIGHT,ORDER_REDUCED_BORDER_TOP+1);
        }
        combatOrders.previousOrderHighlightPanel=ORDER_NO_HIGHLIGHT;
        combatOrders.previousOrderHighlightRow=ORDER_NO_HIGHLIGHT;
        BtechUpdateLanceOrderHighlight();
        combatOrders.orderMapPositionMask=BtechBuildOrderMapPositionMask();
    }
    if (!orderMapCameraObject || !orderMapCameraContext || combatOrders.orderMapScaleIndex>=ORDER_MAP_SCALE_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !orderMapCameraObject || !orderMapCameraContext || orderMapScaleIndex>=ORDER_MAP_SCALE_COUNT");
    if (combatOrders.orderMapScaleIndex==ORDER_MAP_FULL_SCALE)
    {
        if (combatOrders.previousOrderMapScaleIndex!=ORDER_MAP_FULL_SCALE || combatOrders.orderScreenFullRedrawPending)
        {
            orderMapCameraObject->object.prefix.position.positionB=0;
            orderMapCameraObject->object.prefix.position.positionA=0;
            redrawRequired=1;
        }
    }
    else
    {
        BtechPosition3D *camera=&orderMapCameraObject->object.prefix.position;
        if (!playerUnitBinding || !playerUnitBinding->positionedLeftLeg) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerUnitBinding || !playerUnitBinding->positionedLeftLeg");
        camera->positionA=playerUnitBinding->positionedLeftLeg->object.prefix.position.positionA&combatOrders.orderMapPositionMask;
        /* Native reloads camera/binding/object after the X write. */
        camera=&orderMapCameraObject->object.prefix.position;
        if (!playerUnitBinding || !playerUnitBinding->positionedLeftLeg) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerUnitBinding || !playerUnitBinding->positionedLeftLeg");
        camera->positionB=playerUnitBinding->positionedLeftLeg->object.prefix.position.positionB&combatOrders.orderMapPositionMask;
        redrawRequired=1;
    }
    orderMapCameraObject->object.prefix.position.positionC=
        BtechShiftDwordLeftPair(1,combatOrders.orderMapScaleShifts[combatOrders.orderMapScaleIndex]);
    combatVideo.drawPolygonFilled=true;combatVideo.drawPageSelector=combatVideo.backPageSelector;
    if (redrawRequired)
    {
        BtechRenderObjectListVeneer(orderMapCameraContext,selectedWorldObjects,(int16_t)selectedObjectCount);
        if (combatOrders.orderMapScaleIndex==ORDER_MAP_FULL_SCALE)
        {
            combatVideo.copySourcePageSelector=combatVideo.backPageSelector;
            const BtechCameraContext *viewport=orderMapCameraContext;
            BtechCaptureScreenRectangle(combatOrders.orderPanelCaptureBuffer,(uint16_t)viewport->clipMinimumA,(uint16_t)viewport->clipMinimumB,
                (uint16_t)(viewport->clipMaximumA-viewport->clipMinimumA),(uint16_t)(viewport->clipMaximumB-viewport->clipMinimumB+1));
        }
    }
    else
    {
        const BtechCameraContext *viewport=orderMapCameraContext;
        BtechRestoreScreenRectangle(combatOrders.orderPanelCaptureBuffer,(uint16_t)viewport->clipMinimumA,(uint16_t)viewport->clipMinimumB,
            (uint16_t)(viewport->clipMaximumA-viewport->clipMinimumA),(uint16_t)(viewport->clipMaximumB-viewport->clipMinimumB+1));
    }
    for (uint16_t panel=0;panel<ORDER_WINGMATE_PANELS;++panel)
        combatOrders.lanceOrderControls[panel]=unitBindings[panel+1].mech?&unitControls[panel+1]:NULL;
    combatVideo.screenPolygonNeedsClip=false;
    BtechDrawLanceMapUnitMarkers();
    if (combatOrders.lanceOrderControls[0])
    {
        if (combatOrders.selectedLanceOrderPanel>=ORDER_WINGMATE_PANELS || !combatOrders.lanceOrderControls[combatOrders.selectedLanceOrderPanel]) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: selectedLanceOrderPanel>=ORDER_WINGMATE_PANELS || !lanceOrderControls[selectedLanceOrderPanel]");
        const BtechUnitControlState *control=combatOrders.lanceOrderControls[combatOrders.selectedLanceOrderPanel];
        uint16_t unitSlot=control->unitSlot;
        if (control->trackedPosition[2]>=0 && control->conditionScoreOrExitCode>COMBAT_CONDITION_SCORE_MINIMUM)
        {
            /* A typed read-only argument value replaces native control+20,
             * not a cast between unrelated C struct and int32 array types. */
            BtechPosition3D trackedPosition={(uint32_t)control->trackedPosition[0],(uint32_t)control->trackedPosition[1],(uint32_t)control->trackedPosition[2]};
            int16_t trackedX,trackedY;
            if (BtechProjectPositionToOrderMap(&trackedPosition,&trackedX,&trackedY))
            {
                if (combatImport.hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES || combatImport.combatLandscapeIndex>=COMBAT_LANDSCAPES || unitSlot>=COMBAT_MECH_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES || combatLandscapeIndex>=COMBAT_LANDSCAPES || unitSlot>=COMBAT_MECH_SLOTS");
                combatVideo.drawFillColour=(uint8_t)combatVideo.landscapeCameraColourRows[combatImport.hudColourSchemeIndex][combatImport.combatLandscapeIndex].orderTrackingLineColour;
                BtechDrawClippedLine(combatOrders.unitOrderMapScreenX[unitSlot],combatOrders.unitOrderMapScreenY[unitSlot],trackedX,trackedY);
                if ((combatOrders.orderPanelMode==ORDER_PANEL_DESTINATION || combatOrders.orderPanelMode==ORDER_PANEL_TARGET) && (combatStartup.combatTickCounter&ORDER_DESTINATION_BLINK_MASK))
                {
                    combatVideo.drawFillColour=Combat_Pal_EGA_Black;combatVideo.drawPolygonFilled=false;
                    BtechDrawStyledRectangle((uint16_t)(trackedX-ORDER_DESTINATION_MARKER_INSET),(uint16_t)(trackedY-ORDER_DESTINATION_MARKER_INSET),ORDER_DESTINATION_MARKER_SIZE,ORDER_DESTINATION_MARKER_SIZE);
                }
            }
        }
    }
    if (combatOrders.orderMapScaleIndex==ORDER_MAP_FULL_SCALE) BtechDrawLanceMapRetreatBoundaries();
    combatVideo.screenPolygonNeedsClip=false;combatVideo.drawPolygonFilled=false;
    combatOrders.previousOrderMapScaleIndex=combatOrders.orderMapScaleIndex;
    BtechEditSelectedUnitOrders();
    if (combatStartup.combatDispatchState!=PLAYER_SCREEN_ORDERS)
    {combatVideo.screenPolygonNeedsClip=true;combatVideo.drawPolygonFilled=true;return;}
    BtechDrawUnitOrderPanels();
    while (BtechGetActiveRuntimeDelayFar())
    {if (!btechHostPollInterrupts) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !btechHostPollInterrupts");btechHostPollInterrupts();}
    BtechSwapScreenPages(false);
    if (combatOrders.orderScreenFullRedrawPending)
    {
        BtechPresentScreenRectangle(0,0,COMBAT_SCREEN_WIDTH,COMBAT_SCREEN_HEIGHT);
        combatVideo.copySourcePageSelector=combatVideo.frontPageSelector;combatVideo.drawPageSelector=combatVideo.backPageSelector;
        BtechCopyScreenRectangle(0,0,COMBAT_SCREEN_WIDTH,COMBAT_SCREEN_HEIGHT);
        combatOrders.orderScreenFullRedrawPending=0;
    }
    else
    {
        const BtechCameraContext *viewport=orderMapCameraContext;
        BtechPresentScreenRectangle(viewport->clipMinimumA,viewport->clipMinimumB,
            (uint16_t)(viewport->clipMaximumA-viewport->clipMinimumA),(uint16_t)(viewport->clipMaximumB-viewport->clipMinimumB+1));
    }
}
