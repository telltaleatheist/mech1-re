#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_render.h"
#include <stdlib.h>

/*22B6:02D2. Clear ONLY the occupied flags BYTE. Camera object/pose stay stale. */
void BtechReleaseCameraContextFar(BtechCameraContext *camera)
{
    if (!camera) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !camera"); /* Unrepresented native near-zero corruption. */
    camera->flags=0;
}

/*22B6:07FA/0820. Original folded Q14 table, WORD angle wrap before indexing. */
uint16_t BtechLookupSine(uint16_t angle)
{
    uint16_t index=(uint16_t)(angle+0xC000)>>4;
    if (index&0x0800) index=(uint16_t)(0x1000-index);
    return orientationCosines[index];
}

uint16_t BtechLookupCosine(uint16_t angle)
{
    uint16_t index=angle>>4;
    if (index&0x0800) index=(uint16_t)(0x1000-index);
    return orientationCosines[index];
}

/*22B6:0008. Typed camera owners and actual relative/view basis composition.
 * EGA horizon rectangles and tilted polygon regions use their native raster paths. */
void BtechPrepareCameraContext(BtechCameraContext *camera)
{
    combatRender.activeCameraContext=camera;
    combatRender.cameraReferencePosition.positionA=camera->viewObject->position.positionA;
    combatRender.cameraReferencePosition.positionB=camera->viewObject->position.positionB;
    combatRender.cameraReferencePosition.positionC=camera->viewObject->position.positionC;
    uint16_t builderCx=BtechBuildOrientationMatrix(camera->relativeOrientation,&cameraOrientationBasis,0); /* REP MOVSW exhausted CX. */
    BtechWorldObjectPrefix *viewObject=combatRender.activeCameraContext->viewObject;
    if (cameraOrientationBasis.mode)
    {
        uint16_t orientation[]={viewObject->orientationA,viewObject->orientationB,viewObject->orientationC};
        builderCx=BtechBuildOrientationMatrix(orientation,&combatRender.composedTransformBasis,builderCx);
        BtechComposeTransformBases(builderCx,&cameraOrientationBasis,&combatRender.composedTransformBasis,&combatRender.viewTransformBasis);
        BtechExtractOrientationFromBasis(&combatRender.viewTransformBasis,combatRender.cameraViewOrientation);
    }
    else
    {
        combatRender.cameraViewOrientation[0]=viewObject->orientationA;
        combatRender.cameraViewOrientation[1]=viewObject->orientationB;
        combatRender.cameraViewOrientation[2]=viewObject->orientationC;
        BtechBuildOrientationMatrix(combatRender.cameraViewOrientation,&combatRender.viewTransformBasis,builderCx);
    }
    BtechTransposeTransformBasis(&combatRender.viewTransformBasis);
    combatRender.forceObjectScreenClip=0;
    combatRender.projectionMode10=camera->flags&CAMERA_PROJECTION_MODE_10;
    combatRender.mirrorProjectionA=camera->flags&CAMERA_MIRROR_HORIZONTAL;
    if (combatRender.projectionMode10 || combatRender.mirrorProjectionA || (combatRender.activeCameraContext->flags&CAMERA_KEEP_SCREEN_CENTRE))
        combatRender.forceObjectScreenClip=1;
    combatRender.combatProjectionShift=camera->projectionShift;
    uint16_t projectionUnit=1;
    for (uint8_t shifts=combatRender.combatProjectionShift;shifts;--shifts) projectionUnit=(uint16_t)(projectionUnit<<1);
    combatRender.combatProjectionUnit=projectionUnit;
    combatRender.nearClipDepth=projectionUnit;
    clipMinimumX=camera->clipMinimumA;
    clipMaximumX=camera->clipMaximumA;
    clipMinimumY=camera->clipMinimumB;
    clipMaximumY=camera->clipMaximumB;
    BtechTransformWordVectorFar(cameraReferenceVector,&combatRender.viewTransformBasis,combatRender.transformedReferenceVector);
    combatRender.activeColourShadeClasses=camera->colourShadeClasses;
    combatRender.activePaletteRemap=camera->paletteRemap;
    if (!(camera->flags&CAMERA_KEEP_SCREEN_CENTRE))
    {
        uint16_t difference=clipMaximumX-clipMinimumX;
        combatRender.screenCentreA=(int16_t)(clipMinimumX+((difference>>1)|(difference&0x8000)));
        difference=(uint16_t)(clipMaximumY-clipMinimumY);
        combatRender.screenCentreB=(int16_t)(clipMinimumY+((difference>>1)|(difference&0x8000)));
    }
    if (!(camera->flags&CAMERA_NO_BACKGROUND))
    {
        uint8_t fillA=combatRender.activeCameraContext->fillPaletteA;
        if (combatRender.activePaletteRemap) fillA=combatRender.activePaletteRemap[fillA];
        uint8_t fillB=combatRender.activeCameraContext->fillPaletteB;
        if (combatRender.activePaletteRemap) fillB=combatRender.activePaletteRemap[fillB];
        if ((camera->flags&CAMERA_GRADIENT_BACKGROUND) && graphicsColourMode) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (camera->flags&CAMERA_GRADIENT_BACKGROUND) && graphicsColourMode"); /*2D03 alternate adapter. */
        BtechFillTiltedCameraHorizonRegions(fillA,fillB,combatRender.cameraViewOrientation[0],combatRender.cameraViewOrientation[1]);
    }
}

/*22B6:2EEE. Native culling and basis selection. Stops before the unconverted
 * resource primitives; hidden/near/outside returns really return. */
void BtechCullAndRenderWorldObject(BtechHeapWorldObject *object)
{
    if (object->prefix.renderFlags&OBJECT_HIDDEN) return;
    uint32_t farthestDepth=combatRender.transformedViewPosition.positionB+combatRender.resourceScaledExtent;
    uint16_t builderCx=farthestDepth; /*2F00/2F08, retained on forced-clip branch. */
    if ((int32_t)farthestDepth<(int16_t)combatRender.nearClipDepth || !combatRender.objectProjectionScale) return;
    if (combatRender.forceObjectScreenClip) combatRender.objectNeedsScreenClip=1;
    else
    {
        for (uint16_t phase=0;phase<2;++phase)
        {
            uint32_t magnitude=combatRender.transformedViewPosition.positionA;
            if ((int32_t)magnitude<0) magnitude=0-magnitude;
            combatRender.projectionRatio.ratio.numerator=phase?magnitude+combatRender.resourceScaledExtent:magnitude-combatRender.resourceScaledExtent;
            combatRender.projectionRatio.ratio.denominator=phase?combatRender.transformedViewPosition.positionB-combatRender.resourceScaledExtent
                :combatRender.transformedViewPosition.positionB+combatRender.resourceScaledExtent;
            uint32_t nearDepth=(int16_t)combatRender.nearClipDepth;
            if (phase && (int32_t)nearDepth>(int32_t)combatRender.projectionRatio.ratio.denominator)
                combatRender.projectionRatio.ratio.denominator=nearDepth;
            magnitude=combatRender.transformedViewPosition.positionC;
            if ((int32_t)magnitude<0) magnitude=0-magnitude;
            combatRender.projectionRatio.ratio.third=phase?magnitude+combatRender.resourceScaledExtent:magnitude-combatRender.resourceScaledExtent;
            builderCx=BtechProjectViewPosition(&combatRender.projectionRatio.vector,&combatRender.projectionVectorScratch);
            BtechOffsetProjectedScreenPair(&combatRender.projectionVectorScratch);
            uint8_t outside=clipMaximumX<(int32_t)combatRender.projectionVectorScratch.positionA
                || clipMinimumY>(int32_t)combatRender.projectionVectorScratch.positionB;
            if (!phase && outside) return;
            if (phase) combatRender.objectNeedsScreenClip=outside;
        }
    }
    combatRender.useViewBasisDirectly=1;
    combatRender.activeRenderBasis=&combatRender.viewTransformBasis;
    if (!(object->prefix.renderFlags&OBJECT_SKIP_ORIENTATION))
    {
        uint16_t orientation[]={object->prefix.orientationA,object->prefix.orientationB,object->prefix.orientationC};
        builderCx=BtechBuildOrientationMatrix(orientation,&cameraOrientationBasis,builderCx);
        if (cameraOrientationBasis.mode)
        {
            BtechComposeTransformBases(builderCx,&cameraOrientationBasis,&combatRender.viewTransformBasis,&combatRender.composedTransformBasis);
            combatRender.useViewBasisDirectly=0;
            combatRender.activeRenderBasis=&combatRender.composedTransformBasis;
        }
    }
    BtechPrepareRenderResourceEntries(object->prefix.resourceId,object->entryVariants);
}
