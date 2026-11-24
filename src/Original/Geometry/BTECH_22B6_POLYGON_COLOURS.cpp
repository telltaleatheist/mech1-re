#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_render.h"
#include <stdlib.h>

/*22B6:6501. Record colours, optional normal shade and ordered palette remap.
 * The authored CS table is imported once, not rebuilt from the SDL palette. */
void BtechSelectPolygonColoursAndShade(const BtechRenderPolygon *polygon)
{
    if (combatVideo.graphicsColourMode)
    {
        combatVideo.drawFillColour=polygon->fillColour;
        combatVideo.drawLineColour=polygon->lineColour;
    }
    else
    {
        combatVideo.drawFillColour=polygon->alternateFillColour;
        combatVideo.drawLineColour=polygon->alternateLineColour;
    }
    uint8_t flags=polygon->flags;
    combatVideo.drawPolygonFilled=(flags&POLYGON_FILLED)!=0;
    if (combatVideo.drawPolygonFilled && (flags&POLYGON_SHADE_NORMAL)
        && (flags&POLYGON_FACING_MASK)==1 && combatVideo.graphicsColourMode && combatRender.activeColourShadeClasses)
    {
        uint8_t shadeClass=combatRender.activeColourShadeClasses[combatVideo.drawLineColour];
        if (shadeClass && (int8_t)shadeClass<=POLYGON_SHADE_CLASSES) /* Native signed JG; do not clamp80..FF. */
        {
            combatRender.polygonShadeClass=shadeClass;
            BtechCachePolygonShadeNormal(polygon->normalVectorIndex);
            uint16_t dotHigh=BtechDotProductSignedWordVectors(combatRender.transformedReferenceVector,combatRender.viewVectorCoefficients)>>16;
            if (dotHigh==0x0010) dotHigh=0x000F;
            uint16_t shadeOffset=dotHigh+0x0010+(uint16_t)(uint8_t)(combatRender.polygonShadeClass-1)*POLYGON_SHADE_STEPS;
            if (shadeOffset>=sizeof(polygonShadeDeltas)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: shadeOffset>=sizeof(polygonShadeDeltas)"); /* Native read enters other CS owners, not a ninth shade row. */
            uint8_t originalLineColour=combatVideo.drawLineColour;
            combatVideo.drawLineColour+=polygonShadeDeltas[shadeOffset/POLYGON_SHADE_STEPS][shadeOffset%POLYGON_SHADE_STEPS];
            if (originalLineColour==combatVideo.drawFillColour) combatVideo.drawFillColour=combatVideo.drawLineColour;
        }
    }
    if (combatRender.activePaletteRemap)
    {
        combatVideo.drawFillColour=combatRender.activePaletteRemap[combatVideo.drawFillColour];
        combatVideo.drawLineColour=combatRender.activePaletteRemap[combatVideo.drawLineColour];
    }
}
