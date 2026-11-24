#include "mechwarrior/combat_polygons.h"
#include "mechwarrior/btech_start_image.h"

CombatPolygonState combatPolygons;

/* Executable IO projection only. Runtime uses these actual named owners. */
bool BtechImportPolygonInitialData(void)
{
    const BtechStartRowData polygonCoordinatesRow("polygon.coordinates");
    const BtechStartRowData polygonScanlineHeaderRow("polygon.scanline_header");
    const BtechStartRowData polygonEgaMasksRow("polygon.ega_masks");
    if (polygonScanlineHeaderRow.word()) return false;
    combatPolygons.polygonScanlineHeader=NULL;combatPolygons.graphicsFillPolygonSpans=NULL;
    combatPolygons.graphicsDrawLine=NULL;
    int16_t *coordinates[]={combatPolygons.screenPolygonX,combatPolygons.screenPolygonY,combatPolygons.polygonWorkX,combatPolygons.polygonWorkY,combatPolygons.polygonOutlineX,combatPolygons.polygonOutlineY};
    for (uint16_t axis=0;axis<6;++axis)
        for (uint16_t i=0;i<COMBAT_POLYGON_VERTEX_WORDS;++i) coordinates[axis][i]=(int16_t)polygonCoordinatesRow.word(axis*0x28+i*2);
    combatPolygons.polygonVertexCount=0; /*2FEC*/
    for (uint16_t i=0;i<=COMBAT_SCREEN_HEIGHT;++i)
    {combatPolygons.polygonSpanStorage[i].endpoints.left=0; /*2FFC*/combatPolygons.polygonSpanStorage[i].endpoints.right=0; /*2FFE*/}
    combatPolygons.polygonMinimumYVertexByteOffset=0; /*3FD0*/combatPolygons.polygonMaximumYVertexByteOffset=0; /*3FD2*/
    combatPolygons.polygonFirstChainVertexCount=0; /*3FD4*/combatPolygons.polygonSecondChainVertexCount=0; /*3FD6*/
    combatPolygons.polygonChainEdgesRemaining=0; /*3FD8*/combatPolygons.polygonChainVertexByteOffset=0; /*3FDA*/
    combatPolygons.polygonSpanBoundaryByteOffset=0; /*3FDC*/combatPolygons.polygonPreviousVertexX=0; /*3FDE*/
    combatPolygons.polygonPreviousVertexY=0; /*3FE0*/combatPolygons.polygonOutlineVertexCount=0; /*3FE4*/
    for (uint16_t i=0;i<8;++i) {combatPolygons.egaPolygonLeadingMasks[i]=polygonEgaMasksRow.byte(i);combatPolygons.egaPolygonTrailingMasks[i]=polygonEgaMasksRow.byte(8+i);}
    combatPolygons.egaSpanHighColour=0; /*44A4*/combatPolygons.egaSpanLowColour=0; /*44A5*/combatPolygons.egaSpanRowsRemaining=0; /*44A6*/
    return true;
}
