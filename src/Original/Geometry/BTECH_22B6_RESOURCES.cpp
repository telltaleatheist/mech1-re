#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_resources.h"
#include "mechwarrior/combat_heap.h"
#include "mechwarrior/combat_record.h"
#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_installation.h"
#include <stdlib.h>
#include <string.h>

/*22B6:05DE. ADD/ADC of selector:offset from a paragraph-aligned allocation
 * becomes typed resource references. Internal near addresses must still use
 * paragraphBase, NOT the header pointer. No count-prefix is invented. */
BtechResourceBank BtechDecodeAndRelocateResourceBank(const char *filename)
{
    BtechResourceBank bank={0};
    bank.allocation=BtechAllocateAndDecodeResource(filename);
    const CombatRecord file(bank.allocation.bytes,bank.allocation.byteCount,filename);
    uint16_t count=0;
    for (;;)
    {
        const CombatRecord pair=file.part((uint16_t)(count*4),4,"the directory's end");
        if (!(pair.word(0)|pair.word(2))) break;
        if (++count==0x4000) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: ++count==0x4000"); /* Missing terminator would wrap the native cursor. */
    }
    bank.resources=static_cast<BtechShapeResource **>(calloc((size_t)count+1,sizeof(*bank.resources)));
    bank.decodedResources=static_cast<BtechShapeResource *>(calloc(count?count:1,sizeof(*bank.decodedResources)));
    bank.decodedResourceCount=count;
    if (!bank.resources || !bank.decodedResources) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !bank.resources || !bank.decodedResources");
    for (uint16_t index=0;index<count;++index)
    {
        const CombatRecord pair=file.part(index*4,4,"the directory's end");
        uint16_t offset=pair.word(0);
        uint32_t paragraphBytes=(uint32_t)pair.word(2)<<4;
        /* Offsets inside a shape are near its paragraph. */
        const CombatRecord shape=file.window(paragraphBytes);
        const CombatRecord header=shape.part(offset,14,"a shape's header");
        BtechShapeResource *resource=&bank.decodedResources[index];
        bank.resources[index]=resource;
        resource->paragraphBase=bank.allocation.bytes+paragraphBytes;
        resource->header=resource->paragraphBase+offset;
        resource->flags=header.byte(0);
        resource->scaleShift=header.byte(1);
        resource->baseScale=(int16_t)header.word(2);
        for (uint16_t axis=0;axis<3;++axis) resource->referenceVector[axis]=header.word(4+axis*2);
        resource->entryCount=(int16_t)header.word(0x0A);
        uint16_t entryOffset=header.word(0x0C);
        if (resource->entryCount<=0) continue;
        resource->entries=static_cast<BtechRenderEntry *>(calloc((uint16_t)resource->entryCount,sizeof(*resource->entries)));
        if (!resource->entries) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !resource->entries");
        for (int16_t entryIndex=0;entryIndex<resource->entryCount;++entryIndex,entryOffset+=8)
        {
            const CombatRecord encodedEntry=shape.part(entryOffset,8,"a shape's entry");
            BtechRenderEntry *entry=&resource->entries[entryIndex];
            entry->firstVectorIndex=encodedEntry.byte(0);entry->vectorCount=encodedEntry.byte(1);
            uint16_t vectorOffset=encodedEntry.word(2);
            entry->variantCount=encodedEntry.word(4);
            uint16_t variantOffset=encodedEntry.word(6);
            uint16_t vectorCount=entry->vectorCount;
            if (vectorCount<=entry->firstVectorIndex) vectorCount=(uint16_t)entry->firstVectorIndex+1;
            entry->vectors=static_cast<uint16_t (*)[3]>(calloc(vectorCount,sizeof(*entry->vectors)));
            if (!entry->vectors) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !entry->vectors");
            for (uint16_t vector=0;vector<vectorCount;++vector,vectorOffset+=6)
            {
                const CombatRecord encodedVector=shape.part(vectorOffset,6,"a shape's vector");
                for (uint16_t axis=0;axis<3;++axis) entry->vectors[vector][axis]=encodedVector.word(axis*2);
            }
            /* A zero variant limit still selects row zero in19CE. Its count
             * zero LOD backs up one eight-byte record; retain that neighbour. */
            uint32_t variants=entry->variantCount?entry->variantCount:1;
            entry->variants=static_cast<BtechRenderVariant *>(calloc(variants,sizeof(*entry->variants)));
            if (!entry->variants) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !entry->variants");
            for (uint32_t variantIndex=0;variantIndex<variants;++variantIndex,variantOffset+=4)
            {
                const CombatRecord encodedVariant=shape.part(variantOffset,4,"a shape's variant");
                BtechRenderVariant *variant=&entry->variants[variantIndex];
                variant->primitiveCount=(int16_t)encodedVariant.word(0);
                uint16_t primitiveOffset=encodedVariant.word(2);
                uint16_t primitives=variant->primitiveCount>0?(uint16_t)variant->primitiveCount:1;
                BtechRenderPrimitive *storage=static_cast<BtechRenderPrimitive *>(calloc((size_t)primitives+1,sizeof(*storage)));
                if (!storage) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !storage");
                variant->primitives=storage+1;
                for (int32_t primitiveIndex=variant->primitiveCount==0?-1:0;primitiveIndex<primitives;++primitiveIndex)
                {
                    uint16_t at=primitiveOffset+primitiveIndex*8;
                    const CombatRecord encoded=shape.part(at,8,"a shape's primitive");
                    BtechRenderPrimitive *primitive=&variant->primitives[primitiveIndex];
                    primitive->scaleLimit=encoded.byte(0);
                    primitive->kind=encoded.byte(1);
                    const CombatRecord encodedPayload=encoded.part(2,6,"a shape's primitive");
                    if (primitive->kind==RENDER_PRIMITIVE_ELLIPSE)
                        primitive->ellipsePayload=BtechRenderEllipse{(int16_t)encodedPayload.word(0),encodedPayload.byte(2),encodedPayload.byte(3),encodedPayload.byte(4),encodedPayload.byte(5)};
                    else if (primitive->kind!=RENDER_PRIMITIVE_POLYGON) primitive->encodedPayload=encodedPayload.bytes();
                    if (primitive->kind==RENDER_PRIMITIVE_POLYGON)
                    {
                        primitive->polygonPayload.polygonCount=(int16_t)encodedPayload.word(0);
                        uint16_t polygonOffset=encodedPayload.word(2);
                        /* DEC/JGE uses overflow as well as sign. Count8000
                         * exits even though the decremented WORD is7FFF. */
                        uint16_t polygons=primitive->polygonPayload.polygonCount>0?(uint16_t)primitive->polygonPayload.polygonCount:0;
                        if (!polygons) continue;
                        primitive->polygonPayload.polygons=static_cast<BtechRenderPolygon *>(calloc(polygons,sizeof(*primitive->polygonPayload.polygons)));
                        if (!primitive->polygonPayload.polygons) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !primitive->polygonPayload.polygons");
                        for (uint16_t polygonIndex=0;polygonIndex<polygons;++polygonIndex,polygonOffset+=8)
                        {
                            const CombatRecord encodedPolygon=shape.part(polygonOffset,8,"a shape's polygon");
                            BtechRenderPolygon *polygon=&primitive->polygonPayload.polygons[polygonIndex];
                            polygon->flags=encodedPolygon.byte(0);polygon->fillColour=encodedPolygon.byte(1);polygon->lineColour=encodedPolygon.byte(2);
                            polygon->alternateFillColour=encodedPolygon.byte(3);polygon->alternateLineColour=encodedPolygon.byte(4);polygon->normalVectorIndex=encodedPolygon.byte(5);
                            polygon->vertexIndices=shape.list(encodedPolygon.word(6),POLYGON_VERTEX_LIST_END,"a polygon's vertex list");
                        }
                    }
                }
            }
        }
    }
    /* Count is established by the original bind/count callees, not guessed
     * from resource sizes. Additional allocations only decode binary views;
     * no new game entries, geometry or variants are synthesized. Malformed
     * physical overreads/field-straddling near wraps remain unsupported. */
    return bank;
}

/*22B6:02DE. Same relocation kernel, but independent collision-bank ownership.
 * Keep this original boundary rather than alias collision and render banks. */
void BtechLoadCollisionResourceBank(const char *filename)
{
    BtechResourceAllocation allocation=BtechAllocateAndDecodeResource(filename);
    combatResources.surfaceResourceBank.allocation=allocation;
    const CombatRecord file(allocation.bytes,allocation.byteCount,filename);
    uint16_t count=0;
    for (;;)
    {
        const CombatRecord pair=file.part((uint16_t)(count*4),4,"the directory's end");
        if (!(pair.word(0)|pair.word(2))) break;
        if (++count==0x4000) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: ++count==0x4000");
    }
    combatResources.surfaceResourceBank.surfaces=static_cast<BtechSurfaceDefinition **>(calloc((size_t)count+1,sizeof(BtechSurfaceDefinition *)));
    if (!combatResources.surfaceResourceBank.surfaces) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !surfaceResourceBank.surfaces");
    for (uint16_t index=0;index<count;++index)
    {
        const CombatRecord pair=file.part(index*4,4,"the directory's end");
        uint16_t offset=pair.word(0),paragraph=pair.word(2);
        /* Offsets inside a surface are near its paragraph. */
        const CombatRecord surfaceBytes=file.window((uint32_t)paragraph<<4);
        const CombatRecord header=surfaceBytes.part(offset,10,"a surface's header");
        /* Duplicate relocation pairs denote ONE immutable surface, preserving
         * descriptor identity in cached surface queries. This is IO only. */
        for (uint16_t prior=0;prior<index;++prior)
            if (file.part(prior*4,2,"the directory's end").word(0)==offset && file.part(prior*4+2,2,"the directory's end").word(0)==paragraph)
            {combatResources.surfaceResourceBank.surfaces[index]=combatResources.surfaceResourceBank.surfaces[prior];break;}
        if (combatResources.surfaceResourceBank.surfaces[index]) continue;
        BtechSurfaceDefinition *surface=static_cast<BtechSurfaceDefinition *>(calloc(1,sizeof(*surface)));if (!surface) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !surface");
        combatResources.surfaceResourceBank.surfaces[index]=surface;
        surface->extentA=header.word(0);surface->extentB=header.word(2);
        surface->directResultFlag=header.byte(4);surface->priorityClass=header.byte(5);
        surface->surfaceClass=header.byte(6);surface->patchCount=header.byte(7);
        uint16_t table=header.word(8);
        if (surface->patchCount)
        {
            surface->patches=static_cast<BtechSurfacePatch *>(calloc(surface->patchCount,sizeof(BtechSurfacePatch)));
            if (!surface->patches) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !surface->patches");
        }
        for (uint16_t patchIndex=0;patchIndex<surface->patchCount;++patchIndex)
        {
            uint16_t at=table+patchIndex*0x10;
            const CombatRecord encoded=surfaceBytes.part(at,0x10,"a surface's patch");
            BtechSurfacePatch *patch=&surface->patches[patchIndex];
            for (uint16_t axis=0;axis<3;++axis) patch->orderingPlaneCoefficients[axis]=(int16_t)encoded.word(axis*2);
            patch->heightProductScale=encoded.byte(6);patch->orientationResponseMagnitude=encoded.byte(7);
            patch->orientationResponseHeading=encoded.byte(8);patch->boundaryCount=encoded.byte(9);
            patch->baseHeight=(int16_t)encoded.word(0x0C);
            uint16_t boundaryOffset=encoded.word(0x0A);
            uint16_t boundaryCount=patch->boundaryCount?patch->boundaryCount:1; /* C8C0 reads first even when count0. */
            patch->boundaries=static_cast<BtechSurfaceBoundary *>(calloc(boundaryCount,sizeof(BtechSurfaceBoundary)));if (!patch->boundaries) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !patch->boundaries");
            for (uint16_t boundary=0;boundary<=boundaryCount;++boundary)
            {
                uint16_t cursor=boundary==boundaryCount?encoded.word(0x0E):(uint16_t)(boundaryOffset+boundary*8);
                const CombatRecord line=surfaceBytes.part(cursor,8,"a patch's boundary");
                BtechSurfaceBoundary *target=boundary==boundaryCount?&patch->heightBoundary:&patch->boundaries[boundary];
                *target=BtechSurfaceBoundary{(int16_t)line.word(0),(int16_t)line.word(2),
                    (int16_t)line.word(4),(int16_t)line.word(6)};
            }
        }
    }
    combatResources.surfaceResourceBank.resourceCount=count;
}

/*22B6:04FA. Initial bank is copied; remaining slots are all cleared. */
void BtechAllocateShapeBankRegistry(uint16_t count)
{
    BtechHostRetainDetachedShapeRegistry(); /* Host ownership of the native orphan, no early free. */
    combatResources.shapeResourceBankCount=count;
    if (!count || (uint32_t)count*6>UINT16_MAX) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !count || (uint32_t)count*6>UINT16_MAX");
    enum { NATIVE_SHAPE_BANK_BYTES=6 };
    combatResources.shapeResourceBanks=static_cast<BtechResourceBank *>(BtechAllocateNearBytes((uint16_t)(count*NATIVE_SHAPE_BANK_BYTES),(size_t)count*sizeof(*combatResources.shapeResourceBanks),BTECH_NEAR_SHAPE_BANKS));
    if (!combatResources.shapeResourceBanks) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !shapeResourceBanks");
    combatResources.shapeResourceBanks[0]=combatResources.initialShapeResourceBank;
    for (uint16_t slot=1;slot<count;++slot) combatResources.shapeResourceBanks[slot]=BtechResourceBank{};
}

/*22B6:034E. Native leaves its far pair/count stale until the next load.
 * Host decoded references become invalid at paragraph free; no live surface
 * query between release/reload is supported. Count is deliberately retained. */
void BtechReleaseCollisionBankAllocation(void)
{
    BtechFreeDosMemoryBlock(combatResources.surfaceResourceBank.allocation.bytes);
    BtechHostDiscardDecodedSurfaceBank(&combatResources.surfaceResourceBank);
    combatResources.surfaceResourceBank.allocation.bytes=NULL; /* Released host ownership only. */
}

/*22B6:064A. Native forwards the original two WORD far pair;1F10 consumes
 * only its selector. The wider C bank also carries host decoded ownership. */
void BtechReleaseResourceBankSelector(BtechResourceBank bank)
{
    BtechFreeDosMemoryBlock(bank.allocation.bytes);
    BtechHostDiscardDecodedShapeBank(&bank);
}

/*22B6:06C4. Return allocation before clearing live directory/count. The host
 * decoded owner travels with that detached allocation, not another live bank. */
BtechResourceBank BtechDetachShapeResourceBank(uint16_t bankSlot)
{
    if ((int16_t)bankSlot<0 || (int16_t)bankSlot>=(int16_t)combatResources.shapeResourceBankCount)
        BtechShowFatalErrorAndExit("a shape list in the combat resources is damaged"); /*304E:2D83*/
    BtechResourceBank bank=combatResources.shapeResourceBanks[bankSlot];
    combatResources.shapeResourceBanks[bankSlot]=BtechResourceBank{};
    return bank;
}

/*22B6:056A. Counts until the zero far pair, independent of cached count. */
uint16_t BtechCountShapeResourceBankEntries(uint16_t bankSlot)
{
    if (bankSlot&0x8000 || bankSlot>=combatResources.shapeResourceBankCount) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: bankSlot&0x8000 || bankSlot>=shapeResourceBankCount");
    const BtechResourceBank *bank=&combatResources.shapeResourceBanks[bankSlot];
    uint16_t count=0;
    if (bank->resources) while (bank->resources[count]) ++count;
    return count;
}

/*22B6:065E. Validating and storing the directory precedes counting it. */
void BtechBindShapeBankDirectory(BtechResourceBank bank,uint16_t bankSlot)
{
    if (bankSlot&0x8000 || bankSlot>=combatResources.shapeResourceBankCount || combatResources.shapeResourceBanks[bankSlot].resources) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: bankSlot&0x8000 || bankSlot>=shapeResourceBankCount || shapeResourceBanks[bankSlot].resources");
    combatResources.shapeResourceBanks[bankSlot]=bank;
    combatResources.shapeResourceBanks[bankSlot].resourceCount=BtechCountShapeResourceBankEntries(bankSlot);
}

/*22B6:0714. Null banks still subtract their counts. Signed live bank bound. */
const BtechShapeResource *BtechLookupBankResource(uint16_t resourceId)
{
    for (uint16_t slot=0;!(combatResources.shapeResourceBankCount&0x8000) && slot<combatResources.shapeResourceBankCount;++slot)
    {
        const BtechResourceBank *bank=&combatResources.shapeResourceBanks[slot];
        if (bank->resources && resourceId<bank->resourceCount) return bank->resources[resourceId];
        resourceId-=bank->resourceCount;
    }
    return NULL;
}

/*22B6:0780. Replace the live directory reference, not a copy of its header.
 * Null banks still consume their counts; selector:offset becomes a typed ref. */
void BtechSetShapeResourceEntryAddress(uint16_t resourceId,BtechShapeResource *resource)
{
    for (uint16_t slot=0;(int16_t)slot<(int16_t)combatResources.shapeResourceBankCount;++slot)
    {
        BtechResourceBank *bank=&combatResources.shapeResourceBanks[slot];
        if (bank->resources && resourceId<bank->resourceCount)
        {bank->resources[resourceId]=resource;return;}
        resourceId-=bank->resourceCount;
    }
    BtechShowFatalErrorAndExit("a shape number in the combat resources is out of range"); /*304E:2D92. */
}

/*22B6:1E28. Signed WORD scale, repeated DWORD SHL with wrap. */
uint32_t BtechGetResourceScaledValue(uint16_t resourceId)
{
    const BtechShapeResource *resource=BtechLookupBankResource(resourceId);
    uint32_t scale=0;
    if (resource)
    {
        scale=(uint32_t)resource->baseScale;
        for (uint8_t shifts=resource->scaleShift;shifts;--shifts) scale<<=1;
    }
    return scale;
}

/*22B6:1E64. Missing resource returns zero; the shift is an unsigned BYTE. */
uint16_t BtechGetResourceScaleShift(uint16_t resourceId)
{
    const BtechShapeResource *resource=BtechLookupBankResource(resourceId);
    return resource?resource->scaleShift:0;
}
