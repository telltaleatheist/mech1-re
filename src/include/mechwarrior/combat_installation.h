#ifndef MECHWARRIOR_COMBAT_INSTALLATION_H
#define MECHWARRIOR_COMBAT_INSTALLATION_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { BTECH_IMAGE_LOAD_SELECTOR=0x0810,BTECH_IMAGE_BYTES=0x2D2C0,
    BTECH_PACKED_BYTES=0x2A8BB,BTECH_DATA_IMAGE_OFFSET=0x283E0,
    BTECH_TONE_IMAGE_OFFSET=0x27DB0,BTECH_GEOMETRY_IMAGE_OFFSET=0x1AA60,
    BTECH_BSS_START=0x4EDC,BTECH_BSS_END=0xD5E0 };
/* The BTECH.EXE revision the story was imported from (story/import.json),
 * set by BtechHostLoadInitialData. Behaviour that differs between revisions
 * branches on this one switch; the branches are kept together in
 * BTECH_0810_INPUT.cpp. */
typedef enum BtechRevision { BTECH_REVISION_REFERENCE, BTECH_REVISION_1990_01_22 } BtechRevision;
extern BtechRevision btechLoadedRevision;
/* A revision's name as import.json records it, and back (false if unknown). */
const char *BtechRevisionName(BtechRevision revision);
bool BtechRevisionFromName(const char *name,BtechRevision *revision);
size_t BtechInstallationPackedBytes(size_t fileBytes); /* 0 if no known revision has this size */
/* Unpacks a known BTECH.EXE revision (by size and CRC) into a new image the
 * caller frees; the revision goes to `revision` (if not NULL). Only the story
 * import and the tests decode BTECH.EXE; play never opens it. */
bool BtechDecodeImage(const uint8_t *packed,size_t packedBytes,uint8_t **image,BtechRevision *revision);
/* Loads BTECH's initial data for a fresh EXEC, without the exe: the
 * Btech*InitialData importers read BTECH's start image (btech_start_image.h,
 * slices 6c and 6d), and the revision comes from the story. In the
 * importers, a zero (`name=0;` or a memset) followed by a bare offset in a
 * comment is a value the exe holds as zero at that offset of 304E: it is
 * set, not read (docs/phase2/PLAN.md 8, slice 6a). */
bool BtechHostLoadInitialData(void);
/* Host EXEC/CRT data projections, NOT additional native game procedures. */
void BtechInitializeFreshMechOwners(const uint8_t *data);
void BtechInitializeFreshWorldOwners(void);
void BtechInitializeFreshStartupOwners(const uint8_t *data);
void BtechHostDiscardChildData(void);
struct BtechResourceBank;
struct BtechSurfaceBank;
/* Host decoded allocations only; never release DOS paragraphs or run game logic. */
void BtechHostDiscardDecodedShapeBank(struct BtechResourceBank *bank);
void BtechHostDiscardDecodedSurfaceBank(struct BtechSurfaceBank *bank);
void BtechHostRetainDetachedShapeRegistry(void);
extern void (*btechHostReleaseChildDosAllocations)(void);
#endif
