#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_render.h"
#include <stdlib.h>

/*22B6:2908. Native endpoint construction, signed WORD clipping and fill
 * decisions. Non-horizontal split polygons still need0475; no flat substitute. */
void BtechFillTiltedCameraHorizonRegions(uint16_t fillA,uint16_t fillB,uint16_t pitch,uint16_t roll)
{
    combatVideo.drawPolygonFilled=true;
    if (pitch>0x4000 && pitch<=0xC000)
    {
        uint16_t saved=fillA;fillA=fillB;fillB=saved;
        pitch+=0x8000;
    }
    if (roll>0x4000 && roll<=0xC000)
    {
        uint16_t saved=fillA;fillA=fillB;fillB=saved;
        roll+=0x8000;pitch+=0x8000;
    }
    uint16_t angles[]={(uint16_t)(pitch+0xC000),pitch,(uint16_t)(roll+0xC000),roll};
    int16_t trigonometry[4];
    for (uint16_t component=0;component<4;++component)
    {
        uint16_t index=angles[component]>>4;
        if (index&0x0800) index=(uint16_t)(0x1000-index);
        trigonometry[component]=(int16_t)combatGeometryTables.orientationCosines[index];
        if (component==1)
        {
            if (!trigonometry[component]) trigonometry[component]=(int16_t)combatGeometryTables.orientationCosines[0x03FF];
            if (trigonometry[component]<0) trigonometry[component]=(int16_t)(0-(uint16_t)trigonometry[component]);
        }
    }
    uint16_t rollSin=(uint16_t)trigonometry[2],rollCos=(uint16_t)trigonometry[3];
    for (uint16_t shift=0;shift<6;++shift)
    {rollSin=(rollSin>>1)|(rollSin&0x8000);rollCos=(rollCos>>1)|(rollCos&0x8000);}
    combatRender.cameraDepthScratch=(combatRender.cameraDepthScratch&UINT32_C(0xFFFF0000))|(uint16_t)trigonometry[1];
    uint32_t numerator=(uint32_t)trigonometry[0];
    uint16_t shifts=combatRender.combatProjectionShift;
    if ((uint8_t)shifts>=8 && (uint8_t)shifts<0x80)
    {
        numerator<<=8;
        shifts=(shifts&0xFF00)|(uint8_t)((uint8_t)shifts-8);
        if (!(uint8_t)shifts) goto dividePitch;
    }
    do {numerator<<=1;} while (--shifts);
dividePitch: ;
    int16_t projectedPitch;
    for (;;)
    {
        if (!trigonometry[1]) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !trigonometry[1]"); /* Native zero-divisor retry cannot progress. */
        int64_t quotient=(int64_t)(int32_t)numerator/trigonometry[1];
        if (quotient>=INT16_MIN && quotient<=INT16_MAX) {projectedPitch=(int16_t)quotient;break;}
        numerator=runtime_1989::horizon_divide_fallback(numerator);
    }
    int16_t offsetX=(uint32_t)((int32_t)(int16_t)rollSin*projectedPitch)>>8;
    int16_t offsetY=(uint32_t)((int32_t)(int16_t)(0-rollCos)*projectedPitch)>>8;
    int16_t left=combatRender.screenCentreA-rollCos+offsetX;
    int16_t right=combatRender.screenCentreA+rollCos+offsetX;
    int16_t leftY=combatRender.screenCentreB-((uint16_t)(0-rollSin)+offsetY);
    int16_t rightY=combatRender.screenCentreB-(rollSin+offsetY);
    uint16_t wholeColour;
    if (right<combatVideo.clipMinimumX) {wholeColour=roll>=0x8000?fillA:fillB;goto fillWhole;}
    if (left<combatVideo.clipMinimumX)
    {
        numerator=(uint32_t)((int32_t)(int16_t)(rightY-leftY)*(int16_t)(combatVideo.clipMinimumX-left));
        int16_t divisor=right-left;
        for (;;)
        {
            if (!divisor) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !divisor");
            int64_t quotient=(int64_t)(int32_t)numerator/divisor;
            if (quotient>=INT16_MIN && quotient<=INT16_MAX) {leftY=(int16_t)(leftY+quotient);break;}
            numerator=runtime_1989::horizon_divide_fallback(numerator);
        }
        left=combatVideo.clipMinimumX;
    }
    else if (left>combatVideo.clipMaximumX) {wholeColour=roll>=0x8000?fillB:fillA;goto fillWhole;}
    if (right>combatVideo.clipMaximumX)
    {
        numerator=(uint32_t)((int32_t)(int16_t)(leftY-rightY)*(int16_t)(combatVideo.clipMaximumX-right));
        int16_t divisor=left-right;
        for (;;)
        {
            if (!divisor) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !divisor");
            int64_t quotient=(int64_t)(int32_t)numerator/divisor;
            if (quotient>=INT16_MIN && quotient<=INT16_MAX) {rightY=(int16_t)(rightY+quotient);break;}
            numerator=runtime_1989::horizon_divide_fallback(numerator);
        }
        right=combatVideo.clipMaximumX;
    }
    if (rightY<combatVideo.clipMinimumY)
    {
        if (leftY<combatVideo.clipMinimumY) {wholeColour=fillB;goto fillWhole;}
        numerator=(uint32_t)((int32_t)(int16_t)(left-right)*(int16_t)(combatVideo.clipMinimumY-rightY));
        int16_t divisor=leftY-rightY;
        for (;;)
        {
            if (!divisor) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !divisor");
            int64_t quotient=(int64_t)(int32_t)numerator/divisor;
            if (quotient>=INT16_MIN && quotient<=INT16_MAX) {right=(int16_t)(right+quotient);break;}
            numerator=runtime_1989::horizon_divide_fallback(numerator);
        }
        rightY=combatVideo.clipMinimumY;
        goto clipLeftBottom;
    }
    if (leftY<combatVideo.clipMinimumY)
    {
        numerator=(uint32_t)((int32_t)(int16_t)(right-left)*(int16_t)(combatVideo.clipMinimumY-leftY));
        int16_t divisor=rightY-leftY;
        for (;;)
        {
            if (!divisor) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !divisor");
            int64_t quotient=(int64_t)(int32_t)numerator/divisor;
            if (quotient>=INT16_MIN && quotient<=INT16_MAX) {left=(int16_t)(left+quotient);break;}
            numerator=runtime_1989::horizon_divide_fallback(numerator);
        }
        leftY=combatVideo.clipMinimumY;
    }
    if (rightY>combatVideo.clipMaximumY)
    {
        if (leftY>combatVideo.clipMaximumY) {wholeColour=fillA;goto fillWhole;}
        numerator=(uint32_t)((int32_t)(int16_t)(left-right)*(int16_t)(combatVideo.clipMaximumY-rightY));
        int16_t divisor=leftY-rightY;
        for (;;)
        {
            if (!divisor) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !divisor");
            int64_t quotient=(int64_t)(int32_t)numerator/divisor;
            if (quotient>=INT16_MIN && quotient<=INT16_MAX) {right=(int16_t)(right+quotient);break;}
            numerator=runtime_1989::horizon_divide_fallback(numerator);
        }
        rightY=combatVideo.clipMaximumY;
        goto fillRegions;
    }
clipLeftBottom:
    if (leftY>combatVideo.clipMaximumY)
    {
        numerator=(uint32_t)((int32_t)(int16_t)(right-left)*(int16_t)(combatVideo.clipMaximumY-leftY));
        int16_t divisor=rightY-leftY;
        for (;;)
        {
            if (!divisor) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !divisor");
            int64_t quotient=(int64_t)(int32_t)numerator/divisor;
            if (quotient>=INT16_MIN && quotient<=INT16_MAX) {left=(int16_t)(left+quotient);break;}
            numerator=runtime_1989::horizon_divide_fallback(numerator);
        }
        leftY=combatVideo.clipMaximumY;
    }
fillRegions: ;
    int16_t top;int16_t bottom;top=leftY<rightY?leftY:rightY;bottom=leftY>rightY?leftY:rightY;
    if (top!=combatVideo.clipMinimumY)
    {
        combatVideo.drawLineColour=(uint8_t)fillA;combatVideo.drawFillColour=(uint8_t)fillA;combatVideo.screenPolygonNeedsClip=false;
        BtechDrawStyledRectangle((uint16_t)combatVideo.clipMinimumX,(uint16_t)combatVideo.clipMinimumY,
            (uint16_t)(combatVideo.clipMaximumX-combatVideo.clipMinimumX+1),(uint16_t)(top-combatVideo.clipMinimumY+1));
    }
    if (bottom<combatVideo.clipMaximumY)
    {
        combatVideo.drawLineColour=(uint8_t)fillB;combatVideo.drawFillColour=(uint8_t)fillB;combatVideo.screenPolygonNeedsClip=false;
        BtechDrawStyledRectangle((uint16_t)combatVideo.clipMinimumX,(uint16_t)bottom,
            (uint16_t)(combatVideo.clipMaximumX-combatVideo.clipMinimumX+1),(uint16_t)(combatVideo.clipMaximumY-bottom+1));
    }
    if (leftY!=rightY)
    {
        uint16_t leftColour=leftY>rightY?fillA:fillB,rightColour=leftY>rightY?fillB:fillA;
        if (left==right)
        {
            combatVideo.drawLineColour=(uint8_t)leftColour;combatVideo.drawFillColour=(uint8_t)leftColour;
            BtechDrawStyledRectangle((uint16_t)combatVideo.clipMinimumX,(uint16_t)combatVideo.clipMinimumY,
                (uint16_t)(right+1-combatVideo.clipMinimumX),(uint16_t)(combatVideo.clipMaximumY-combatVideo.clipMinimumY+1));
            combatVideo.drawLineColour=(uint8_t)rightColour;combatVideo.drawFillColour=(uint8_t)rightColour;
            BtechDrawStyledRectangle((uint16_t)(right+1),(uint16_t)combatVideo.clipMinimumY,
                (uint16_t)(combatVideo.clipMaximumX-right),(uint16_t)(combatVideo.clipMaximumY-combatVideo.clipMinimumY+1));
        }
        else
        {
            combatRender.horizonPolygonY[0]=rightY;combatRender.horizonPolygonY[1]=rightY;
            combatRender.horizonPolygonY[2]=leftY;combatRender.horizonPolygonY[3]=leftY;
            combatRender.horizonPolygonX[1]=right;combatRender.horizonPolygonX[2]=left;
            combatRender.horizonPolygonX[0]=combatVideo.clipMinimumX;combatRender.horizonPolygonX[3]=combatVideo.clipMinimumX;
            combatVideo.drawLineColour=(uint8_t)leftColour;combatVideo.drawFillColour=(uint8_t)leftColour;
            BtechDrawScreenPolygon(4,combatRender.horizonPolygonX,combatRender.horizonPolygonY);
            combatRender.horizonPolygonX[0]=combatVideo.clipMaximumX;combatRender.horizonPolygonX[3]=combatVideo.clipMaximumX;
            combatVideo.drawLineColour=(uint8_t)rightColour;combatVideo.drawFillColour=(uint8_t)rightColour;
            BtechDrawScreenPolygon(4,combatRender.horizonPolygonX,combatRender.horizonPolygonY);
        }
    }
    combatVideo.screenPolygonNeedsClip=true;
    return;
fillWhole:
    combatVideo.drawLineColour=(uint8_t)wholeColour;combatVideo.drawFillColour=(uint8_t)wholeColour;combatVideo.screenPolygonNeedsClip=false;
    BtechDrawStyledRectangle((uint16_t)combatVideo.clipMinimumX,(uint16_t)combatVideo.clipMinimumY,
        (uint16_t)(combatVideo.clipMaximumX-combatVideo.clipMinimumX+1),(uint16_t)(combatVideo.clipMaximumY-combatVideo.clipMinimumY+1));
    combatVideo.screenPolygonNeedsClip=true;
}
