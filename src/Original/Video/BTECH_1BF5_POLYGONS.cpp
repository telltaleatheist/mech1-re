#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_polygons.h"
#include <stdlib.h>

/*1BF5:0475. The native copies, winding decision, two edge chains and scanline
 * header remain in this procedure. No host polygon rasterizer is substituted.
 * Typed references transport the unobserved near addresses; buffers retain
 * their original20-WORD unaliased extents and the preceding-row header alias. */
BtechPolygonSpan *BtechDrawScreenPolygon(int16_t count,const int16_t *x,const int16_t *y)
{
    combatPolygons.polygonScanlineHeader=NULL;
    if (count>=0)
    {
        combatPolygons.polygonVertexCount=(uint16_t)count;
        if (count>COMBAT_POLYGON_VERTEX_WORDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: count>COMBAT_POLYGON_VERTEX_WORDS");
        for (uint16_t axis=0;axis<2;++axis)
        {
            const int16_t *source=axis?y:x;int16_t *destination=axis?combatPolygons.screenPolygonY:combatPolygons.screenPolygonX;
            for (uint16_t i=0;i<(uint16_t)count;++i) destination[i]=source[i];
        }
    }
    if (count<2) goto returnPolygon;
    if (count==2) goto drawSingleSegment;
    if (!combatVideo.drawPolygonFilled)
    {
        uint16_t vertices=combatPolygons.polygonVertexCount;
        if (vertices>=COMBAT_POLYGON_VERTEX_WORDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: vertices>=COMBAT_POLYGON_VERTEX_WORDS");
        combatPolygons.screenPolygonX[vertices]=combatPolygons.screenPolygonX[0];combatPolygons.screenPolygonY[vertices]=combatPolygons.screenPolygonY[0];
        BtechDrawScreenPolyline(vertices,combatPolygons.screenPolygonX,combatPolygons.screenPolygonY);
        goto returnPolygon;
    }
    if (combatVideo.drawFillColour!=combatVideo.drawLineColour)
    {
        combatPolygons.polygonOutlineVertexCount=combatPolygons.polygonVertexCount;
        if (combatPolygons.polygonOutlineVertexCount>=COMBAT_POLYGON_VERTEX_WORDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: polygonOutlineVertexCount>=COMBAT_POLYGON_VERTEX_WORDS");
        for (uint16_t axis=0;axis<2;++axis)
        {
            int16_t *source=axis?combatPolygons.screenPolygonY:combatPolygons.screenPolygonX;
            int16_t *destination=axis?combatPolygons.polygonOutlineY:combatPolygons.polygonOutlineX;
            int16_t first=source[0];destination[0]=first;
            for (uint16_t i=1;i<combatPolygons.polygonOutlineVertexCount;++i) destination[i]=source[i];
            destination[combatPolygons.polygonOutlineVertexCount]=first;
        }
    }
    if (combatVideo.screenPolygonNeedsClip) BtechClipScreenPolygonToViewport();
    if (combatPolygons.polygonVertexCount==2) goto drawSingleSegment;
    if ((int16_t)combatPolygons.polygonVertexCount<2) goto returnPolygon;
    combatPolygons.polygonPreviousVertexY=(uint16_t)combatPolygons.screenPolygonY[0];combatPolygons.polygonPreviousVertexX=(uint16_t)combatPolygons.screenPolygonX[0];
    combatPolygons.polygonMinimumYVertexByteOffset=0;combatPolygons.polygonMaximumYVertexByteOffset=0;
    uint16_t source;uint16_t accepted;source=(uint16_t)(combatPolygons.polygonVertexCount-1);accepted=0;
    uint16_t minimumY;uint16_t maximumY;uint16_t minimumXAtMaximumY;uint16_t maximumXAtMinimumY;minimumY=0x7FFF;maximumY=0x8001;minimumXAtMaximumY=0x7FFF;maximumXAtMinimumY=0x8001;
    for (;;)
    {
        uint16_t vertexY=combatPolygons.screenPolygonY[source];
        if (vertexY!=combatPolygons.polygonPreviousVertexY || (uint16_t)combatPolygons.screenPolygonX[source]!=combatPolygons.polygonPreviousVertexX)
        {
            combatPolygons.polygonPreviousVertexY=vertexY;combatPolygons.polygonWorkY[accepted]=(int16_t)vertexY;
            if ((int16_t)vertexY<(int16_t)minimumY || (vertexY==minimumY && combatPolygons.screenPolygonX[source]>(int16_t)maximumXAtMinimumY))
            {combatPolygons.polygonMinimumYVertexByteOffset=accepted*2;minimumY=vertexY;maximumXAtMinimumY=(uint16_t)combatPolygons.screenPolygonX[source];}
            if ((int16_t)vertexY>(int16_t)maximumY || (vertexY==maximumY && combatPolygons.screenPolygonX[source]<=(int16_t)minimumXAtMaximumY))
            {combatPolygons.polygonMaximumYVertexByteOffset=accepted*2;maximumY=vertexY;minimumXAtMaximumY=(uint16_t)combatPolygons.screenPolygonX[source];}
            uint16_t vertexX=combatPolygons.screenPolygonX[source];combatPolygons.polygonPreviousVertexX=vertexX;
            combatPolygons.polygonWorkX[accepted++]=(int16_t)vertexX;
        }
        if (!source) break;
        --source;
    }
    if (minimumY==maximumY || accepted==2)
    {BtechDrawClippedLine((int16_t)minimumXAtMaximumY,(int16_t)maximumY,(int16_t)maximumXAtMinimumY,(int16_t)minimumY);goto returnPolygon;}
    if (accepted<2) goto returnPolygon;
    combatPolygons.polygonVertexCount=accepted;
    uint16_t minimum;uint16_t next;minimum=combatPolygons.polygonMinimumYVertexByteOffset/2;next=minimum+1;
    if (next>=accepted) next=0;
    uint16_t nextNumerator;nextNumerator=combatPolygons.polygonWorkX[next]-combatPolygons.polygonWorkX[minimum];
    uint16_t nextDenominator;nextDenominator=combatPolygons.polygonWorkY[next]-combatPolygons.polygonWorkY[minimum];
    if (!nextDenominator) {nextDenominator=1;nextNumerator=(int16_t)nextNumerator<0?0x8001:0x7FFF;}
    uint16_t previous;previous=minimum?minimum-1:accepted-1;
    uint16_t previousNumerator;previousNumerator=combatPolygons.polygonWorkX[previous]-combatPolygons.polygonWorkX[minimum];
    uint16_t previousDenominator;previousDenominator=combatPolygons.polygonWorkY[previous]-combatPolygons.polygonWorkY[minimum];
    if (!previousDenominator) {previousDenominator=1;previousNumerator=(int16_t)previousNumerator<0?0x8001:0x7FFF;}
    bool reverseWinding;
    if ((int16_t)previousNumerator>=0 && (int16_t)nextNumerator<=0) reverseWinding=true;
    else if ((int16_t)previousNumerator<0 && (int16_t)nextNumerator>=0) reverseWinding=false;
    else
    {
        if ((int16_t)previousNumerator<0)
        {
            uint16_t saved=0-nextNumerator;nextNumerator=(uint16_t)(0-previousNumerator);previousNumerator=saved;
            saved=nextDenominator;nextDenominator=previousDenominator;previousDenominator=saved;
        }
        uint16_t previousQuotient=previousNumerator/previousDenominator,previousRemainder=previousNumerator%previousDenominator;
        uint16_t nextQuotient=nextNumerator/nextDenominator,nextRemainder=nextNumerator%nextDenominator;
        reverseWinding=nextQuotient<previousQuotient;
        if (nextQuotient==previousQuotient)
        {
            uint16_t nextFraction=((uint32_t)nextRemainder<<16)/nextDenominator;
            uint16_t previousFraction=((uint32_t)previousRemainder<<16)/previousDenominator;
            reverseWinding=nextFraction<previousFraction;
        }
    }
    for (uint16_t axis=0;axis<2;++axis)
    {
        int16_t *work=axis?combatPolygons.polygonWorkY:combatPolygons.polygonWorkX,*screen=axis?combatPolygons.screenPolygonY:combatPolygons.screenPolygonX;
        for (uint16_t i=0;i<accepted;++i) screen[reverseWinding?accepted-1-i:i]=work[i];
    }
    if (reverseWinding)
    {
        uint16_t lastByteOffset=(accepted-1)*2;
        combatPolygons.polygonMinimumYVertexByteOffset=lastByteOffset-combatPolygons.polygonMinimumYVertexByteOffset;
        combatPolygons.polygonMaximumYVertexByteOffset=lastByteOffset-combatPolygons.polygonMaximumYVertexByteOffset;
    }
    maximumY=(uint16_t)combatPolygons.screenPolygonY[combatPolygons.polygonMaximumYVertexByteOffset/2];
    source=combatPolygons.polygonMinimumYVertexByteOffset/2;
    uint16_t chainDestination;chainDestination=0;
    for (uint16_t chain=0;chain<2;++chain)
    {
        if (chain)
        {minimumY=(uint16_t)combatPolygons.screenPolygonY[combatPolygons.polygonMinimumYVertexByteOffset/2];source=combatPolygons.polygonMaximumYVertexByteOffset/2;}
        for (;;)
        {
            if (chainDestination>=COMBAT_POLYGON_VERTEX_WORDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: chainDestination>=COMBAT_POLYGON_VERTEX_WORDS");
            combatPolygons.polygonWorkX[chainDestination]=combatPolygons.screenPolygonX[source];
            uint16_t vertexY=combatPolygons.screenPolygonY[source];combatPolygons.polygonWorkY[chainDestination++]=(int16_t)vertexY;
            if (chain?(int16_t)vertexY<=(int16_t)minimumY:(int16_t)vertexY>=(int16_t)maximumY) break;
            if (++source>=accepted) source=0;
        }
        if (!chain) combatPolygons.polygonFirstChainVertexCount=chainDestination;
        else combatPolygons.polygonSecondChainVertexCount=chainDestination-combatPolygons.polygonFirstChainVertexCount;
    }
    combatPolygons.polygonSpanBoundaryByteOffset=2;combatPolygons.polygonChainVertexByteOffset=0;
    uint16_t chainVertices;chainVertices=combatPolygons.polygonFirstChainVertexCount;
    for (;;)
    {
        if (--chainVertices==0)
        {
            if (!combatPolygons.polygonSpanBoundaryByteOffset) break;
            combatPolygons.polygonChainVertexByteOffset+=2;combatPolygons.polygonSpanBoundaryByteOffset=0;
            chainVertices=combatPolygons.polygonSecondChainVertexCount;continue;
        }
        combatPolygons.polygonChainEdgesRemaining=chainVertices;
        uint16_t chainOffset=combatPolygons.polygonChainVertexByteOffset/2;combatPolygons.polygonChainVertexByteOffset+=2;
        if (chainOffset+1>=COMBAT_POLYGON_VERTEX_WORDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: chainOffset+1>=COMBAT_POLYGON_VERTEX_WORDS");
        int16_t firstX=combatPolygons.polygonWorkX[chainOffset],secondX=combatPolygons.polygonWorkX[chainOffset+1];
        int16_t firstY=combatPolygons.polygonWorkY[chainOffset],secondY=combatPolygons.polygonWorkY[chainOffset+1];
        int16_t deltaX=(int16_t)(firstX-secondX),deltaY=(int16_t)(firstY-secondY);
        uint16_t magnitudeX=deltaX<0?(uint16_t)(0-(uint16_t)deltaX):(uint16_t)deltaX;
        uint16_t magnitudeY=deltaY<0?(uint16_t)(0-(uint16_t)deltaY):(uint16_t)deltaY;
        if (!magnitudeX) BtechAccumulateVerticalPolygonEdge(firstY,secondX,secondY);
        else if (!magnitudeY)
        {
            if (firstX>=secondX) {int16_t saved=firstX;firstX=secondX;secondX=saved;}
            if ((uint16_t)firstY>=COMBAT_SCREEN_HEIGHT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (uint16_t)firstY>=COMBAT_SCREEN_HEIGHT");
            if (combatPolygons.polygonSpanBoundaryByteOffset) polygonScanlineSpans[(uint16_t)firstY].endpoints.right=(uint16_t)secondX;
            else polygonScanlineSpans[(uint16_t)firstY].endpoints.left=(uint16_t)firstX;
        }
        else if ((int16_t)magnitudeX<(int16_t)magnitudeY) BtechAccumulateYMajorPolygonEdge(firstY,firstX,secondX,secondY);
        else if (magnitudeX==magnitudeY) BtechAccumulateDiagonalPolygonEdge(firstY,firstX,secondX,secondY);
        else if (!combatPolygons.polygonSpanBoundaryByteOffset) BtechAccumulateAscendingXMajorPolygonEdge(firstY,firstX,secondX,secondY);
        else BtechAccumulateDescendingXMajorPolygonEdge(firstY,firstX,secondX,secondY);
        chainVertices=combatPolygons.polygonChainEdgesRemaining;
    }
    minimumY=(uint16_t)combatPolygons.screenPolygonY[combatPolygons.polygonMinimumYVertexByteOffset/2];
    if (minimumY>=COMBAT_SCREEN_HEIGHT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: minimumY>=COMBAT_SCREEN_HEIGHT");
    combatPolygons.polygonScanlineHeader=&combatPolygons.polygonSpanStorage[minimumY];
    combatPolygons.polygonScanlineHeader->header.firstRow=minimumY;
    maximumY=(uint16_t)combatPolygons.screenPolygonY[combatPolygons.polygonMaximumYVertexByteOffset/2];
    combatPolygons.polygonScanlineHeader->header.rowCount=(uint16_t)(maximumY-minimumY+1);
    if (!combatPolygons.graphicsFillPolygonSpans) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !graphicsFillPolygonSpans");
    combatPolygons.graphicsFillPolygonSpans(combatPolygons.polygonScanlineHeader);
    if (combatVideo.drawFillColour!=combatVideo.drawLineColour) BtechDrawScreenPolyline(combatPolygons.polygonOutlineVertexCount,combatPolygons.polygonOutlineX,combatPolygons.polygonOutlineY);
    goto returnPolygon;
drawSingleSegment:
    BtechDrawScreenPolyline(1,combatPolygons.screenPolygonX,combatPolygons.screenPolygonY);
returnPolygon:
    return combatPolygons.polygonScanlineHeader;
}
