#include "mechwarrior/combat_render.h"
#include "mechwarrior/btech_start_image.h"
#include <string.h>

CombatRenderState combatRender;

/* Fresh executable/BSS IO only. Native pointer owners are zero in this image;
 * runtime camera/list code assigns actual typed references, never a registry. */
bool BtechImportRenderInitialData(void)
{
    const BtechStartRowData renderSelectionMarkerPixelsRow("render.selection_marker_pixels");
    const BtechStartRowData renderProjectionScaleRow("render.projection_scale");
    const BtechStartRowData renderPolygonShadeClassRow("render.polygon_shade_class");
    combatRender.activeCameraContext=NULL;combatRender.activeRenderBasis=NULL;
    combatRender.polygonShadeClass=renderPolygonShadeClassRow.byte();
    combatRender.wordPolygonsRemaining=0; /*2E14*/combatRender.wordPolygonHasNearOutsideVertex=0; /*2E16*/
    combatRender.polygonHasNearClippedVertex=0; /*2E38*/combatRender.widePolygonsRemaining=0; /*2E36*/
    combatRender.activeRenderHeader=NULL;combatRender.activeRenderEntry=NULL;combatRender.activePrimitiveRecord=NULL;combatRender.activeVariant=NULL;
    combatRender.activeEntryGeometry=NULL;combatRender.renderGeometry=NULL;
    for (uint16_t i=0;i<RENDER_ENTRY_PREFIX_CAPACITY;++i)
    {combatRender.renderEntryOrderingKeys[i]=0;combatRender.renderEntryVariants[i]=0;combatRender.orderedRenderEntries[i]=NULL;}
    for (uint16_t i=0;i<3;++i) {combatRender.resourceReferenceVector[i]=0;combatRender.transformedEntryVector[i]=0;}
    for (uint16_t i=0;i<3;++i) combatRender.renderTransformedReference[i]=0; /*775A*/
    combatRender.projectionScaleMultiplier=renderProjectionScaleRow.word();combatRender.renderGeometryVectorCount=0;
    combatRender.resourceScaleShift=0;combatRender.widePolygonCoordinates=0;combatRender.useWordProductClipping=0;combatRender.skipWordPolygonRenderer=0;
    combatRender.activeColourShadeClasses=NULL;combatRender.activePaletteRemap=NULL;
    combatRender.cameraReferencePosition=BtechPosition3D{};combatRender.cameraRelativePosition=BtechPosition3D{};
    combatRender.transformedViewPosition=BtechPosition3D{};combatRender.projectionVectorScratch=BtechPosition3D{};
    combatRender.projectionRatio=BtechProjectionRatio{};combatRender.viewTransformBasis=BtechOrientationBasis{};
    combatRender.composedTransformBasis=BtechOrientationBasis{};
    for (uint16_t axis=0;axis<3;++axis)
    {combatRender.cameraViewOrientation[axis]=0;combatRender.transformedReferenceVector[axis]=0;combatRender.viewVectorCoefficients[axis]=0;}
    combatRender.combatProjectionShift=0; /*C356*/combatRender.nearClipDepth=0; /*C12E*/
    combatRender.combatProjectionUnit=0; /*D1EA*/combatRender.objectProjectionScale=0; /*C8DA*/
    combatRender.resourceScaledExtent=0;combatRender.cameraDepthScratch=0;
    combatRender.forceObjectScreenClip=0; /*C953*/combatRender.projectionMode10=0; /*C358*/combatRender.mirrorProjectionA=0; /*798D*/
    combatRender.objectNeedsScreenClip=0; /*798C*/combatRender.useViewBasisDirectly=0; /*C94A*/
    combatRender.screenCentreA=0; /*CB16*/combatRender.screenCentreB=0; /*CB18*/
    combatRender.dotProductScratchHigh=0; /*2E0E*/combatRender.dotProductScratchMiddle=0; /*2E10*/combatRender.dotProductScratchLow=0; /*2E12*/
    combatRender.selectionCount=0; /*6F58*/combatRender.targetSelectionProbeActive=0; /*281A*/
    combatRender.selectionPointScreenX=0; /*6F54*/combatRender.selectionPointScreenY=0; /*6F56*/
    combatRender.selectionMarkerPixels=renderSelectionMarkerPixelsRow.word()|((uint32_t)renderSelectionMarkerPixelsRow.word(2)<<16);
    combatRender.selectionCapturedPixels=0; /*6F6C*/
    combatRender.selectionPointBackground=0; /*6F70*/
    memset(combatRender.selectionObjectListIndices,0,sizeof(combatRender.selectionObjectListIndices)); /* 6F5A, past the file data. */
    for (uint16_t i=0;i<4;++i) {combatRender.horizonPolygonX[i]=0; /*2E18*/combatRender.horizonPolygonY[i]=0; /*2E20*/}
    return true;
}
