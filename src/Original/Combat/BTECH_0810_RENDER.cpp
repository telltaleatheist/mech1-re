#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_scene.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_import.h"
#include <stdlib.h>

/*0810:0985. Genuine FAR veneer into the NEAR CF5C entry. */
void BtechRenderObjectListVeneer(BtechCameraContext *camera,BtechWorldObjectPrefix *const *objects,int16_t count)
{
    BtechRenderObjectList(camera,objects,count);
}

/*0810:CF5C. Three native arguments. Incidental preserved DI is not a return
 * value. List/callback fields remain live across every native callee. */
void BtechRenderObjectList(BtechCameraContext *camera,BtechWorldObjectPrefix *const *objects,int16_t count)
{
    BtechPrepareCameraContext(camera);
    if (combatWorld.beforeObjectListCallbackOffset)
    {
        if (combatWorld.beforeObjectListCallbackOffset!=WORLD_LIST_BEFORE_CALLBACK) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: beforeObjectListCallbackOffset!=WORLD_LIST_BEFORE_CALLBACK");
        BtechPrepareSelectionPixels(objects,count);
    }
    for (int16_t ordinal=0;ordinal<count;++ordinal)
    {
        BtechHeapWorldObject *object=(BtechHeapWorldObject *)objects[ordinal];
        if (&object->prefix==combatRender.activeCameraContext->viewObject) continue;
        combatRender.cameraRelativePosition.positionA=object->prefix.position.positionA-combatRender.cameraReferencePosition.positionA;
        combatRender.cameraRelativePosition.positionB=object->prefix.position.positionB-combatRender.cameraReferencePosition.positionB;
        combatRender.cameraRelativePosition.positionC=object->prefix.position.positionC-combatRender.cameraReferencePosition.positionC;
        BtechTransformViewVector(&combatRender.cameraRelativePosition,&combatRender.viewTransformBasis,&combatRender.transformedViewPosition);
        combatRender.cameraDepthScratch=combatRender.transformedViewPosition.positionB;
        combatRender.resourceScaledExtent=object->prefix.resourceScaledValue;
        combatRender.projectionRatio.ratio.numerator=combatRender.resourceScaledExtent;
        combatRender.projectionRatio.ratio.denominator=combatRender.transformedViewPosition.positionB-combatRender.resourceScaledExtent;
        uint32_t nearDepth=(int16_t)combatRender.nearClipDepth;
        if ((int32_t)nearDepth>(int32_t)combatRender.projectionRatio.ratio.denominator)
            combatRender.projectionRatio.ratio.denominator=nearDepth;
        BtechProjectValueDepthRatio(&combatRender.projectionRatio,&combatRender.projectionVectorScratch.positionA);
        if ((int32_t)combatRender.projectionVectorScratch.positionA>OBJECT_MAXIMUM_PROJECTION_SCALE)
            combatRender.projectionVectorScratch.positionA=OBJECT_MAXIMUM_PROJECTION_SCALE;
        combatRender.objectProjectionScale=(uint16_t)combatRender.projectionVectorScratch.positionA;
        if (object->prefix.renderFlags&OBJECT_CAPTURE_PROJECTION)
        {
            object->transformedPosition[0]=combatRender.transformedViewPosition.positionA;
            object->transformedPosition[1]=combatRender.transformedViewPosition.positionB;
            object->transformedPosition[2]=combatRender.transformedViewPosition.positionC;
            object->prefix.renderFlags&=(uint8_t)~OBJECT_PROJECTED_VISIBLE;
            if ((int16_t)combatRender.nearClipDepth<=(int32_t)combatRender.transformedViewPosition.positionB)
            {
                BtechProjectViewPosition(&combatRender.transformedViewPosition,&combatRender.projectionVectorScratch);
                BtechOffsetProjectedScreenPair(&combatRender.projectionVectorScratch);
                if (combatVideo.clipMinimumX<=(int32_t)combatRender.projectionVectorScratch.positionA
                    && combatVideo.clipMaximumX>=(int32_t)combatRender.projectionVectorScratch.positionA
                    && combatVideo.clipMinimumY<=(int32_t)combatRender.projectionVectorScratch.positionB
                    && combatVideo.clipMaximumY>=(int32_t)combatRender.projectionVectorScratch.positionB)
                {
                    object->projectionValue=combatRender.objectProjectionScale;
                    object->projectedScreenA=(int16_t)combatRender.projectionVectorScratch.positionA;
                    object->projectedScreenB=(int16_t)combatRender.projectionVectorScratch.positionB;
                    object->prefix.renderFlags|=OBJECT_PROJECTED_VISIBLE;
                }
            }
        }
        if (object->prefix.updateCallbackOffset && (object->prefix.renderFlags&OBJECT_CALLBACK_BEFORE_RENDER))
        {
            if (object->prefix.updateCallbackOffset!=WORLD_OBJECT_SELECTION_PIXEL_CALLBACK) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: object->prefix.updateCallbackOffset!=WORLD_OBJECT_SELECTION_PIXEL_CALLBACK");
            BtechUpdateObjectSelectionPixels(object,(uint16_t)ordinal);
        }
        BtechCullAndRenderWorldObject(object);
        if (object->prefix.updateCallbackOffset && !(object->prefix.renderFlags&OBJECT_CALLBACK_BEFORE_RENDER))
        {
            if (object->prefix.updateCallbackOffset!=WORLD_OBJECT_SELECTION_PIXEL_CALLBACK) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: object->prefix.updateCallbackOffset!=WORLD_OBJECT_SELECTION_PIXEL_CALLBACK");
            BtechUpdateObjectSelectionPixels(object,(uint16_t)ordinal);
        }
    }
    if (combatWorld.afterObjectListCallbackOffset)
    {
        if (combatWorld.afterObjectListCallbackOffset!=WORLD_LIST_AFTER_CALLBACK) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: afterObjectListCallbackOffset!=WORLD_LIST_AFTER_CALLBACK");
        BtechRestoreSelectionPixels(objects);
    }
}

/*0810:C1F6/C260/C058. Native marker lifecycle around the actual object draw.
 * The saved pixels alias the same CDD6 owner used by world ordering. */
void BtechPrepareSelectionPixels(BtechWorldObjectPrefix *const *objects,int16_t count)
{
    combatRender.selectionCount=0;
    if (combatRender.targetSelectionProbeActive)
    {
        combatRender.selectionPointBackground=BtechCaptureSelectionPixels((int16_t)combatRender.selectionPointScreenX,(int16_t)combatRender.selectionPointScreenY);
        BtechWriteSelectionPixels((int16_t)combatRender.selectionPointScreenX,(int16_t)combatRender.selectionPointScreenY,&combatRender.selectionMarkerPixels);
    }
    for (int16_t ordinal=0;ordinal<count;++ordinal)
        if (objects[ordinal]->renderFlags&OBJECT_CAPTURE_PROJECTION) objects[ordinal]->flags|=WORLD_OBJECT_TARGET_PROBE_VISIBLE;
}

void BtechRestoreSelectionPixels(BtechWorldObjectPrefix *const *objects)
{
    for (int16_t index=0;index<(int16_t)combatRender.selectionCount;++index)
    {
        if (index>=SELECTION_LIST_PREFIX) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: index>=SELECTION_LIST_PREFIX");
        uint16_t ordinal=combatRender.selectionObjectListIndices[index];
        if (ordinal>=COMBAT_WORLD_OBJECTS || !objects[ordinal]) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: ordinal>=COMBAT_WORLD_OBJECTS || !objects[ordinal]");
        BtechHeapWorldObject *object=(BtechHeapWorldObject *)objects[ordinal];
        BtechWriteSelectionPixels(object->projectedScreenA,object->projectedScreenB,&combatScene.objectSortOrSelectionScratch.distances[ordinal]);
    }
    if (combatRender.targetSelectionProbeActive)
    {
        BtechWriteSelectionPixels((int16_t)combatRender.selectionPointScreenX,(int16_t)combatRender.selectionPointScreenY,&combatRender.selectionPointBackground);
        combatRender.targetSelectionProbeActive=0;
    }
}

void BtechUpdateObjectSelectionPixels(BtechHeapWorldObject *object,uint16_t ordinal)
{
    int16_t resourceDelta=object->prefix.resourceId-otherShapeResourceBase;
    uint8_t retainCoveredEntry=(resourceDelta>=2 && resourceDelta<=7)
        || (resourceDelta>=10 && resourceDelta<=13) || (resourceDelta>=15 && resourceDelta<=22);
    if (combatRender.targetSelectionProbeActive)
    {
        combatRender.selectionCapturedPixels=BtechCaptureSelectionPixels((int16_t)combatRender.selectionPointScreenX,(int16_t)combatRender.selectionPointScreenY);
        if (!BtechTestSelectionMarkerPixels(&combatRender.selectionCapturedPixels))
        {
            if (!retainCoveredEntry) combatStartup.selectedHitObject=&object->prefix;
            combatRender.selectionPointBackground=combatRender.selectionCapturedPixels;
            BtechWriteSelectionPixels((int16_t)combatRender.selectionPointScreenX,(int16_t)combatRender.selectionPointScreenY,&combatRender.selectionMarkerPixels);
        }
    }
    for (int16_t index=0;index<(int16_t)combatRender.selectionCount;++index)
    {
        if (index>=SELECTION_LIST_PREFIX) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: index>=SELECTION_LIST_PREFIX");
        uint16_t selectedOrdinal=combatRender.selectionObjectListIndices[index];
        if (selectedOrdinal>=COMBAT_WORLD_OBJECTS || !combatWorld.cockpitWorldObjects[selectedOrdinal]) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: selectedOrdinal>=COMBAT_WORLD_OBJECTS || !cockpitWorldObjects[selectedOrdinal]");
        BtechHeapWorldObject *selected=(BtechHeapWorldObject *)combatWorld.cockpitWorldObjects[selectedOrdinal];
        combatRender.selectionCapturedPixels=BtechCaptureSelectionPixels(selected->projectedScreenA,selected->projectedScreenB);
        if (!BtechTestSelectionMarkerPixels(&combatRender.selectionCapturedPixels))
        {
            if (!retainCoveredEntry)
            {
                if (combatRender.selectionCount>SELECTION_LIST_PREFIX) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: selectionCount>SELECTION_LIST_PREFIX");
                combatRender.selectionObjectListIndices[index]=combatRender.selectionObjectListIndices[combatRender.selectionCount-1];
                --index;--combatRender.selectionCount;
                selected->prefix.flags&=(uint8_t)~WORLD_OBJECT_TARGET_PROBE_VISIBLE;
            }
            else
            {
                combatScene.objectSortOrSelectionScratch.distances[selectedOrdinal]=combatRender.selectionCapturedPixels;
                BtechWriteSelectionPixels(selected->projectedScreenA,selected->projectedScreenB,&combatRender.selectionMarkerPixels);
            }
        }
    }
    /* C194/C19A repeat the same first-byte bit08 test. */
    if ((object->prefix.renderFlags&(OBJECT_CAPTURE_PROJECTION|OBJECT_PROJECTED_VISIBLE))
        ==(OBJECT_CAPTURE_PROJECTION|OBJECT_PROJECTED_VISIBLE))
    {
        uint16_t count=combatRender.selectionCount++;
        if (count>=SELECTION_LIST_PREFIX || ordinal>=COMBAT_WORLD_OBJECTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: count>=SELECTION_LIST_PREFIX || ordinal>=COMBAT_WORLD_OBJECTS");
        combatRender.selectionObjectListIndices[count]=ordinal;
        combatScene.objectSortOrSelectionScratch.distances[ordinal]=BtechCaptureSelectionPixels(object->projectedScreenA,object->projectedScreenB);
        BtechWriteSelectionPixels(object->projectedScreenA,object->projectedScreenB,&combatRender.selectionMarkerPixels);
    }
    else object->prefix.flags&=(uint8_t)~WORLD_OBJECT_TARGET_PROBE_VISIBLE;
}

/*0810:C03C. Probe position and latch; no alternative hit-test algorithm. */
void BtechBeginSelectionPixelProbe(uint16_t x,uint16_t y)
{
    combatRender.selectionPointScreenX=x;combatRender.selectionPointScreenY=y;
    combatStartup.selectedHitObject=NULL;combatRender.targetSelectionProbeActive=1;
}

/*0810:C2CE. Single-pixel mode observes only AL. The other three returned
 * stack bytes are undefined native residue, not colours: discarded here,
 * not fabricated DOS RAM. Every mode-zero consumer reads only the low byte. */
uint32_t BtechCaptureSelectionPixels(int16_t x,int16_t y)
{
    if (!combatImport.hudColourSchemeIndex) return (uint8_t)BtechReadClippedPixel(x,y);
    uint32_t pixels=0;
    for (uint16_t column=0;column<2;++column)
        for (uint16_t row=0;row<2;++row)
            pixels|=(uint32_t)(uint8_t)BtechReadClippedPixel((int16_t)(x+column),(int16_t)(y+row))<<((column*2+row)*8);
    return pixels;
}

/*0810:C33D. Pointer identity selects colour0D only in single-pixel mode. */
void BtechWriteSelectionPixels(int16_t x,int16_t y,const uint32_t *pixels)
{
    if (combatImport.hudColourSchemeIndex)
    {
        for (uint16_t column=0;column<2;++column)
            for (uint16_t row=0;row<2;++row)
                BtechWriteClippedPixel((int16_t)(x+column),(int16_t)(y+row),(uint8_t)(*pixels>>((column*2+row)*8)));
    }
    else BtechWriteClippedPixel(x,y,pixels==&combatRender.selectionMarkerPixels?SELECTION_MARKER_COLOUR:(uint8_t)*pixels);
}

/*0810:C3B9. Multi-pixel marker is the authored00010203 column-major pattern. */
uint16_t BtechTestSelectionMarkerPixels(const uint32_t *pixels)
{
    return combatImport.hudColourSchemeIndex?*pixels==UINT32_C(0x03020100):(uint8_t)*pixels==SELECTION_MARKER_COLOUR;
}
