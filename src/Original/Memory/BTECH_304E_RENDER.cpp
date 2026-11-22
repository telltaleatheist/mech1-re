#include "mechwarrior/combat_render.h"
#include "mechwarrior/native_address.h"
#include <string.h>

BtechCameraContext *activeCameraContext;
BtechPosition3D cameraReferencePosition,cameraRelativePosition,transformedViewPosition,projectionVectorScratch;
BtechProjectionRatio projectionRatio;
BtechOrientationBasis viewTransformBasis;
BtechOrientationBasis composedTransformBasis;
uint16_t cameraViewOrientation[3],transformedReferenceVector[3],viewVectorCoefficients[3];
uint16_t combatProjectionShift,nearClipDepth,combatProjectionUnit,objectProjectionScale;
uint32_t resourceScaledExtent,cameraDepthScratch;
uint8_t forceObjectScreenClip,projectionMode10,mirrorProjectionA,objectNeedsScreenClip,useViewBasisDirectly;
int16_t screenCentreA,screenCentreB;
const uint8_t *activeColourShadeClasses,*activePaletteRemap;
BtechOrientationBasis *activeRenderBasis;
uint16_t dotProductScratchHigh,dotProductScratchMiddle,dotProductScratchLow;
BtechDwordDivisionResult signedDivisionResult;
uint16_t selectionCount;
uint16_t selectionPointScreenX,selectionPointScreenY;
uint16_t selectionObjectListIndices[SELECTION_LIST_PREFIX];
uint32_t selectionMarkerPixels,selectionCapturedPixels,selectionPointBackground;
int16_t horizonPolygonX[4],horizonPolygonY[4];
uint8_t targetSelectionProbeActive;
const BtechShapeResource *activeRenderHeader;
const BtechRenderEntry *activeRenderEntry,*orderedRenderEntries[RENDER_ENTRY_PREFIX_CAPACITY];
const BtechRenderPrimitive *activePrimitiveRecord;
const BtechRenderVariant *activeVariant;
const uint16_t (*activeEntryGeometry)[3],(*renderGeometry)[3];
uint32_t renderEntryOrderingKeys[RENDER_ENTRY_PREFIX_CAPACITY];
uint8_t renderEntryVariants[RENDER_ENTRY_PREFIX_CAPACITY];
uint16_t resourceReferenceVector[3],transformedEntryVector[3];
uint16_t renderTransformedReference[3];
uint16_t projectionScaleMultiplier,renderGeometryVectorCount,resourceScaleShift;
uint8_t widePolygonCoordinates,useWordProductClipping,skipWordPolygonRenderer;
uint8_t polygonShadeClass;
uint8_t polygonHasNearClippedVertex;
uint16_t wordPolygonsRemaining;
uint8_t wordPolygonHasNearOutsideVertex;
uint16_t widePolygonsRemaining;

/* Fresh executable/BSS IO only. Native pointer owners are zero in this image;
 * runtime camera/list code assigns actual typed references, never a registry. */
bool BtechImportRenderInitialData(const uint8_t *data,size_t bytes)
{
    if (!data || bytes<0x10000 || MwReadNativeWord(data,0xC944) || MwReadNativeWord(data,0xC332)
        || MwReadNativeWord(data,0xC34A) || MwReadNativeWord(data,0xC8F8)) return false;
    activeCameraContext=NULL;activeRenderBasis=NULL;
    polygonShadeClass=data[0x2E34];
    wordPolygonsRemaining=0; /*2E14*/wordPolygonHasNearOutsideVertex=0; /*2E16*/
    polygonHasNearClippedVertex=0; /*2E38*/widePolygonsRemaining=0; /*2E36*/
    activeRenderHeader=NULL;activeRenderEntry=NULL;activePrimitiveRecord=NULL;activeVariant=NULL;
    activeEntryGeometry=NULL;renderGeometry=NULL;
    for (uint16_t i=0;i<RENDER_ENTRY_PREFIX_CAPACITY;++i)
    {renderEntryOrderingKeys[i]=0;renderEntryVariants[i]=0;orderedRenderEntries[i]=NULL;}
    for (uint16_t i=0;i<3;++i) {resourceReferenceVector[i]=0;transformedEntryVector[i]=0;}
    for (uint16_t i=0;i<3;++i) renderTransformedReference[i]=0; /*775A*/
    projectionScaleMultiplier=MwReadNativeWord(data,0x2CE6);renderGeometryVectorCount=0;
    resourceScaleShift=0;widePolygonCoordinates=0;useWordProductClipping=0;skipWordPolygonRenderer=0;
    activeColourShadeClasses=NULL;activePaletteRemap=NULL;
    cameraReferencePosition=BtechPosition3D{};cameraRelativePosition=BtechPosition3D{};
    transformedViewPosition=BtechPosition3D{};projectionVectorScratch=BtechPosition3D{};
    projectionRatio=BtechProjectionRatio{};viewTransformBasis=BtechOrientationBasis{};
    composedTransformBasis=BtechOrientationBasis{};
    for (uint16_t axis=0;axis<3;++axis)
    {cameraViewOrientation[axis]=0;transformedReferenceVector[axis]=0;viewVectorCoefficients[axis]=0;}
    combatProjectionShift=0; /*C356*/nearClipDepth=0; /*C12E*/
    combatProjectionUnit=0; /*D1EA*/objectProjectionScale=0; /*C8DA*/
    resourceScaledExtent=0;cameraDepthScratch=0;
    forceObjectScreenClip=data[0xC953];projectionMode10=data[0xC358];mirrorProjectionA=data[0x798D];
    objectNeedsScreenClip=data[0x798C];useViewBasisDirectly=data[0xC94A];
    screenCentreA=0; /*CB16*/screenCentreB=0; /*CB18*/
    dotProductScratchHigh=0; /*2E0E*/dotProductScratchMiddle=0; /*2E10*/dotProductScratchLow=0; /*2E12*/
    signedDivisionResult=BtechDwordDivisionResult{}; /*4BA0*/
    selectionCount=0; /*6F58*/targetSelectionProbeActive=0; /*281A*/
    selectionPointScreenX=0; /*6F54*/selectionPointScreenY=0; /*6F56*/
    selectionMarkerPixels=MwReadNativeWord(data,0x2816)|((uint32_t)MwReadNativeWord(data,0x2818)<<16);
    selectionCapturedPixels=0; /*6F6C*/
    selectionPointBackground=0; /*6F70*/
    memset(selectionObjectListIndices,0,sizeof(selectionObjectListIndices)); /* 6F5A, past the file data. */
    for (uint16_t i=0;i<4;++i) {horizonPolygonX[i]=0; /*2E18*/horizonPolygonY[i]=0; /*2E20*/}
    return true;
}
