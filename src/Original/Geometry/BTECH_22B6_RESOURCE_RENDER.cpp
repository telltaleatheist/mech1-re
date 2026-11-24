#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_world.h"
#include <stdlib.h>

/*22B6:1503. Signed IMUL products accumulated as a wrapped DWORD, DF-clear. */
uint32_t BtechDotProductSignedWordVectors(const uint16_t first[3],const uint16_t second[3])
{
    uint32_t product=0;
    for (uint16_t component=0;component<3;++component)
    {
        int16_t firstComponent=first[component];
        int16_t secondComponent=second[component];
        product+=(uint32_t)((int32_t)firstComponent*secondComponent);
    }
    return product;
}

/*22B6:272A. Code-owned BYTE flags, normal forward REP STOSW and odd tail. */
void BtechClearGeometryVectorCacheFlags(void)
{
    if (combatRender.renderGeometryVectorCount>sizeof(combatGeometryTables.vertexNearPlaneState)) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: renderGeometryVectorCount>sizeof(vertexNearPlaneState)"); /* Unrelated CS-owner overwrite, not extra cache slots. */
    uint16_t destination=0;
    for (uint16_t words=combatRender.renderGeometryVectorCount>>1;words;--words)
    {combatGeometryTables.vertexNearPlaneState[destination++]=0;combatGeometryTables.vertexNearPlaneState[destination++]=0;}
    if (combatRender.renderGeometryVectorCount&1) combatGeometryTables.vertexNearPlaneState[destination]=0;
}

/*22B6:19CE. Native entry ordering, per-entry variant bytes and LOD walk.
 * Binary bank offsets have already become typed references at05DE. Keep the
 * three original parallel tables, signed counts and live shared working owners. */
void BtechPrepareRenderResourceEntries(uint16_t resourceId,const uint8_t *entryVariants)
{
    combatRender.activeRenderHeader=BtechLookupBankResource(resourceId);
    if (!combatRender.activeRenderHeader) return;
    BtechPrepareResourceCoordinatePaths();
    combatRender.activeRenderEntry=combatRender.activeRenderHeader->entries;
    int16_t entryCount=combatRender.activeRenderHeader->entryCount;
    for (int16_t entryIndex=0;entryIndex<entryCount;++entryIndex)
    {
        if (entryIndex>=RENDER_ENTRY_PREFIX_CAPACITY) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: entryIndex>=RENDER_ENTRY_PREFIX_CAPACITY"); /* Native keys overlap7538; no padded allocation. */
        combatRender.orderedRenderEntries[entryIndex]=combatRender.activeRenderEntry;
        combatRender.activeEntryGeometry=combatRender.activeRenderEntry->vectors;
        const uint16_t *vector=combatRender.activeEntryGeometry[combatRender.activeRenderEntry->firstVectorIndex];
        for (uint16_t component=0;component<3;++component) combatRender.viewVectorCoefficients[component]=vector[component];
        const uint16_t *keyVector=combatRender.viewVectorCoefficients;
        if (!combatRender.useViewBasisDirectly)
        {
            BtechTransformWordVectorFar(combatRender.viewVectorCoefficients,combatRender.activeRenderBasis,combatRender.transformedEntryVector);
            keyVector=combatRender.transformedEntryVector;
        }
        combatRender.renderEntryOrderingKeys[entryIndex]=BtechDotProductSignedWordVectors(combatRender.resourceReferenceVector,keyVector);
        combatRender.renderEntryVariants[entryIndex]=entryVariants?entryVariants[entryIndex]:0;
        if (entryVariants && combatRender.renderEntryVariants[entryIndex]>=combatRender.activeRenderEntry->variantCount)
            combatRender.renderEntryVariants[entryIndex]=0;
        ++combatRender.activeRenderEntry;
    }
    for (int16_t entryIndex=0;entryIndex<entryCount;++entryIndex)
    {
        int16_t greatestIndex=entryIndex;
        for (int16_t candidate=entryIndex+1;candidate<entryCount;++candidate)
            if ((int32_t)combatRender.renderEntryOrderingKeys[greatestIndex]<(int32_t)combatRender.renderEntryOrderingKeys[candidate]) greatestIndex=candidate;
        if (greatestIndex!=entryIndex)
        {
            uint8_t savedVariant=combatRender.renderEntryVariants[entryIndex];
            combatRender.renderEntryVariants[entryIndex]=combatRender.renderEntryVariants[greatestIndex];
            combatRender.renderEntryVariants[greatestIndex]=savedVariant;
            uint32_t savedKey=combatRender.renderEntryOrderingKeys[entryIndex];
            combatRender.renderEntryOrderingKeys[entryIndex]=combatRender.renderEntryOrderingKeys[greatestIndex];
            combatRender.renderEntryOrderingKeys[greatestIndex]=savedKey;
            combatRender.activeRenderEntry=combatRender.orderedRenderEntries[entryIndex];
            combatRender.orderedRenderEntries[entryIndex]=combatRender.orderedRenderEntries[greatestIndex];
            combatRender.orderedRenderEntries[greatestIndex]=combatRender.activeRenderEntry;
        }
    }
    uint8_t sharedGeometry=combatRender.activeRenderHeader->flags&RESOURCE_SHARED_GEOMETRY;
    if (sharedGeometry)
    {
        combatRender.activeRenderEntry=combatRender.orderedRenderEntries[0]; /* Retained entry zero even when count<=0. */
        if (!combatRender.activeRenderEntry) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !activeRenderEntry"); /* Unsupported native null-near alias. */
        combatRender.renderGeometry=combatRender.activeRenderEntry->vectors;
        combatRender.renderGeometryVectorCount=combatRender.activeRenderEntry->vectorCount;
        BtechClearGeometryVectorCacheFlags();
    }
    for (int16_t entryIndex=0;entryIndex<entryCount;++entryIndex)
    {
        combatRender.activeRenderEntry=combatRender.orderedRenderEntries[entryIndex];
        combatRender.activeEntryGeometry=combatRender.activeRenderEntry->vectors;
        combatRender.activeVariant=&combatRender.activeRenderEntry->variants[combatRender.renderEntryVariants[entryIndex]];
        int16_t recordCount=combatRender.activeVariant->primitiveCount;
        int16_t recordIndex=0;
        const BtechRenderPrimitive *selected=combatRender.activeVariant->primitives;
        while (recordIndex<recordCount && selected->scaleLimit<combatRender.objectProjectionScale)
        {++recordIndex;++selected;}
        combatRender.activePrimitiveRecord=selected;
        if (recordIndex==recordCount) --combatRender.activePrimitiveRecord; /* Count0 backs up too; negative doesn't. */
        if (!sharedGeometry)
        {
            combatRender.renderGeometry=combatRender.activeEntryGeometry;
            combatRender.renderGeometryVectorCount=combatRender.activeRenderEntry->vectorCount;
            BtechClearGeometryVectorCacheFlags();
        }
        switch (combatRender.activePrimitiveRecord->kind)
        {
        case RENDER_PRIMITIVE_POLYGON: BtechDrawPolygonPayload(&combatRender.activePrimitiveRecord->polygonPayload);break;
        case RENDER_PRIMITIVE_ELLIPSE: BtechDrawReferencePointEllipse(&combatRender.activePrimitiveRecord->ellipsePayload);break;
        case RENDER_PRIMITIVE_ORIGIN_PICTURE: /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Render resource uses an unsupported origin-picture primitive."); /*176E. */
        case RENDER_PRIMITIVE_REFERENCE_PICTURE: /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Render resource uses an unsupported reference-picture primitive."); /*1862. */
        } /* Other native BYTE kinds really are skipped. No returning renderer seam. */
    }
}

/*22B6:1596. FAR payload pair becomes a typed six-byte primitive at bank IO.
 * Preserve shared projection scratch and the signed upper-only radius cap. */
void BtechDrawReferencePointEllipse(const BtechRenderEllipse *ellipse)
{
    const uint16_t *reference=combatRender.activeEntryGeometry[ellipse->referenceVectorIndex];
    for (uint16_t component=0;component<3;++component) combatRender.viewVectorCoefficients[component]=reference[component];
    BtechTransformWordVectorFar(combatRender.viewVectorCoefficients,combatRender.activeRenderBasis,combatRender.renderTransformedReference);
    combatRender.projectionRatio.vector.positionA=combatRender.transformedViewPosition.positionA+(uint32_t)(int16_t)combatRender.renderTransformedReference[0];
    combatRender.projectionRatio.vector.positionB=combatRender.transformedViewPosition.positionB+(uint32_t)(int16_t)combatRender.renderTransformedReference[1];
    combatRender.projectionRatio.vector.positionC=combatRender.transformedViewPosition.positionC+(uint32_t)(int16_t)combatRender.renderTransformedReference[2];
    if ((int32_t)(int16_t)combatRender.nearClipDepth>(int32_t)combatRender.projectionRatio.ratio.denominator) return;
    combatRender.projectionVectorScratch.positionA=(uint32_t)ellipse->radiusScale;
    combatRender.projectionVectorScratch.positionB=combatRender.projectionRatio.ratio.denominator;
    uint32_t radius;
    BtechProjectionRatio radiusProjection={.ratio={combatRender.projectionVectorScratch.positionA,combatRender.projectionVectorScratch.positionB,0}};
    BtechProjectValueDepthRatio(&radiusProjection,&radius);
    if ((int32_t)radius>0x0100) radius=0x0100;
    BtechProjectViewPosition(&combatRender.projectionRatio.vector,&combatRender.projectionVectorScratch);
    BtechOffsetProjectedScreenPair(&combatRender.projectionVectorScratch);
    if ((int32_t)(int16_t)(combatVideo.clipMinimumX-0x0100)>(int32_t)combatRender.projectionVectorScratch.positionA
        || (int32_t)(int16_t)(combatVideo.clipMaximumX+0x0100)<(int32_t)combatRender.projectionVectorScratch.positionA
        || (int32_t)(int16_t)(combatVideo.clipMinimumY-0x0100)>(int32_t)combatRender.projectionVectorScratch.positionB
        || (int32_t)(int16_t)(combatVideo.clipMaximumY+0x0100)<(int32_t)combatRender.projectionVectorScratch.positionB) return;
    combatVideo.screenPolygonNeedsClip=true;combatVideo.drawPolygonFilled=true;
    uint8_t colour=combatVideo.graphicsColourMode?ellipse->colour:ellipse->alternateColour;
    combatVideo.drawLineColour=combatRender.activePaletteRemap?combatRender.activePaletteRemap[colour]:colour;
    if (!combatVideo.graphicsColourMode) combatVideo.drawLineColour&=0x1F;
    combatVideo.drawFillColour=combatVideo.drawLineColour;
    BtechDrawStyledEllipse((int16_t)radius,(uint16_t)combatRender.projectionVectorScratch.positionA,(uint16_t)combatRender.projectionVectorScratch.positionB);
}
