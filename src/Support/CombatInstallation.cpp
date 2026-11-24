#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_installation.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_runtime.h"
#include "mechwarrior/combat_sound.h"
#include "mechwarrior/combat_heap.h"
#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_orders.h"
#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_pose.h"
#include "mechwarrior/combat_scene.h"
#include "mechwarrior/native_address.h"
#include "mechwarrior/btech_start_image.h"
#include "mechwarrior/story.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void (*btechHostReleaseChildDosAllocations)(void);

/* A saved vector the image carries must be 0000:0000 (H4c): the interrupt's
 * previous owner. Any other bytes name a handler this port has no owner for. */
static void RequireEmptySavedVector(const uint8_t *segment,uint16_t selector,uint16_t offset)
{
    if (!segment[offset] && !segment[offset+1] && !segment[offset+2] && !segment[offset+3]) return;
    char reason[112];
    snprintf(reason,sizeof(reason),"The image's saved vector at %04X:%04X holds %02X %02X %02X %02X, not 0000:0000",
        selector,offset,segment[offset],segment[offset+1],segment[offset+2],segment[offset+3]);
    MW_PORT_FAILSAFE(reason);
}

/*04FA overwrites its old near allocation without freeing it. Preserve that
 * lifetime through child disposal, without losing the host allocation owner. */
typedef struct RetiredShapeRegistry {
    BtechResourceBank *banks;
    struct RetiredShapeRegistry *next;
} RetiredShapeRegistry;
static RetiredShapeRegistry *retiredShapeRegistries;

void BtechHostRetainDetachedShapeRegistry(void)
{
    if (!combatResources.shapeResourceBanks || combatResources.shapeResourceBanks==&combatResources.initialShapeResourceBank) return;
    for (uint16_t bank=0;bank<combatResources.shapeResourceBankCount;++bank)
        if (combatResources.shapeResourceBanks[bank].resources || combatResources.shapeResourceBanks[bank].decodedResources) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: shapeResourceBanks[bank].resources || shapeResourceBanks[bank].decodedResources");
    RetiredShapeRegistry *retired=static_cast<RetiredShapeRegistry *>(malloc(sizeof(*retired)));if (!retired) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !retired");
    *retired=RetiredShapeRegistry{combatResources.shapeResourceBanks,retiredShapeRegistries};retiredShapeRegistries=retired;
}

/* Decoded host-only allocations have no counterpart in the native paragraph
 * block. Dispose these at its release boundary or child disposal, once. */
void BtechHostDiscardDecodedShapeBank(BtechResourceBank *bank)
{
    if (bank->decodedResources)
        for (BtechShapeResource *resource=bank->decodedResources;resource<bank->decodedResources+bank->decodedResourceCount;++resource)
        {
            if (resource->entries)
                for (int16_t entryIndex=0;entryIndex<resource->entryCount;++entryIndex)
                {
                    BtechRenderEntry *entry=&resource->entries[entryIndex];
                    if (entry->variants)
                        for (uint32_t variant=0;variant<(entry->variantCount?entry->variantCount:1);++variant)
                        {
                            BtechRenderVariant *lod=&entry->variants[variant];
                            if (!lod->primitives) continue;
                            int32_t count=lod->primitiveCount>0?lod->primitiveCount:1;
                            for (int32_t record=lod->primitiveCount==0?-1:0;record<count;++record)
                                if (lod->primitives[record].kind==RENDER_PRIMITIVE_POLYGON)
                                    free(lod->primitives[record].polygonPayload.polygons);
                            free(lod->primitives-1);
                        }
                    free(entry->variants);free(entry->vectors);
                }
            free(resource->entries);
        }
    free(bank->resources);free(bank->decodedResources);
    bank->resources=NULL;bank->decodedResources=NULL;bank->decodedResourceCount=0;
}

void BtechHostDiscardDecodedSurfaceBank(BtechSurfaceBank *bank)
{
    if (bank->surfaces)
        for (uint16_t index=0;index<bank->resourceCount;++index)
        {
            BtechSurfaceDefinition *surface=bank->surfaces[index];
            bool alias=false;
            for (uint16_t prior=0;prior<index;++prior) alias|=bank->surfaces[prior]==surface;
            if (!surface || alias) continue;
            for (uint16_t patch=0;patch<surface->patchCount;++patch) free(surface->patches[patch].boundaries);
            free(surface->patches);free(surface);
        }
    free(bank->surfaces);bank->surfaces=NULL;
}

/* DOS EXEC process disposal, not a new game cleanup routine. The host owns
 * paragraph allocations (including native orphan allocations); converted
 * references/near allocations are released here before reloading the child.
 * Never run this on campaign owners or reuse it as1337/DE52. */
void BtechHostDiscardChildData(void)
{
    bool paragraphs=false;
    for (uint16_t slot=0;slot<COMBAT_BITMAP_LISTS;++slot)
        if (combatVideo.bitmapRegistryLists[slot])
        {
            BtechBitmapList *list=combatVideo.bitmapRegistryLists[slot];
            paragraphs|=list->pixels.bytes!=NULL;
            for (uint16_t alias=slot;alias<COMBAT_BITMAP_LISTS;++alias)
                if (combatVideo.bitmapRegistryLists[alias]==list) combatVideo.bitmapRegistryLists[alias]=NULL;
            /* Near owners are retained until the whole child heap is disposed. */
        }
    combatVideo.promptBitmaps=NULL;
    combatVideo.pointPictures=NULL;combatVideo.smallMechPictures=NULL;combatVideo.strutPictures=NULL;
    combatVideo.cockpitPictures=NULL;combatVideo.hudGlyphPictures=NULL;combatVideo.inspectionDamagePictures=NULL;
    paragraphs|=orderPanelCaptureBuffer.bytes!=NULL;
    orderPanelCaptureBuffer=BtechResourceAllocation{};
    for (uint16_t slot=0;slot<RESOURCE_ARCHIVE_SLOTS;++slot)
        if (combatResources.archiveDescriptors[slot])
        {
            FILE *stream=combatResources.archiveDescriptors[slot]->stream;
            bool runtimeOwned=false;
            for (uint16_t file=0;file<COMBAT_STREAM_SLOTS;++file) runtimeOwned|=runtimeStreams[file].file==stream;
            if (stream && !runtimeOwned) fclose(stream);
            BtechReleaseArchiveDescriptor(slot);
        }
    BtechCloseRuntimeStreamFiles();
    for (uint16_t slot=0;slot<COMBAT_WORLD_OBJECTS;++slot)
        if (heapWorldObjectOwned[slot])
        {worldObjects[slot]=NULL;heapWorldObjectOwned[slot]=0;}
    for (uint16_t bank=0;bank<COMBAT_AUDIO_BANK_ROOTS;++bank)
    {
        /* DOS and near allocator owners include detached/reused nodes. Process
         * disposal must not follow a game's stale or cyclic next chain. */
        paragraphs|=soundBanks[bank].root.allocation.bytes!=NULL;
        free(soundBanks[bank].events);soundBanks[bank]=BtechSoundBank{};
    }
    for (uint32_t owner=0;owner<(uint32_t)combatResources.shapeResourceBankCount+1;++owner)
    {
        BtechResourceBank *bank;
        if (owner<combatResources.shapeResourceBankCount)
        {
            if (!combatResources.shapeResourceBanks || combatResources.shapeResourceBanks==&combatResources.initialShapeResourceBank) continue;
            bank=&combatResources.shapeResourceBanks[owner];
            if (bank->resources==combatResources.initialShapeResourceBank.resources) continue;
        }
        else bank=&combatResources.initialShapeResourceBank;
        paragraphs|=bank->allocation.bytes!=NULL;
        BtechHostDiscardDecodedShapeBank(bank);
    }
    while (retiredShapeRegistries)
    {
        RetiredShapeRegistry *retired=retiredShapeRegistries;retiredShapeRegistries=retired->next;
        free(retired); /* Bank arrays belong to the shared child near heap. */
    }
    paragraphs|=combatResources.surfaceResourceBank.allocation.bytes!=NULL;
    BtechHostDiscardDecodedSurfaceBank(&combatResources.surfaceResourceBank);
    combatResources.initialShapeResourceBank=BtechResourceBank{};combatResources.surfaceResourceBank=BtechSurfaceBank{};
    combatResources.shapeResourceBanks=NULL;combatResources.shapeResourceBankCount=0;
    for (uint16_t slot=1;slot<COMBAT_FONT_SLOTS;++slot) paragraphs|=combatVideo.fonts[slot].bytes!=NULL;
    paragraphs|=soundAllocationOverride.bytes!=NULL;
    if (paragraphs && !btechHostReleaseChildDosAllocations) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: paragraphs && !btechHostReleaseChildDosAllocations");
    if (btechHostReleaseChildDosAllocations) btechHostReleaseChildDosAllocations();
    for (uint16_t slot=0;slot<COMBAT_FONT_SLOTS;++slot) combatVideo.fonts[slot]=BtechFont{};
    combatVideo.egaFontBuffer=NULL; // Borrowed staging reference, not a second allocation owner.
    BtechHostDiscardNearHeap();
}

/* Host fresh EXEC image + DFCE BSS projection for converted owners. This is
 * still NOT native0019/1E84 entry and does not open the live SDL BTECH gate.
 * The native stack/tail outside4EDC..D5DF is never supplied as zero residue. */
bool BtechHostLoadInitialData(void)
{
    /* Play never opens BTECH.EXE: the revision is the one the story was
     * imported from, and the importers read BTECH's start image, built from
     * code, generated tables, zeros, the story and data/ (btech_start_image.h). */
    if (!BtechRevisionFromName(MwStoryBtechRevision(),&btechLoadedRevision))
    {
        fprintf(stderr,"MechWarrior: the story file names no known BTECH revision\n");
        return false;
    }
    uint8_t *image=static_cast<uint8_t *>(malloc(BTECH_IMAGE_BYTES));
    if (!image || !BtechBuildStartImage(image)) {free(image);return false;}
    uint8_t *data=static_cast<uint8_t *>(malloc(0x10000));
    if (!data) {free(image);return false;}
    memcpy(data,image+BTECH_DATA_IMAGE_OFFSET,BTECH_BSS_START);
    memset(data+BTECH_BSS_START,0,BTECH_BSS_END-BTECH_BSS_START);
    BtechHostDiscardChildData();
    BtechInitializeFreshMechOwners(data);BtechInitializeFreshWorldOwners(data);BtechInitializeFreshStartupOwners(data);
    BtechInitializeFreshSceneOwners();
    /* Reload writable CS owners as well as DS; handlers can patch their bytes. */
    const uint8_t *platform=image+(0x1BF5-BTECH_IMAGE_LOAD_SELECTOR)*0x10;
    const uint8_t *audio=image+(0x1ADF-BTECH_IMAGE_LOAD_SELECTOR)*0x10;
    /* The saved vectors the image carries (H4c): 0000:0000 in both
     * revisions, nothing saved yet, so each starts as its interrupt's
     * previous owner. INT 1Ch's (2BC4) is checked though the hook is gone. */
    RequireEmptySavedVector(platform,0x1BF5,0x2BC0);RequireEmptySavedVector(platform,0x1BF5,0x2BC4);
    RequireEmptySavedVector(platform,0x1BF5,0x253C);RequireEmptySavedVector(audio,0x1ADF,0x0E7D);
    combatHandlers.savedKeyboardOwner=MwKeyboardOwner::System;combatHandlers.savedTimerOwner=MwTimerOwner::Previous;combatSoundIrq.savedSampleIrqOwner=MwTimerOwner::Previous;
    combatHandlers.keyboardScanMask=platform[0x2D51];combatHandlers.keyboardExtendedScanMask=platform[0x2D5E];
    /* 0810:CE9A, INT 24's saved vector, is gone with that hook (H4c). It held
     * code bytes in the 1990 revision and zero in the reference: dead either
     * way, as startup saved INT 24 into it before any read. */
    bool loaded=BtechImportCombatStageInitialData(data,0x10000)
        && BtechImportWorldInitialData(data,0x10000,image+BTECH_GEOMETRY_IMAGE_OFFSET,BTECH_IMAGE_BYTES-BTECH_GEOMETRY_IMAGE_OFFSET)
        && BtechImportStartupInitialData(data,0x10000)
        && BtechImportSoundInitialData(data,0x10000)
        && BtechImportToneInitialData(data,0x10000,image+BTECH_TONE_IMAGE_OFFSET,BTECH_IMAGE_BYTES-BTECH_TONE_IMAGE_OFFSET)
        && BtechImportVideoInitialData(data,0x10000)
        && BtechImportOrderInitialData(data,0x10000)
        && BtechImportCockpitInitialData(data,0x10000)
        && BtechImportPoseInitialData(data,0x10000)
        && BtechImportRenderInitialData(data,0x10000)
        && BtechImportPolygonInitialData(data,0x10000);
    free(data);free(image);return loaded;
}
