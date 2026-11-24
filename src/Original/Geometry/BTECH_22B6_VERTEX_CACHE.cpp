#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_render.h"
#include <stdlib.h>

/*22B6:13EB. Genuine NEAR counterpart to12C4, not a new game helper. */
void BtechTransformWordVector(const uint16_t source[3],const BtechOrientationBasis *basis,uint16_t destination[3])
{
    uint8_t mode=basis->mode;
    for (uint16_t output=0;output<3;++output)
    {
        if (!mode || (mode==1 && output==2)) destination[output]=source[output];
        else
        {
            uint32_t product=0;
            for (uint16_t input=0;input<(mode==1?2:3);++input)
                product+=(uint32_t)((int32_t)(int16_t)source[input]*(int16_t)basis->coefficients[input*3+output]);
            destination[output]=(uint16_t)((product<<2)>>16);
        }
    }
}

/*22B6:1F7C. WORD input, DWORD integer output and three live fractional WORDs.
 * Products wrap to32 bits BEFORE sign extension into DX:AX:BP and shifting.
 * Keep each destination store before its aliased7864 fraction store. */
void BtechTransformWordVectorToDword(const uint16_t source[3],const BtechOrientationBasis *basis,BtechPosition3D *destination)
{
    uint8_t mode=basis->mode;
    uint32_t *components[]={&destination->positionA,&destination->positionB,&destination->positionC};
    for (uint16_t output=0;output<3;++output)
    {
        if (!mode || (mode==1 && output==2))
        {
            uint32_t value=(int16_t)source[output];
            for (uint16_t shifts=combatRender.resourceScaleShift;shifts;--shifts) value<<=1;
            *components[output]=value;
            geometryTransformFractions[output]=0;
        }
        else
        {
            uint32_t product=0;
            for (uint16_t input=0;input<(mode==1?2:3);++input)
                product+=(uint32_t)((int32_t)(int16_t)source[input]*(int16_t)basis->coefficients[input*3+output]);
            uint64_t shifted=(int32_t)product;
            shifted=(shifted<<2)&UINT64_C(0xFFFFFFFFFFFF);
            for (uint16_t shifts=combatRender.resourceScaleShift;shifts;--shifts) shifted=(shifted<<1)&UINT64_C(0xFFFFFFFFFFFF);
            *components[output]=(uint32_t)(shifted>>16);
            geometryTransformFractions[output]=(uint16_t)shifted;
        }
    }
}

/*22B6:2175. Any nonzero flag reuses the existing cache, including normals.
 * ADD fraction,0 clears carry: translation is a wrapped DWORD integer add. */
void BtechCacheTransformedGeometryVector(uint8_t vectorIndex)
{
    if (vectorIndex>=GEOMETRY_CACHE_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: vectorIndex>=GEOMETRY_CACHE_VERTICES"); /* FF aliases X-cache bytes in the native code owner. */
    if (combatGeometryTables.vertexNearPlaneState[vectorIndex]) return;
    combatGeometryTables.vertexNearPlaneState[vectorIndex]=1;
    for (uint16_t axis=0;axis<3;++axis) combatRender.viewVectorCoefficients[axis]=combatRender.renderGeometry[vectorIndex][axis];
    BtechTransformWordVectorToDword(combatRender.viewVectorCoefficients,combatRender.activeRenderBasis,&combatRender.projectionRatio.vector);
    combatGeometry.geometryViewX.wide[vectorIndex]=combatRender.projectionRatio.vector.positionA+combatRender.transformedViewPosition.positionA;
    combatGeometry.geometryViewDepth.wide[vectorIndex]=combatRender.projectionRatio.vector.positionB+combatRender.transformedViewPosition.positionB;
    combatGeometry.geometryViewZ.wide[vectorIndex]=combatRender.projectionRatio.vector.positionC+combatRender.transformedViewPosition.positionC;
}

/*22B6:2227. WORD normal values occupy LOW WORDs at DWORD strides. The high
 * halves are intentionally stale and share storage with transformed vertices. */
void BtechCacheRotatedGeometryVector(uint8_t vectorIndex)
{
    if (vectorIndex>=GEOMETRY_CACHE_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: vectorIndex>=GEOMETRY_CACHE_VERTICES");
    if (combatGeometryTables.vertexNearPlaneState[vectorIndex]) return;
    combatGeometryTables.vertexNearPlaneState[vectorIndex]=1;
    for (uint16_t axis=0;axis<3;++axis) combatRender.viewVectorCoefficients[axis]=combatRender.renderGeometry[vectorIndex][axis];
    BtechTransformWordVector(combatRender.viewVectorCoefficients,combatRender.activeRenderBasis,combatRender.transformedEntryVector);
    combatGeometry.geometryViewX.words[vectorIndex*2]=combatRender.transformedEntryVector[0];
    combatGeometry.geometryViewDepth.words[vectorIndex*2]=combatRender.transformedEntryVector[1];
    combatGeometry.geometryViewZ.words[vectorIndex*2]=combatRender.transformedEntryVector[2];
}

/*22B6:2586, including its24E9 direct-basis tail. Native result DX:AX:CX;
 * uint64_t only transports those48 bits. Callers test signed HIGH WORD. */
uint64_t BtechGetWidePolygonFacingDotHighWord(uint8_t normalIndex,uint8_t firstVertexIndex)
{
    if (combatRender.useViewBasisDirectly)
    {
        if (normalIndex>=GEOMETRY_CACHE_VERTICES || firstVertexIndex>=GEOMETRY_CACHE_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: normalIndex>=GEOMETRY_CACHE_VERTICES || firstVertexIndex>=GEOMETRY_CACHE_VERTICES"); /* FF geometry overread is outside the recovered array. */
        uint32_t *position[]={&combatRender.projectionVectorScratch.positionA,&combatRender.projectionVectorScratch.positionB,&combatRender.projectionVectorScratch.positionC};
        const uint32_t *relative[]={&combatRender.cameraRelativePosition.positionA,&combatRender.cameraRelativePosition.positionB,&combatRender.cameraRelativePosition.positionC};
        for (uint16_t axis=0;axis<3;++axis)
        {
            uint32_t value=(int16_t)combatRender.renderGeometry[firstVertexIndex][axis];
            for (uint16_t shifts=combatRender.resourceScaleShift;shifts;--shifts) value<<=1;
            *position[axis]=value+*relative[axis];
        }
        for (uint16_t axis=0;axis<3;++axis) combatRender.viewVectorCoefficients[axis]=combatRender.renderGeometry[normalIndex][axis];
    }
    else
    {
        BtechCacheRotatedGeometryVector(normalIndex);
        BtechCacheTransformedGeometryVector(firstVertexIndex);
        combatRender.projectionVectorScratch.positionA=combatGeometry.geometryViewX.wide[firstVertexIndex];
        combatRender.projectionVectorScratch.positionB=combatGeometry.geometryViewDepth.wide[firstVertexIndex];
        combatRender.projectionVectorScratch.positionC=combatGeometry.geometryViewZ.wide[firstVertexIndex];
        combatRender.viewVectorCoefficients[0]=combatGeometry.geometryViewX.words[normalIndex*2];
        combatRender.viewVectorCoefficients[1]=combatGeometry.geometryViewDepth.words[normalIndex*2];
        combatRender.viewVectorCoefficients[2]=combatGeometry.geometryViewZ.words[normalIndex*2];
    }
    return BtechDotSignedVectorByWordCoefficients(&combatRender.projectionVectorScratch,combatRender.viewVectorCoefficients);
}

/*22B6:6473. Shade normals use WORD strides on the narrow path and DWORD
 * strides on the wide path. Neither path clears the other half of a slot. */
void BtechCachePolygonShadeNormal(uint8_t normalIndex)
{
    if (normalIndex>=GEOMETRY_CACHE_VERTICES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: normalIndex>=GEOMETRY_CACHE_VERTICES");
    uint16_t wordIndex=normalIndex*(combatRender.widePolygonCoordinates?2:1);
    if (combatGeometryTables.vertexNearPlaneState[normalIndex])
    {
        combatRender.viewVectorCoefficients[0]=combatGeometry.geometryViewX.words[wordIndex];
        combatRender.viewVectorCoefficients[1]=combatGeometry.geometryViewDepth.words[wordIndex];
        combatRender.viewVectorCoefficients[2]=combatGeometry.geometryViewZ.words[wordIndex];
        return;
    }
    combatGeometryTables.vertexNearPlaneState[normalIndex]=1;
    for (uint16_t axis=0;axis<3;++axis) combatRender.transformedEntryVector[axis]=combatRender.renderGeometry[normalIndex][axis];
    BtechTransformWordVector(combatRender.transformedEntryVector,combatRender.activeRenderBasis,combatRender.viewVectorCoefficients);
    combatGeometry.geometryViewX.words[wordIndex]=combatRender.viewVectorCoefficients[0];
    combatGeometry.geometryViewDepth.words[wordIndex]=combatRender.viewVectorCoefficients[1];
    combatGeometry.geometryViewZ.words[wordIndex]=combatRender.viewVectorCoefficients[2];
}
