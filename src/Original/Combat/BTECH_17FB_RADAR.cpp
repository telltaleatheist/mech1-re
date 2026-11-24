#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_orders.h"
#include <stdlib.h>

/*17FB:1C84. Terrain pixels and page-interleaved contact backgrounds. Previous
 * tick240E is authored state, not a BSS variable cleared between stages. */
void BtechDrawTerrainRadar(void)
{
    uint8_t backdropChanged=0;
    uint16_t tickDiscontinuity=(uint16_t)(combatCockpit.radarPreviousTick+1)!=combatStartup.combatTickCounter;
    combatCockpit.radarPreviousTick=combatStartup.combatTickCounter;
    combatCockpit.radarCachePage=combatImport.hudColourSchemeIndex?0:combatStartup.combatTickCounter&1;
    combatVideo.drawPageSelector=combatVideo.backPageSelector;
    if (!combatWorld.playerUnitBinding) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !playerUnitBinding");
    combatCockpit.radarPlayerObject=combatWorld.playerUnitBinding->positionedLeftLeg;
    if (combatImport.activeCockpitLayoutIndex>=COMBAT_COCKPIT_LAYOUTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: activeCockpitLayoutIndex>=COMBAT_COCKPIT_LAYOUTS");
    combatCockpit.radarRectangle=&combatCockpit.cockpitLayouts[combatImport.activeCockpitLayoutIndex].radarPanel;
    int16_t halfWidth=(combatCockpit.radarRectangle->width-(combatCockpit.radarRectangle->width<0))/2;
    int16_t halfHeight=(combatCockpit.radarRectangle->height-(combatCockpit.radarRectangle->height<0))/2;
    combatCockpit.radarCentreX=(uint16_t)(combatCockpit.radarRectangle->x+halfWidth-1);
    combatCockpit.radarCentreY=(uint16_t)(combatCockpit.radarRectangle->y+halfHeight-1);
    if (!combatCockpit.radarPlayerObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !radarPlayerObject");
    const BtechPosition3D *player=&combatCockpit.radarPlayerObject->object.prefix.position;
    uint16_t mapX=BtechShiftSignedDwordRightLow(player->positionA,TERRAIN_WORLD_CELL_SHIFT)-halfWidth+TERRAIN_TILE_ROWS;
    uint16_t mapY=TERRAIN_TILE_COLUMNS-halfHeight-BtechShiftSignedDwordRightLow(player->positionB,TERRAIN_WORLD_CELL_SHIFT);
    if (mapX!=combatCockpit.radarMapOriginX || mapY!=combatCockpit.radarMapOriginY || combatCockpit.cockpitFullRedrawPending || tickDiscontinuity)
    {
        combatCockpit.radarMapOriginX=mapX;combatCockpit.radarMapOriginY=mapY;combatVideo.drawPolygonFilled=true;
        if (combatImport.hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES");
        combatVideo.drawLineColour=(uint8_t)combatCockpit.hudColourSchemes[combatImport.hudColourSchemeIndex].radarBackground;
        combatVideo.drawFillColour=combatVideo.drawLineColour;
        BtechDrawStyledRectangle(combatCockpit.radarRectangle->x,combatCockpit.radarRectangle->y,combatCockpit.radarRectangle->width,combatCockpit.radarRectangle->height);
        for (uint16_t panelX=0;(int16_t)panelX<combatCockpit.radarRectangle->width;++panelX,++mapX)
        {
            uint16_t terrainY=mapY;
            for (uint16_t panelY=0;(int16_t)panelY<combatCockpit.radarRectangle->height;++panelY,++terrainY)
            {
                if (mapX>=TERRAIN_GRID_ROWS || terrainY>=TERRAIN_GRID_COLUMNS) continue;
                uint16_t height=combatWorld.terrainHeightGrid[mapX][terrainY];
                if (!height) continue;
                uint16_t band=height>>5;if (band>3) band=3;
                BtechWriteClippedPixel((int16_t)(combatCockpit.radarRectangle->x+panelX),
                    (int16_t)(combatCockpit.radarRectangle->y+panelY),combatCockpit.hudColourSchemes[combatImport.hudColourSchemeIndex].radarTerrainBands[band]);
            }
        }
        backdropChanged=1;
    }
    else
    {
        for (uint16_t marker=0;(int16_t)marker<(int16_t)combatCockpit.radarMarkerCounts[combatCockpit.radarCachePage];++marker)
        {
            if (marker>=COMBAT_RADAR_MARKERS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: marker>=COMBAT_RADAR_MARKERS"); /* Native next-owner alias. */
            BtechWriteClippedPixel((int16_t)combatCockpit.radarMarkerX[marker][combatCockpit.radarCachePage],
                (int16_t)combatCockpit.radarMarkerY[marker][combatCockpit.radarCachePage],combatCockpit.radarMarkerBackground[marker][combatCockpit.radarCachePage]);
        }
    }
    combatCockpit.radarMarkerCounts[combatCockpit.radarCachePage]=0;
    if (combatImport.hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES");
    uint16_t paletteIndex=combatCockpit.hudColourSchemes[combatImport.hudColourSchemeIndex].radarFriendly;
    for (uint16_t slot=1;slot<COMBAT_MECH_SLOTS;++slot)
    {
        if (!combatMechs.unitBindings[slot].mech || combatMechs.unitControls[slot].conditionScoreOrExitCode<=COMBAT_EXIT_CODE_THRESHOLD) continue;
        if (slot>=COMBAT_FRIENDLY_SLOTS)
        {
            if (!combatMechs.unitVisibility[0][slot]) continue;
            paletteIndex=combatCockpit.hudColourSchemes[combatImport.hudColourSchemeIndex].radarOpposing;
        }
        if (!combatMechs.unitBindings[slot].torso) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !unitBindings[slot].torso");
        BtechDrawRadarPositionMarker(&combatMechs.unitBindings[slot].torso->object.prefix.position,paletteIndex);
    }
    if (combatImport.sideObjectiveRules[0]==OBJECTIVE_SPECIAL_TARGET || combatImport.sideObjectiveRules[0]==OBJECTIVE_DEFEND_SPECIAL_TARGET)
    {
        if (!combatWorld.specialTargetObject) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !specialTargetObject");
        BtechDrawRadarPositionMarker(&combatWorld.specialTargetObject->object.prefix.position,combatCockpit.hudColourSchemes[combatImport.hudColourSchemeIndex].radarSpecialTarget);
    }
    BtechDrawRadarPositionMarker(player,combatCockpit.hudColourSchemes[combatImport.hudColourSchemeIndex].radarPlayer);
    if (backdropChanged)
    {
        BtechCopyHudRectangleIfReady(combatCockpit.radarRectangle->x,combatCockpit.radarRectangle->y,combatCockpit.radarRectangle->width,combatCockpit.radarRectangle->height);
        if (!combatImport.hudColourSchemeIndex)
        {
            uint16_t otherPage=combatCockpit.radarCachePage<1?1:0;
            combatCockpit.radarMarkerCounts[otherPage]=combatCockpit.radarMarkerCounts[combatCockpit.radarCachePage];
            uint16_t count=combatCockpit.radarMarkerCounts[combatCockpit.radarCachePage];
            for (uint16_t marker=0;(int16_t)marker<(int16_t)count;++marker)
            {
                if (marker>=COMBAT_RADAR_MARKERS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: marker>=COMBAT_RADAR_MARKERS");
                combatCockpit.radarMarkerX[marker][otherPage]=combatCockpit.radarMarkerX[marker][combatCockpit.radarCachePage];
                combatCockpit.radarMarkerY[marker][otherPage]=combatCockpit.radarMarkerY[marker][combatCockpit.radarCachePage];
                combatCockpit.radarMarkerBackground[marker][otherPage]=combatCockpit.radarMarkerBackground[marker][combatCockpit.radarCachePage];
            }
        }
    }
    else BtechPresentHudRectangleIfReady(combatCockpit.radarRectangle->x,combatCockpit.radarRectangle->y,combatCockpit.radarRectangle->width,combatCockpit.radarRectangle->height);
}

/*17FB:204E. Shift each absolute DWORD before subtracting LOW WORDs. Cache the
 * background even when2127 returnsFFFF for a clipped pixel. */
void BtechDrawRadarPositionMarker(const BtechPosition3D *position,uint16_t paletteIndex)
{
    if (!combatCockpit.radarPlayerObject || !position) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !radarPlayerObject || !position");
    const BtechPosition3D *player=&combatCockpit.radarPlayerObject->object.prefix.position;
    uint16_t playerX=BtechShiftSignedDwordRightLow(player->positionA,TERRAIN_WORLD_CELL_SHIFT);
    int16_t relativeX=BtechShiftSignedDwordRightLow(position->positionA,TERRAIN_WORLD_CELL_SHIFT)-playerX;
    uint16_t playerY=BtechShiftSignedDwordRightLow(player->positionB,TERRAIN_WORLD_CELL_SHIFT);
    int16_t relativeY=BtechShiftSignedDwordRightLow(position->positionB,TERRAIN_WORLD_CELL_SHIFT)-playerY;
    if (!combatCockpit.radarRectangle) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !radarRectangle");
    int16_t halfWidth=(combatCockpit.radarRectangle->width-(combatCockpit.radarRectangle->width<0))/2;
    int16_t halfHeight=(combatCockpit.radarRectangle->height-(combatCockpit.radarRectangle->height<0))/2;
    int32_t absoluteX=relativeX==INT16_MIN?INT16_MAX:relativeX<0?-(int32_t)relativeX:relativeX;
    if (absoluteX>=halfWidth) return;
    int32_t absoluteY=relativeY==INT16_MIN?INT16_MAX:relativeY<0?-(int32_t)relativeY:relativeY;
    if (absoluteY>=halfHeight) return;
    uint16_t x=(uint16_t)(combatCockpit.radarCentreX+relativeX),y=(uint16_t)(combatCockpit.radarCentreY-relativeY);
    if (combatCockpit.radarCachePage>=COMBAT_RADAR_CACHE_PAGES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: radarCachePage>=COMBAT_RADAR_CACHE_PAGES");
    uint16_t marker=combatCockpit.radarMarkerCounts[combatCockpit.radarCachePage]++;
    if (marker>=COMBAT_RADAR_MARKERS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: marker>=COMBAT_RADAR_MARKERS"); /* Counter is already incremented. */
    combatCockpit.radarMarkerX[marker][combatCockpit.radarCachePage]=x;combatCockpit.radarMarkerY[marker][combatCockpit.radarCachePage]=y;
    combatCockpit.radarMarkerBackground[marker][combatCockpit.radarCachePage]=BtechReadClippedPixel((int16_t)x,(int16_t)y);
    BtechWriteClippedPixel((int16_t)x,(int16_t)y,paletteIndex);
}
