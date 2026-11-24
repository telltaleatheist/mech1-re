#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_polygons.h"
#include "mechwarrior/combat_video.h"
#include <stdlib.h>
#include "mechwarrior/combat_polygons.h"

/*1BF5:1D04. Clipped-away sides do not acquire replacement borders. Vertical
 * edges use035C; top/bottom use solid fill-colour spans, interior line patterns. */
uint16_t BtechDrawStyledRectangle(uint16_t left,uint16_t top,uint16_t width,uint16_t height)
{
    BtechSetEgaSpriteWriteRegisters();
    CombatPage page=combatVideo.drawPageSelector;
    if (combatVideo.combatRendererKind!=COMBAT_VIDEO_EGA) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: combatRendererKind!=COMBAT_VIDEO_EGA");
    combatVideo.spanPrimaryPattern=combatVideo.palettePrimarySlots[combatVideo.drawLineColour/COMBAT_PALETTE_WORDS][combatVideo.drawLineColour%COMBAT_PALETTE_WORDS];
    combatVideo.spanSecondaryPattern=combatVideo.paletteSecondarySlots[combatVideo.drawLineColour/COMBAT_PALETTE_WORDS][combatVideo.drawLineColour%COMBAT_PALETTE_WORDS];
    if (combatVideo.spanPrimaryPattern==combatVideo.spanSecondaryPattern)
        combatVideo.shapeSpanCallback=BtechShapeSpanCallback{combatVideo.graphicsFillSolidSpan,NULL};
    else combatVideo.shapeSpanCallback=BtechShapeSpanCallback{NULL,combatVideo.graphicsFillAlternatingSpan};
    combatVideo.drawColourPattern=combatVideo.palettePrimarySlots[combatVideo.drawFillColour/COMBAT_PALETTE_WORDS][combatVideo.drawFillColour%COMBAT_PALETTE_WORDS];
    if ((int16_t)width<=0 || (int16_t)height<=0) return width;
    uint16_t clippedSides=0;
    if (combatVideo.screenPolygonNeedsClip==1)
    {
        uint16_t margin=left-combatVideo.clipMinimumX;
        if ((int16_t)left<combatVideo.clipMinimumX)
        {
            left=(uint16_t)combatVideo.clipMinimumX;
            int32_t adjusted=(int16_t)width+(int16_t)margin;
            width=(uint16_t)adjusted;
            if (adjusted<=0) return width;
            clippedSides|=1;
        }
        margin=(uint16_t)(top-combatVideo.clipMinimumY);
        if ((int16_t)top<combatVideo.clipMinimumY)
        {
            top=(uint16_t)combatVideo.clipMinimumY;
            int32_t adjusted=(int16_t)height+(int16_t)margin;
            height=(uint16_t)adjusted;
            if (adjusted<=0) return width;
            clippedSides|=2;
        }
        uint16_t end=(uint16_t)(width+left),limit=(uint16_t)(combatVideo.clipMaximumX+1);
        margin=(uint16_t)(limit-end);
        if ((int16_t)limit<(int16_t)end)
        {
            int32_t adjusted=(int16_t)width+(int16_t)margin;
            width=(uint16_t)adjusted;
            if (adjusted<=0) return width;
            clippedSides|=4;
        }
        end=(uint16_t)(height+top);limit=(uint16_t)(combatVideo.clipMaximumY+1);
        margin=(uint16_t)(limit-end);
        if ((int16_t)limit<(int16_t)end)
        {
            clippedSides|=8;
            int32_t adjusted=(int16_t)height+(int16_t)margin;
            height=(uint16_t)adjusted;
            if (adjusted<=0) return width;
        }
    }
    if (!combatVideo.drawPolygonFilled || combatVideo.drawFillColour!=combatVideo.drawLineColour)
    {
        uint8_t savedClip=combatVideo.screenPolygonNeedsClip;combatVideo.screenPolygonNeedsClip=false;
        if (!(clippedSides&1))
        {
            BtechDrawClippedLine((int16_t)left,(int16_t)top,(int16_t)left,(int16_t)(top+height-1));
            ++left;--width;
            if ((int16_t)width<0) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (int16_t)width<0"); /*1E0A native RETF consumes saved clip as ES and corrupts return; clip remains0. */
        }
        if (!(clippedSides&4))
        {
            uint16_t right=left+width-1;
            BtechDrawClippedLine((int16_t)right,(int16_t)top,(int16_t)right,(int16_t)(top+height-1));
            --width;
            if ((int16_t)width<0) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (int16_t)width<0"); /* Same native broken nonlocal return, not a normal C return. */
        }
        if (!((int16_t)height<3 && (clippedSides&0x0A)==0x0A))
        {
            if (!(clippedSides&2))
            {
                if (!combatVideo.graphicsFillSolidSpan || top>=COMBAT_SCREEN_HEIGHT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !graphicsFillSolidSpan || top>=COMBAT_SCREEN_HEIGHT");
                combatVideo.graphicsFillSolidSpan(combatVideo.drawColourPattern,left,(int16_t)width,combatVideo.graphicsRowOffsets[top],page);
                ++top;--height;
                if ((int16_t)height<=0) goto restoreClip;
            }
            if (!(clippedSides&8))
            {
                uint16_t bottom=top+height-1;
                if (!combatVideo.graphicsFillSolidSpan || bottom>=COMBAT_SCREEN_HEIGHT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !graphicsFillSolidSpan || bottom>=COMBAT_SCREEN_HEIGHT");
                combatVideo.graphicsFillSolidSpan(combatVideo.drawColourPattern,left,(int16_t)width,combatVideo.graphicsRowOffsets[bottom],page);
                --height;
            }
        }
restoreClip:
        combatVideo.screenPolygonNeedsClip=savedClip;
        if ((int16_t)height<=0) return width;
    }
    if (!combatVideo.drawPolygonFilled) return width;
    combatVideo.spanAlternationState=top&1;
    if (top>=COMBAT_SCREEN_HEIGHT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: top>=COMBAT_SCREEN_HEIGHT"); /* Adjacent row-table alias not mapped. */
    uint16_t rowOffset=combatVideo.graphicsRowOffsets[top];
    do
    {
        if (combatVideo.shapeSpanCallback.alternating)
            combatVideo.shapeSpanCallback.alternating(combatVideo.spanPrimaryPattern,left,(int16_t)width,combatVideo.spanSecondaryPattern,rowOffset,page);
        else
        {
            if (!combatVideo.shapeSpanCallback.solid) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !shapeSpanCallback.solid");
            combatVideo.shapeSpanCallback.solid(combatVideo.spanPrimaryPattern,left,(int16_t)width,rowOffset,page);
        }
        ++top;
        /* Native reads one extra row WORD after the last span. Row200 aliases
         * the first WORD of the adjacent Hercules table, not palette state. */
        if (top<COMBAT_SCREEN_HEIGHT) rowOffset=combatVideo.graphicsRowOffsets[top];
        else if (top==COMBAT_SCREEN_HEIGHT) rowOffset=combatVideo.herculesDisplayRowOffsets[0];
        else /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rectangle edge references an unsupported scanline.");
    } while (--height);
    return width;
}
