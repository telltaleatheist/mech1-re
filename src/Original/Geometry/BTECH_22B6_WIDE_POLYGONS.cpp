#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_render.h"
#include <stdlib.h>

/*22B6:6733. Signed DWORD product returned as native DX:AX:CX:BX. The host
 * uint64_t transports all four WORDs, not an invented native return ABI. */
uint64_t BtechMultiplySignedDwordsWide(uint32_t first,uint32_t second)
{
    return (uint64_t)((int64_t)(int32_t)first*(int32_t)second);
}

/*22B6:2744. Original two forward REP MOVSWs, X before Y. */
void BtechCopyRendererWorkingCoordinateArrays(void)
{
    uint16_t count=(uint16_t)(combatPolygons.polygonVertexCount*2)>>1; /* Native WORD count wraps before REP. */
    if (count>WIDE_POLYGON_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: count>WIDE_POLYGON_VERTICES");
    for (uint16_t vertex=0;vertex<count;++vertex) combatGeometry.wideScreenX[vertex]=combatGeometry.widePolygonWorkX[vertex];
    for (uint16_t vertex=0;vertex<count;++vertex) combatGeometry.wideScreenY[vertex]=combatGeometry.widePolygonWorkY[vertex];
}

/*22B6:67E0. Do/while is native: a direct zero-count call still emits vertex0. */
void BtechOffsetWidePolygonVerticesToScreen(void)
{
    uint32_t centreX=(uint32_t)combatRender.screenCentreA,centreY=(uint32_t)combatRender.screenCentreB;
    uint16_t destination=0;
    do
    {
        if (destination>=WIDE_POLYGON_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: destination>=WIDE_POLYGON_VERTICES");
        uint8_t vertex=combatGeometry.polygonVertexIndices[destination];
        if (vertex>=GEOMETRY_CACHE_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: vertex>=GEOMETRY_CACHE_VERTICES");
        combatGeometry.wideScreenX[destination]=centreX+combatGeometry.geometryProjectedX.wide[vertex];
        combatGeometry.wideScreenY[destination]=centreY-combatGeometry.geometryProjectedY.wide[vertex];
        ++destination;
    } while ((int16_t)destination<(int16_t)combatPolygons.polygonVertexCount);
}

/*22B6:6836. Actual wide polygon orchestration, not a replacement renderer. */
void BtechRenderWidePolygonPayload(const BtechRenderPolygonPayload *payload)
{
    combatRender.widePolygonsRemaining=(uint16_t)payload->polygonCount;
    const BtechRenderPolygon *polygon=payload->polygons;
    for (;;++polygon)
    {
        if ((int16_t)combatRender.widePolygonsRemaining--<=0) return; /* DEC/JGE, including overflow. */
        uint8_t flags=polygon->flags;
        combatVideo.drawPolygonFilled=(flags&POLYGON_FILLED)!=0;
        if ((flags&POLYGON_FACING_MASK)==2) continue;
        if ((flags&POLYGON_FACING_MASK)!=0)
        {
            uint64_t dot=BtechGetWidePolygonFacingDotHighWord(polygon->normalVectorIndex,polygon->vertexIndices[0]);
            if ((int16_t)(dot>>32)>=0) continue;
        }
        BtechSelectPolygonColoursAndShade(polygon);
        combatRender.polygonHasNearClippedVertex=0;
        combatPolygons.polygonVertexCount=0;
        for (const uint8_t *indices=polygon->vertexIndices;*indices!=POLYGON_VERTEX_LIST_END;++indices)
        {
            uint8_t vertex=*indices;
            BtechCacheTransformedGeometryVector(vertex);
            if (combatGeometryTables.vertexNearPlaneState[vertex]!=GEOMETRY_VERTEX_PROJECTED)
            {
                uint32_t depth=combatGeometry.geometryViewDepth.wide[vertex];
                if ((int16_t)(depth>>16)<0 || (!(depth>>16) && (uint16_t)depth<combatRender.nearClipDepth))
                {
                    combatGeometryTables.vertexNearPlaneState[vertex]=GEOMETRY_VERTEX_BEHIND;
                    combatRender.polygonHasNearClippedVertex=GEOMETRY_VERTEX_BEHIND;
                }
                else
                {
                    combatGeometryTables.vertexNearPlaneState[vertex]=GEOMETRY_VERTEX_PROJECTED;
                    BtechGeometryCoordinateCache *source[]={&combatGeometry.geometryViewX,&combatGeometry.geometryViewZ};
                    BtechGeometryCoordinateCache *destination[]={&combatGeometry.geometryProjectedX,&combatGeometry.geometryProjectedY};
                    for (uint16_t axis=0;axis<2;++axis)
                    {
                        uint32_t value=source[axis]->wide[vertex];
                        uint16_t low=(uint16_t)value,middle=(uint16_t)(value>>16),high=(int16_t)middle<0?UINT16_MAX:0;
                        depth=combatGeometry.geometryViewDepth.wide[vertex];
                        uint16_t shifts=combatRender.combatProjectionShift;
                        if ((int8_t)(uint8_t)shifts>=8)
                        {
                            high=(high&0xFF00)|(middle>>8);middle=(uint16_t)((middle<<8)|(low>>8));low<<=8;
                            shifts=(shifts&0xFF00)|(uint8_t)((uint8_t)shifts-8);
                            if (!(uint8_t)shifts) goto divideCoordinate;
                        }
                        do
                        {high=(uint16_t)((high<<1)|(middle>>15));middle=(uint16_t)((middle<<1)|(low>>15));low<<=1;} while (--shifts);
divideCoordinate:
                        destination[axis]->wide[vertex]=BtechDivideSignedWideNumerator((int16_t)high<0?-1:0,high,middle,low,(uint16_t)depth,(int16_t)(depth>>16),NULL);
                    }
                }
            }
            if (combatPolygons.polygonVertexCount>=POLYGON_VERTEX_INDEX_BYTES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: polygonVertexCount>=POLYGON_VERTEX_INDEX_BYTES");
            combatGeometry.polygonVertexIndices[combatPolygons.polygonVertexCount++]=vertex;
        }
        if (combatRender.polygonHasNearClippedVertex)
        {
            if (combatRender.useWordProductClipping) BtechClipDwordNearPolygonWithWordProducts();
            else BtechClipWidePolygonNearPlane();
        }
        else
        {
            if (!combatPolygons.polygonVertexCount) continue;
            BtechOffsetWidePolygonVerticesToScreen();
        }
        if (combatRender.objectNeedsScreenClip)
        {
            if (combatRender.useWordProductClipping) BtechClipPolygonWithWordProducts();
            else BtechClipWidePolygonToScreen();
        }
        for (uint16_t vertex=0;vertex<combatPolygons.polygonVertexCount;++vertex)
        {
            if (vertex>=COMBAT_POLYGON_VERTEX_WORDS || vertex>=WIDE_POLYGON_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: vertex>=COMBAT_POLYGON_VERTEX_WORDS || vertex>=WIDE_POLYGON_VERTICES");
            combatPolygons.screenPolygonX[vertex]=(int16_t)combatGeometry.wideScreenX[vertex];
            combatPolygons.screenPolygonY[vertex]=(int16_t)combatGeometry.wideScreenY[vertex];
        }
        combatVideo.screenPolygonNeedsClip=false;
        BtechDrawScreenPolygon((int16_t)combatPolygons.polygonVertexCount,combatPolygons.screenPolygonX,combatPolygons.screenPolygonY);
    }
}
