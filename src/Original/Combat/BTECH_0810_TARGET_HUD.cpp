#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_scene.h"
#include "mechwarrior/combat_orders.h"
#include "mechwarrior/combat_damage.h"
#include <stdlib.h>

enum { TARGET_PANEL_DRAW_INSET=1, TARGET_DAMAGE_INSET_X=5,
    TARGET_DAMAGE_WIDTH_TRIM=10, TARGET_DAMAGE_HEIGHT_TRIM=2, TARGET_PANEL_COPY_PADDING=2,
    TARGET_RETICLE_MINIMUM_WIDTH=8, TARGET_RETICLE_OUTSET=2,
    TARGET_RETICLE_OUTER_SIZE_PADDING=4, SPECIAL_TARGET_RETICLE_SCALE=0x09C4 };

/*0810:6BCA. Cached damage panel followed by a live projected target box.
 *53C4 is the last two bytes of689E's ten-byte scratch, not separate storage. */
void BtechUpdateTargetPanelAndReticle(uint16_t targetUnitSlot)
{
    if (!combatWorld.playerMechState || combatWorld.playerMechState->templateIndex>=COMBAT_MECH_TEMPLATES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerMechState || playerMechState->templateIndex>=COMBAT_MECH_TEMPLATES");
    uint16_t layout=combatCockpit.mechCockpitLayoutIndices[combatWorld.playerMechState->templateIndex];
    if (layout>=COMBAT_COCKPIT_LAYOUTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: layout>=COMBAT_COCKPIT_LAYOUTS");
    const BtechScreenRectangle *panel=&combatCockpit.cockpitLayouts[layout].targetPanel;
    combatVideo.screenPolygonNeedsClip=false;combatVideo.drawPolygonFilled=true;
    if (!combatStartup.combatTickCounter) cachedTargetPanelSlot=0;
    combatVideo.drawPage=combatVideo.backPage;
    if (!targetUnitSlot)
    {
        if (cachedTargetPanelSlot)
        {
            cachedTargetPanelSlot=0;combatVideo.drawPolygonFilled=true;combatVideo.drawFillColour=Combat_Pal_EGA_LightGreen;combatVideo.drawLineColour=Combat_Pal_EGA_LightGreen;
            BtechDrawStyledRectangle((uint16_t)(panel->x+TARGET_PANEL_DRAW_INSET),(uint16_t)(panel->y+TARGET_PANEL_DRAW_INSET),panel->width,panel->height);
            BtechCopyHudRectangleIfReady((uint16_t)(panel->x+TARGET_PANEL_DRAW_INSET),(uint16_t)(panel->y+TARGET_PANEL_DRAW_INSET),panel->width,panel->height);
        }
        return;
    }
    if (targetUnitSlot>COMBAT_SPECIAL_TARGET_SLOT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: targetUnitSlot>COMBAT_SPECIAL_TARGET_SLOT");
    uint8_t *dirty=targetUnitSlot==COMBAT_SPECIAL_TARGET_SLOT?&combatDamagePanels.specialTargetDamagePanelDirty:&combatDamagePanels.targetDamagePanelDirty[targetUnitSlot];
    if (targetUnitSlot!=cachedTargetPanelSlot || *dirty || combatCockpit.cockpitFullRedrawPending)
    {
        BtechDrawTargetDamagePanel(targetUnitSlot,panel);
        cachedTargetPanelSlot=targetUnitSlot;*dirty=0;
    }
    combatVideo.screenPolygonNeedsClip=true;combatVideo.drawPolygonFilled=false;
    if (combatImport.hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES || combatImport.combatLandscapeIndex>=COMBAT_LANDSCAPES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES || combatLandscapeIndex>=COMBAT_LANDSCAPES");
    combatVideo.drawFillColour=combatVideo.landscapeCameraColourRows[combatImport.hudColourSchemeIndex][combatImport.combatLandscapeIndex].hudReticleColour;
    BtechPooledWorldObject *target=targetUnitSlot==COMBAT_SPECIAL_TARGET_SLOT?combatWorld.specialTargetObject:combatMechs.unitBindings[targetUnitSlot].torso;
    if (!target || !combatMechs.unitBindings[0].torso) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !target || !unitBindings[0].torso");
    uint32_t distance=BtechApproximatePlanarDistance(&combatMechs.unitBindings[0].torso->object.prefix.position,&target->object.prefix.position);
    int16_t scale;
    if (targetUnitSlot==COMBAT_SPECIAL_TARGET_SLOT) scale=SPECIAL_TARGET_RETICLE_SCALE;
    else
    {
        BtechMechState *mech=combatMechs.unitBindings[targetUnitSlot].mech;
        if (!mech || mech->templateIndex>=COMBAT_MECH_TEMPLATES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mech || mech->templateIndex>=COMBAT_MECH_TEMPLATES");
        scale=(int16_t)combatMechs.mechTemplates[mech->templateIndex].displayScale;
    }
    uint32_t numerator=BtechShiftDwordLeft((uint32_t)scale,(uint8_t)combatStartup.viewScaleShift);
    int16_t ratio=(int16_t)BtechDivideSignedDword(numerator,distance);
    int16_t width=(ratio-(ratio<0))/2+TARGET_RETICLE_MINIMUM_WIDTH;
    int16_t halfWidth=(width-(width<0))/2;
    /* NativeC958+slot8 aliases the existing C968 special projected object. */
    BtechPooledWorldObject *projected=targetUnitSlot==COMBAT_SPECIAL_TARGET_SLOT?combatWorld.objectiveVisibilityObject:combatStartup.unitLastDrawParts[targetUnitSlot];
    if (!projected) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !projected");
    uint16_t x=projected->object.projectedScreenA-halfWidth;
    uint16_t y=projected->object.projectedScreenB-width;
    uint16_t height=(uint16_t)width*2;
    if (targetUnitSlot==COMBAT_SPECIAL_TARGET_SLOT) {x-=halfWidth;width=(int16_t)((uint16_t)width*2);}
    if ((int16_t)targetUnitSlot<COMBAT_FRIENDLY_SLOTS && (targetUnitSlot!=COMBAT_SPECIAL_TARGET_SLOT || combatImport.sideObjectiveRules[0]!=OBJECTIVE_SPECIAL_TARGET))
    {
        BtechDrawStyledRectangle(x,y,(uint16_t)width,height);
        BtechDrawStyledRectangle((uint16_t)(x-TARGET_RETICLE_OUTSET),(uint16_t)(y-TARGET_RETICLE_OUTSET),(uint16_t)(width+TARGET_RETICLE_OUTER_SIZE_PADDING),(uint16_t)(height+TARGET_RETICLE_OUTER_SIZE_PADDING));
    }
    else BtechDrawStyledRectangle(x,y,(uint16_t)width,height);
    combatVideo.screenPolygonNeedsClip=false;
}

/*0810:6DBA. Armour+internal structure condition drives authored diagram boxes.
 * Rear centre torso remains real stored armour but has no diagram region. */
void BtechDrawTargetDamagePanel(uint16_t targetUnitSlot,const BtechScreenRectangle *panel)
{
    if (!panel || combatImport.hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !panel || hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES");
    uint16_t diagram;
    if (targetUnitSlot==COMBAT_SPECIAL_TARGET_SLOT)
    {
        int16_t condition=combatWorld.specialTargetDamageRemaining;
        const BtechHudColourScheme *colours=&combatCockpit.hudColourSchemes[combatImport.hudColourSchemeIndex];
        uint16_t colour=condition==SPECIAL_TARGET_INITIAL_DAMAGE?colours->undamaged:
            condition>SPECIAL_TARGET_INITIAL_DAMAGE/2?colours->aboveHalfCondition:condition?colours->damaged:colours->destroyed;
        combatVideo.drawFillColour=(uint8_t)colour;combatVideo.drawLineColour=(uint8_t)colour;
        BtechDrawStyledRectangle((uint16_t)(panel->x+TARGET_DAMAGE_INSET_X),(uint16_t)(panel->y+TARGET_PANEL_DRAW_INSET),(uint16_t)(panel->width-TARGET_DAMAGE_WIDTH_TRIM),(uint16_t)(panel->height-TARGET_DAMAGE_HEIGHT_TRIM));
        diagram=COMBAT_SPECIAL_TARGET_SLOT;
    }
    else
    {
        if (targetUnitSlot>=COMBAT_MECH_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: targetUnitSlot>=COMBAT_MECH_SLOTS");
        BtechMechState *mech=combatMechs.unitBindings[targetUnitSlot].mech;
        if (!mech || mech->templateIndex>=COMBAT_MECH_TEMPLATES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !mech || mech->templateIndex>=COMBAT_MECH_TEMPLATES");
        const BtechMechTemplate *chassis=&combatMechs.mechTemplates[mech->templateIndex];
        combatVideo.drawLineColour=Combat_Pal_EGA_LightGrey;combatVideo.drawFillColour=Combat_Pal_EGA_LightGrey;
        BtechDrawStyledRectangle((uint16_t)(panel->x+TARGET_DAMAGE_INSET_X),(uint16_t)(panel->y+TARGET_PANEL_DRAW_INSET),(uint16_t)(panel->width-TARGET_DAMAGE_WIDTH_TRIM),(uint16_t)(panel->height-TARGET_DAMAGE_HEIGHT_TRIM));
        for (uint16_t armor=0;armor<COMBAT_ARMOR_LOCATIONS;++armor)
        {
            if (armor==COMBAT_ARMOR_REAR_CENTER_TORSO) continue;
            uint16_t body=combatImport.armorToBodyLocations[armor];if (body>=COMBAT_BODY_LOCATIONS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: body>=COMBAT_BODY_LOCATIONS");
            int16_t remaining=mech->currentArmor[armor]+mech->currentInternalStructure[body];
            int16_t maximum=chassis->maximumArmor[armor]+chassis->maximumInternalStructure[body];
            const BtechHudColourScheme *colours=&combatCockpit.hudColourSchemes[combatImport.hudColourSchemeIndex];
            int16_t halfMaximum=(maximum-(maximum<0))/2;
            uint16_t colour=remaining==maximum?colours->undamaged:remaining>halfMaximum?colours->aboveHalfCondition:remaining?colours->damaged:colours->destroyed;
            if (mech->templateIndex>=COMBAT_MECH_TEMPLATES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: mech->templateIndex>=COMBAT_MECH_TEMPLATES");
            const BtechScreenRectangle *region=&combatCockpit.targetDiagramRegions[mech->templateIndex][armor];
            combatVideo.drawFillColour=(uint8_t)colour;combatVideo.drawLineColour=(uint8_t)colour;
            const BtechHudGaugeAnchor *anchor=&combatCockpit.targetDiagramAnchors[mech->templateIndex];
            BtechDrawStyledRectangle((uint16_t)(region->x-anchor->x+panel->x),(uint16_t)(region->y-anchor->y+panel->y),region->width,region->height);
        }
        diagram=mech->templateIndex;
    }
    if (!combatVideo.smallMechPictures || diagram>=combatVideo.smallMechPictures->imageCount) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !smallMechPictures || diagram>=smallMechPictures->imageCount");
    BtechDrawBitmap(&combatVideo.smallMechPictures->images[diagram],(int16_t)(panel->x+TARGET_PANEL_DRAW_INSET),(int16_t)(panel->y+TARGET_PANEL_DRAW_INSET));
    BtechCopyHudRectangleIfReady(panel->x,panel->y,(uint16_t)(panel->width+TARGET_PANEL_COPY_PADDING),(uint16_t)(panel->height+TARGET_PANEL_COPY_PADDING));
}
