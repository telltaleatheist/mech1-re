#include "mechwarrior/combat_render.h"

/*0810:D93E. Native signed extent/coordinate tests, wrapped absolute values,
 * arithmetic shifts and reference-vector staging. No host floating point. */
void BtechPrepareResourceCoordinatePaths(void)
{
    if (combatRender.projectionScaleMultiplier!=0x100)
    {
        uint32_t product=(int32_t)(int16_t)combatRender.projectionScaleMultiplier*(int16_t)combatRender.objectProjectionScale;
        for (uint8_t shifts=8;shifts;--shifts) product=(product>>1)|(product&0x80000000);
        combatRender.objectProjectionScale=(int32_t)product<0?0:(int32_t)product>0x100?0x100:(uint16_t)product;
    }
    combatRender.resourceScaleShift=combatRender.activeRenderHeader->scaleShift;
    uint32_t magnitude=combatRender.transformedViewPosition.positionA;
    if ((int32_t)magnitude<0) magnitude=0-magnitude;
    combatRender.projectionVectorScratch.positionA=magnitude;
    magnitude=combatRender.transformedViewPosition.positionB;
    if ((int32_t)magnitude<0) magnitude=0-magnitude;
    combatRender.projectionVectorScratch.positionB=magnitude;
    magnitude=combatRender.transformedViewPosition.positionC;
    if ((int32_t)magnitude<0) magnitude=0-magnitude;
    combatRender.projectionVectorScratch.positionC=magnitude;
    uint32_t maximumMagnitude=combatRender.projectionVectorScratch.positionA;
    if ((int32_t)maximumMagnitude<(int32_t)combatRender.projectionVectorScratch.positionB)
        maximumMagnitude=combatRender.projectionVectorScratch.positionB;
    if ((int32_t)maximumMagnitude<(int32_t)combatRender.projectionVectorScratch.positionC)
        maximumMagnitude=combatRender.projectionVectorScratch.positionC;
    uint32_t maximumPlusExtent=maximumMagnitude+combatRender.resourceScaledExtent;
    combatRender.widePolygonCoordinates=combatRender.resourceScaleShift!=0 || (int32_t)combatRender.resourceScaledExtent>0x3FFC
        || (int32_t)maximumPlusExtent>0x7FBC;
    combatRender.useWordProductClipping=(int32_t)combatRender.resourceScaledExtent<0x3FAC;
    uint16_t coordinateShift=0;
    if (combatRender.widePolygonCoordinates && (int32_t)maximumMagnitude>0x3FFF)
        while ((int32_t)maximumMagnitude>0x3FFF)
        {maximumMagnitude=(maximumMagnitude>>1)|(maximumMagnitude&0x80000000);++coordinateShift;}
    uint16_t *reference=combatRender.useViewBasisDirectly?combatRender.resourceReferenceVector:combatRender.viewVectorCoefficients;
    for (uint16_t component=0;component<3;++component) reference[component]=combatRender.activeRenderHeader->referenceVector[component];
    const BtechPosition3D *position=&combatRender.cameraRelativePosition;
    if (!combatRender.useViewBasisDirectly)
    {
        BtechTransformWordVectorFar(combatRender.viewVectorCoefficients,combatRender.activeRenderBasis,combatRender.resourceReferenceVector);
        position=&combatRender.transformedViewPosition;
    }
    const uint32_t *components[]={&position->positionA,&position->positionB,&position->positionC};
    for (uint16_t component=0;component<3;++component)
    {
        uint32_t value=*components[component];
        for (uint8_t shifts=coordinateShift;shifts;--shifts) value=(value>>1)|(value&0x80000000);
        combatRender.resourceReferenceVector[component]+=(uint16_t)value;
    }
    combatRender.skipWordPolygonRenderer=0;
}
