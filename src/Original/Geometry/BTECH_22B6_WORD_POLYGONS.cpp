#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_render.h"
#include <stdlib.h>

/*22B6:6A24. WORD-stride cache, any nonzero flag reuses existing coordinates.
 * Native geometry BP is an ES offset, not a stack frame. */
void BtechCacheTransformedPolygonVertex(uint8_t vectorIndex)
{
    if (vectorIndex>=GEOMETRY_CACHE_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: vectorIndex>=GEOMETRY_CACHE_VERTICES");
    if (combatGeometryTables.vertexNearPlaneState[vectorIndex]) return;
    combatGeometryTables.vertexNearPlaneState[vectorIndex]=GEOMETRY_VERTEX_TRANSFORMED;
    for (uint16_t axis=0;axis<3;++axis) combatRender.viewVectorCoefficients[axis]=combatRender.renderGeometry[vectorIndex][axis];
    BtechTransformWordVector(combatRender.viewVectorCoefficients,combatRender.activeRenderBasis,combatRender.transformedEntryVector);
    combatGeometry.geometryViewX.words[vectorIndex]=(uint16_t)(combatRender.transformedEntryVector[0]+(uint16_t)combatRender.transformedViewPosition.positionA);
    combatGeometry.geometryViewDepth.words[vectorIndex]=(uint16_t)(combatRender.transformedEntryVector[1]+(uint16_t)combatRender.transformedViewPosition.positionB);
    combatGeometry.geometryViewZ.words[vectorIndex]=(uint16_t)(combatRender.transformedEntryVector[2]+(uint16_t)combatRender.transformedViewPosition.positionC);
}

/*22B6:6AC2. Normals share the vertex cache flags but omit translation. */
void BtechCacheTransformedPolygonNormal(uint8_t vectorIndex)
{
    if (vectorIndex>=GEOMETRY_CACHE_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: vectorIndex>=GEOMETRY_CACHE_VERTICES");
    if (combatGeometryTables.vertexNearPlaneState[vectorIndex]) return;
    combatGeometryTables.vertexNearPlaneState[vectorIndex]=GEOMETRY_VERTEX_TRANSFORMED;
    for (uint16_t axis=0;axis<3;++axis) combatRender.viewVectorCoefficients[axis]=combatRender.renderGeometry[vectorIndex][axis];
    BtechTransformWordVector(combatRender.viewVectorCoefficients,combatRender.activeRenderBasis,combatRender.transformedEntryVector);
    combatGeometry.geometryViewX.words[vectorIndex]=combatRender.transformedEntryVector[0];
    combatGeometry.geometryViewDepth.words[vectorIndex]=combatRender.transformedEntryVector[1];
    combatGeometry.geometryViewZ.words[vectorIndex]=combatRender.transformedEntryVector[2];
}

/*22B6:6BD0, including shared6B51 direct-basis continuation, not a new call.
 * Return only the HIGH WORD observed by276A; native dot wraps at32 bits. */
int16_t BtechGetPolygonFacingDotHighWord(uint8_t normalIndex,uint8_t firstVertexIndex)
{
    if (normalIndex>=GEOMETRY_CACHE_VERTICES || firstVertexIndex>=GEOMETRY_CACHE_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: normalIndex>=GEOMETRY_CACHE_VERTICES || firstVertexIndex>=GEOMETRY_CACHE_VERTICES");
    uint16_t vertex[3],normal[3];
    if (combatRender.useViewBasisDirectly)
    {
        vertex[0]=(uint16_t)(combatRender.renderGeometry[firstVertexIndex][0]+(uint16_t)combatRender.cameraRelativePosition.positionA);
        vertex[1]=(uint16_t)(combatRender.renderGeometry[firstVertexIndex][1]+(uint16_t)combatRender.cameraRelativePosition.positionB);
        vertex[2]=(uint16_t)(combatRender.renderGeometry[firstVertexIndex][2]+(uint16_t)combatRender.cameraRelativePosition.positionC);
        for (uint16_t axis=0;axis<3;++axis) normal[axis]=combatRender.renderGeometry[normalIndex][axis];
    }
    else
    {
        BtechCacheTransformedPolygonNormal(normalIndex);
        BtechCacheTransformedPolygonVertex(firstVertexIndex);
        vertex[0]=combatGeometry.geometryViewX.words[firstVertexIndex];normal[0]=combatGeometry.geometryViewX.words[normalIndex];
        vertex[1]=combatGeometry.geometryViewDepth.words[firstVertexIndex];normal[1]=combatGeometry.geometryViewDepth.words[normalIndex];
        vertex[2]=combatGeometry.geometryViewZ.words[firstVertexIndex];normal[2]=combatGeometry.geometryViewZ.words[normalIndex];
    }
    uint32_t dot=0;
    for (uint16_t axis=0;axis<3;++axis) dot+=(uint32_t)((int32_t)(int16_t)vertex[axis]*(int16_t)normal[axis]);
    return (int16_t)(dot>>16);
}

/*22B6:2601. Crossing overwrites the OUTSIDE vertex's projected cache slot.
 * Differences wrap toWORD before signed IMUL and trapping IDIV. */
void BtechIntersectWordEdgeWithNearPlane(uint16_t outsideVertex,uint16_t insideVertex)
{
    if (outsideVertex>=GEOMETRY_CACHE_VERTICES || insideVertex>=GEOMETRY_CACHE_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: outsideVertex>=GEOMETRY_CACHE_VERTICES || insideVertex>=GEOMETRY_CACHE_VERTICES");
    int16_t denominator=combatGeometry.geometryViewDepth.words[insideVertex]-combatGeometry.geometryViewDepth.words[outsideVertex];
    int16_t planeDistance=combatRender.nearClipDepth-combatGeometry.geometryViewDepth.words[outsideVertex];
    BtechGeometryCoordinateCache *source[]={&combatGeometry.geometryViewX,&combatGeometry.geometryViewZ};
    BtechGeometryCoordinateCache *destination[]={&combatGeometry.geometryProjectedX,&combatGeometry.geometryProjectedY};
    for (uint16_t axis=0;axis<2;++axis)
    {
        int16_t difference=source[axis]->words[insideVertex]-source[axis]->words[outsideVertex];
        uint32_t product=(int32_t)difference*planeDistance;
        if (!denominator) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !denominator"); /* Native zero-divisor retries forever. */
        int64_t quotient=(int64_t)(int32_t)product/denominator;
        while (quotient<INT16_MIN || quotient>INT16_MAX)
        {product=runtime_1989::horizon_divide_fallback(product);quotient=(int64_t)(int32_t)product/denominator;}
        destination[axis]->words[outsideVertex]=(uint16_t)(source[axis]->words[outsideVertex]+(int16_t)quotient);
    }
}

/*22B6:2653. Equality-count loop and crossing/current output order. No new
 * vertices: intersections reuse the outside index. */
void BtechClipWordPolygonNearPlane(void)
{
    if (!combatPolygons.polygonVertexCount || combatPolygons.polygonVertexCount>POLYGON_VERTEX_INDEX_BYTES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !polygonVertexCount || polygonVertexCount>POLYGON_VERTEX_INDEX_BYTES"); /* Native empty aliases4616. */
    uint16_t previous=combatGeometry.polygonVertexIndices[combatPolygons.polygonVertexCount-1],output=0,input=0;
    do
    {
        uint16_t current=combatGeometry.polygonVertexIndices[input],emit[2],emitted=0;
        if (previous>=GEOMETRY_CACHE_VERTICES || current>=GEOMETRY_CACHE_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: previous>=GEOMETRY_CACHE_VERTICES || current>=GEOMETRY_CACHE_VERTICES");
        if (combatGeometryTables.vertexNearPlaneState[current]!=GEOMETRY_VERTEX_BEHIND)
        {
            if (combatGeometryTables.vertexNearPlaneState[previous]==GEOMETRY_VERTEX_BEHIND)
            {BtechIntersectWordEdgeWithNearPlane(previous,current);emit[emitted++]=previous;}
            emit[emitted++]=current;
        }
        else if (combatGeometryTables.vertexNearPlaneState[previous]!=GEOMETRY_VERTEX_BEHIND)
        {BtechIntersectWordEdgeWithNearPlane(current,previous);emit[emitted++]=current;}
        for (uint16_t index=0;index<emitted;++index)
        {
            if (output>=COMBAT_POLYGON_VERTEX_WORDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: output>=COMBAT_POLYGON_VERTEX_WORDS");
            combatPolygons.screenPolygonX[output]=(int16_t)((uint16_t)combatRender.screenCentreA+combatGeometry.geometryProjectedX.words[emit[index]]);
            combatPolygons.screenPolygonY[output]=(int16_t)((uint16_t)combatRender.screenCentreB-combatGeometry.geometryProjectedY.words[emit[index]]);
            ++output;
        }
        previous=current;++input;
    } while (input!=combatPolygons.polygonVertexCount);
    combatPolygons.polygonVertexCount=output;
}

/*22B6:276A. FAR tails select6836 or genuine DBEE RETF; otherwise native WORD
 * facing, projection, near clipping and shared0475 viewport/raster path. */
void BtechDrawPolygonPayload(const BtechRenderPolygonPayload *payload)
{
    if (combatRender.widePolygonCoordinates) {BtechRenderWidePolygonPayload(payload);return;}
    if (combatRender.skipWordPolygonRenderer) return;
    combatRender.wordPolygonsRemaining=(uint16_t)payload->polygonCount;
    const BtechRenderPolygon *polygon=payload->polygons;
    for (;;++polygon)
    {
        /* DEC/JGE observes overflow too:8000 ->7FFF still exits. */
        if ((int16_t)combatRender.wordPolygonsRemaining--<=0) return;
        uint8_t flags=polygon->flags;
        combatVideo.drawPolygonFilled=(flags&POLYGON_FILLED)!=0;
        if ((flags&POLYGON_FACING_MASK)==2) continue;
        if ((flags&POLYGON_FACING_MASK)!=0
            && BtechGetPolygonFacingDotHighWord(polygon->normalVectorIndex,polygon->vertexIndices[0])>=0) continue;
        BtechSelectPolygonColoursAndShade(polygon);
        combatRender.wordPolygonHasNearOutsideVertex=0;combatPolygons.polygonVertexCount=0;
        for (const uint8_t *indices=polygon->vertexIndices;*indices!=POLYGON_VERTEX_LIST_END;++indices)
        {
            uint8_t vertex=*indices;
            BtechCacheTransformedPolygonVertex(vertex);
            if (combatGeometryTables.vertexNearPlaneState[vertex]!=GEOMETRY_VERTEX_PROJECTED)
            {
                if ((int16_t)combatGeometry.geometryViewDepth.words[vertex]<(int16_t)combatRender.nearClipDepth)
                {combatGeometryTables.vertexNearPlaneState[vertex]=GEOMETRY_VERTEX_BEHIND;combatRender.wordPolygonHasNearOutsideVertex=GEOMETRY_VERTEX_BEHIND;}
                else
                {
                    BtechGeometryCoordinateCache *source[]={&combatGeometry.geometryViewX,&combatGeometry.geometryViewZ};
                    BtechGeometryCoordinateCache *destination[]={&combatGeometry.geometryProjectedX,&combatGeometry.geometryProjectedY};
                    int16_t depth=combatGeometry.geometryViewDepth.words[vertex];
                    for (uint16_t axis=0;axis<2;++axis)
                    {
                        uint32_t numerator=(int16_t)source[axis]->words[vertex];
                        uint16_t shifts=combatRender.combatProjectionShift;
                        if ((int8_t)(uint8_t)shifts>=8)
                        {
                            numerator<<=8;shifts=(shifts&0xFF00)|(uint8_t)((uint8_t)shifts-8);
                            if (!(uint8_t)shifts) goto divideCoordinate;
                        }
                        do {numerator<<=1;} while (--shifts);
divideCoordinate:
                        if (!depth) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !depth"); /* Native division fault cannot progress on zero. */
                        int64_t quotient=(int64_t)(int32_t)numerator/depth;
                        while (quotient<INT16_MIN || quotient>INT16_MAX)
                        {numerator=runtime_1989::horizon_divide_fallback(numerator);quotient=(int64_t)(int32_t)numerator/depth;}
                        destination[axis]->words[vertex]=(uint16_t)quotient;
                    }
                    combatGeometryTables.vertexNearPlaneState[vertex]=GEOMETRY_VERTEX_PROJECTED;
                }
            }
            if (combatPolygons.polygonVertexCount>=POLYGON_VERTEX_INDEX_BYTES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: polygonVertexCount>=POLYGON_VERTEX_INDEX_BYTES");
            combatGeometry.polygonVertexIndices[combatPolygons.polygonVertexCount++]=vertex;
        }
        if (combatRender.wordPolygonHasNearOutsideVertex) BtechClipWordPolygonNearPlane();
        else
        {
            if (!combatPolygons.polygonVertexCount) continue;
            for (uint16_t index=0;index<combatPolygons.polygonVertexCount;++index)
            {
                if (index>=COMBAT_POLYGON_VERTEX_WORDS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: index>=COMBAT_POLYGON_VERTEX_WORDS");
                uint8_t vertex=combatGeometry.polygonVertexIndices[index];
                combatPolygons.screenPolygonX[index]=(int16_t)((uint16_t)combatRender.screenCentreA+combatGeometry.geometryProjectedX.words[vertex]);
                combatPolygons.screenPolygonY[index]=(int16_t)((uint16_t)combatRender.screenCentreB-combatGeometry.geometryProjectedY.words[vertex]);
            }
        }
        combatVideo.screenPolygonNeedsClip=combatRender.objectNeedsScreenClip;
        BtechDrawScreenPolygon((int16_t)combatPolygons.polygonVertexCount,combatPolygons.screenPolygonX,combatPolygons.screenPolygonY);
    }
}
