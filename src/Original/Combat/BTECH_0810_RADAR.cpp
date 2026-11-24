#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_ballistics.h"
#include <stdlib.h>

/*0810:65E2. Signed sensor damage chooses radar or an empty coloured panel. */
void BtechUpdateRadarPanel(void)
{
    if (!combatWorld.playerMechState) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerMechState");
    if ((int16_t)combatWorld.playerMechState->sensorHits<2)
    {
        if (combatStartup.headingRadarSelected) BtechDrawHeadingRadar();
        else BtechDrawTerrainRadar();
        return;
    }
    if (combatImport.activeCockpitLayoutIndex>=COMBAT_COCKPIT_LAYOUTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: activeCockpitLayoutIndex>=COMBAT_COCKPIT_LAYOUTS");
    const BtechScreenRectangle *panel=&combatCockpit.cockpitLayouts[combatImport.activeCockpitLayoutIndex].radarPanel;
    combatVideo.drawPolygonFilled=true;
    if (combatStartup.headingRadarSelected)
        combatVideo.drawLineColour=combatImport.hudColourSchemeIndex?RADAR_ALTERNATE_HEADING_BACKGROUND:RADAR_HEADING_BACKGROUND;
    else
    {
        if (combatImport.hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES");
        combatVideo.drawLineColour=(uint8_t)combatCockpit.hudColourSchemes[combatImport.hudColourSchemeIndex].radarBackground;
    }
    combatVideo.drawFillColour=combatVideo.drawLineColour;
    BtechDrawStyledRectangle(panel->x,panel->y,panel->width,panel->height);
}

/*0810:665A. Body+torso-relative contacts, not the terrain radar's cache.
 * Native671B tests the binding ADDRESS, not its Mech pointer. */
void BtechDrawHeadingRadar(void)
{
    combatVideo.drawPage=combatVideo.backPage;
    if (combatImport.activeCockpitLayoutIndex>=COMBAT_COCKPIT_LAYOUTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: activeCockpitLayoutIndex>=COMBAT_COCKPIT_LAYOUTS");
    const BtechScreenRectangle *panel=&combatCockpit.cockpitLayouts[combatImport.activeCockpitLayoutIndex].radarPanel;
    int16_t halfWidth=(panel->width-(panel->width<0))/2;
    int16_t halfHeight=(panel->height-(panel->height<0))/2;
    uint16_t centreX=panel->x+halfWidth-1;
    uint16_t centreY=panel->y+halfWidth-1; /*6680 uses WIDTH. */
    combatVideo.drawPolygonFilled=true;
    combatVideo.drawLineColour=combatImport.hudColourSchemeIndex?RADAR_ALTERNATE_HEADING_BACKGROUND:RADAR_HEADING_BACKGROUND;
    combatVideo.drawFillColour=combatVideo.drawLineColour;
    BtechDrawStyledRectangle(panel->x,panel->y,panel->width,panel->height);
    combatVideo.drawFillColour=combatImport.hudColourSchemeIndex?Combat_Pal_EGA_White:Combat_Pal_EGA_Green;
    BtechDrawClippedLine(panel->x,panel->y,(int16_t)centreX,(int16_t)centreY);
    BtechDrawClippedLine((int16_t)(panel->x+panel->width-1),panel->y,(int16_t)centreX,(int16_t)centreY);
    if (!combatWorld.playerUnitBinding || !combatWorld.playerUnitBinding->torso || !combatWorld.playerUnitBinding->positionedLeftLeg) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerUnitBinding || !playerUnitBinding->torso || !playerUnitBinding->positionedLeftLeg");
    BtechWorldObjectPrefix *player=&combatWorld.playerUnitBinding->positionedLeftLeg->object.prefix;
    uint16_t heading=player->orientationC+combatWorld.playerUnitBinding->torso->object.prefix.orientationC;
    for (uint16_t slot=1;slot<COMBAT_MECH_SLOTS;++slot)
    {
        if (combatMechs.unitControls[slot].conditionScoreOrExitCode<=COMBAT_EXIT_CODE_THRESHOLD) continue;
        uint16_t paletteIndex;
        if (!combatMechs.unitControls[slot].sideIndex) paletteIndex=combatImport.hudColourSchemeIndex?Combat_Pal_EGA_Black:Combat_Pal_EGA_LightBlue;
        else
        {
            if (!combatMechs.unitVisibility[0][slot]) continue;
            paletteIndex=combatImport.hudColourSchemeIndex?Combat_Pal_EGA_Green:Combat_Pal_EGA_LightRed;
        }
        /* An empty slot (no Mech, condition 0, friendly) still passes the
         * tests above. The original reads its zero torso near pointer:
         * [0+8] and [0+0Ch] are DS:0008/000C, constant null-segment words,
         * so the contact is plotted where they put it, if in range. */
        const BtechPosition3D nullContact=BtechReadNullNearWorldObjectPosition();
        const BtechPosition3D *contact=combatMechs.unitBindings[slot].torso
            ?&combatMechs.unitBindings[slot].torso->object.prefix.position:&nullContact;
        int16_t relativeX=(int16_t)BtechShiftSignedDwordRight(contact->positionA-player->position.positionA,(uint8_t)combatStartup.radarRangeShift);
        int16_t relativeY=(int16_t)BtechShiftSignedDwordRight(contact->positionB-player->position.positionB,(uint8_t)combatStartup.radarRangeShift);
        BtechRotateSignedWordPair(&relativeX,&relativeY,(uint16_t)(0-heading));
        int32_t absoluteX=relativeX==INT16_MIN?INT16_MAX:relativeX<0?-(int32_t)relativeX:relativeX;
        int32_t absoluteY=relativeY==INT16_MIN?INT16_MAX:relativeY<0?-(int32_t)relativeY:relativeY;
        if (absoluteX<halfWidth && absoluteY<halfHeight)
            BtechWriteClippedPixel((int16_t)(centreX+relativeX),(int16_t)(centreY-relativeY-1),paletteIndex);
    }
    combatVideo.textForegroundColour=combatImport.hudColourSchemeIndex?Combat_Pal_EGA_White:Combat_Pal_EGA_Green;combatVideo.textTransparentBackground=true;
    if (combatStartup.radarRangeShift<RADAR_MINIMUM_RANGE_SHIFT || combatStartup.radarRangeShift>RADAR_MAXIMUM_RANGE_SHIFT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: radarRangeShift<RADAR_MINIMUM_RANGE_SHIFT || radarRangeShift>RADAR_MAXIMUM_RANGE_SHIFT"); /* ASCII aliases below2406. */
    BtechDrawNearText(combatCockpit.radarRangeLabels[combatStartup.radarRangeShift-RADAR_MINIMUM_RANGE_SHIFT],
        (int16_t)(panel->x+panel->width/2-0x12),(int16_t)(panel->y+panel->height-8));
    combatVideo.textTransparentBackground=false;
    BtechPresentScreenRectangle(panel->x,panel->y,panel->width,panel->height);
}
