#include "mechwarrior/combat_polygons.h"
#include "mechwarrior/native_address.h"

int16_t screenPolygonX[COMBAT_POLYGON_VERTEX_WORDS],screenPolygonY[COMBAT_POLYGON_VERTEX_WORDS];
int16_t polygonWorkX[COMBAT_POLYGON_VERTEX_WORDS],polygonWorkY[COMBAT_POLYGON_VERTEX_WORDS];
int16_t polygonOutlineX[COMBAT_POLYGON_VERTEX_WORDS],polygonOutlineY[COMBAT_POLYGON_VERTEX_WORDS];
uint16_t polygonVertexCount;
BtechPolygonSpan polygonSpanStorage[COMBAT_SCREEN_HEIGHT+1];
BtechPolygonSpan *polygonScanlineHeader;
uint16_t polygonMinimumYVertexByteOffset,polygonMaximumYVertexByteOffset;
uint16_t polygonFirstChainVertexCount,polygonSecondChainVertexCount;
uint16_t polygonChainEdgesRemaining,polygonChainVertexByteOffset,polygonSpanBoundaryByteOffset;
uint16_t polygonPreviousVertexX,polygonPreviousVertexY,polygonOutlineVertexCount;
uint8_t egaPolygonLeadingMasks[8],egaPolygonTrailingMasks[8],egaSpanHighColour,egaSpanLowColour;
uint16_t egaSpanRowsRemaining;
void (*graphicsFillPolygonSpans)(const BtechPolygonSpan *);
void (*graphicsDrawLine)(uint16_t,uint16_t,uint16_t,uint16_t,uint16_t);

/* Executable IO projection only. Runtime uses these actual named owners. */
bool BtechImportPolygonInitialData(const uint8_t *data,size_t bytes)
{
    if (!data || bytes<0x10000 || MwReadNativeWord(data,0x3FE2)) return false;
    polygonScanlineHeader=NULL;graphicsFillPolygonSpans=NULL;
    graphicsDrawLine=NULL;
    int16_t *coordinates[]={screenPolygonX,screenPolygonY,polygonWorkX,polygonWorkY,polygonOutlineX,polygonOutlineY};
    for (uint16_t axis=0;axis<6;++axis)
        for (uint16_t i=0;i<COMBAT_POLYGON_VERTEX_WORDS;++i) coordinates[axis][i]=(int16_t)MwReadNativeWord(data,0x2EFC+axis*0x28+i*2);
    polygonVertexCount=0; /*2FEC*/
    for (uint16_t i=0;i<=COMBAT_SCREEN_HEIGHT;++i)
    {polygonSpanStorage[i].endpoints.left=0; /*2FFC*/polygonSpanStorage[i].endpoints.right=0; /*2FFE*/}
    polygonMinimumYVertexByteOffset=0; /*3FD0*/polygonMaximumYVertexByteOffset=0; /*3FD2*/
    polygonFirstChainVertexCount=0; /*3FD4*/polygonSecondChainVertexCount=0; /*3FD6*/
    polygonChainEdgesRemaining=0; /*3FD8*/polygonChainVertexByteOffset=0; /*3FDA*/
    polygonSpanBoundaryByteOffset=0; /*3FDC*/polygonPreviousVertexX=0; /*3FDE*/
    polygonPreviousVertexY=0; /*3FE0*/polygonOutlineVertexCount=0; /*3FE4*/
    for (uint16_t i=0;i<8;++i) {egaPolygonLeadingMasks[i]=data[0x4494+i];egaPolygonTrailingMasks[i]=data[0x449C+i];}
    egaSpanHighColour=0; /*44A4*/egaSpanLowColour=0; /*44A5*/egaSpanRowsRemaining=0; /*44A6*/
    return true;
}
