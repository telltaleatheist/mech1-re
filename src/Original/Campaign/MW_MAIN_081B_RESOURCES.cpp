#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/campaign.h"
#include "mechwarrior/dos_files.h"
#include "mechwarrior/ega_campaign.h"
#include <stdlib.h>

/* MW_MAIN 081B:1056..105E / 105F..1065 / 106D..1073 / 1083..108B.
 * AX/CX:DX register transport becomes actual arguments; Carry true is error. */
bool MwDosOpenResourceReadWrite(const char *filename, uint16_t *handleOrError)
{ return mwDosFiles.openReadWrite(filename, handleOrError); }
bool MwDosReadFile(uint8_t *buffer, uint16_t byteCount, uint16_t fileHandle, uint16_t *bytesReadOrError)
{ return mwDosFiles.read(buffer, byteCount, fileHandle, bytesReadOrError); }
bool MwDosCloseFile(uint16_t fileHandle) { return mwDosFiles.close(fileHandle); }
bool MwDosSeekAbsolute(uint32_t fileOffset, uint16_t fileHandle)
{ return mwDosFiles.seekAbsolute(fileOffset, fileHandle); }

/* MW_MAIN 081B:108C..113C. Directory read success does NOT require 600 bytes;
 * indexed record decode uses its STORED size, not the successful read count.
 * A failed open, read or seek is fatal (MwFailGameFileRead): no disk prompts. */
uint16_t MwLoadIndexedPicture(uint16_t pictureIndex)
{
    uint16_t handleOrError, bytesReadOrError;
    if (campaignCore.indexedPictureDirectoryLoaded == 0)
    {
        while (MwDosOpenResourceReadWrite(indexedPictureFilename, &handleOrError))
            MwFailGameFileRead(indexedPictureFilename);
        campaignCore.indexedPictureFileHandle = handleOrError;
        while (MwDosReadFile(campaignCore.indexedPictureDirectory, INDEXED_PICTURE_DIRECTORY_BYTES,
            campaignCore.indexedPictureFileHandle, &bytesReadOrError)) MwFailGameFileRead(indexedPictureFilename);
        campaignCore.indexedPictureDirectoryLoaded++;
    }
    else
    {
        if (pictureIndex == campaignCore.indexedPictureCachedIndex) return campaignCore.indexedPictureOutputSelector;
        while (MwDosOpenResourceReadWrite(indexedPictureFilename, &handleOrError))
            MwFailGameFileRead(indexedPictureFilename);
        campaignCore.indexedPictureFileHandle = handleOrError;
    }
    campaignCore.indexedPictureCachedIndex = pictureIndex;
    for (;;)
    {
        uint16_t offsetEntry = campaignCore.indexedPictureCachedIndex * PICTURE_OFFSET_ENTRY_BYTES;
        /* Outside this proved directory object is unresolved live DS storage,
         * not a safe out-of-bounds C access or a silently clamped index. */
        if (offsetEntry + PICTURE_OFFSET_ENTRY_BYTES > INDEXED_PICTURE_DIRECTORY_BYTES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: offsetEntry + PICTURE_OFFSET_ENTRY_BYTES > INDEXED_PICTURE_DIRECTORY_BYTES");
        uint32_t fileOffset = ((uint32_t)MwReadNativeWord(campaignCore.indexedPictureDirectory, offsetEntry) << NATIVE_WORD_BITS)
            | MwReadNativeWord(campaignCore.indexedPictureDirectory, (uint16_t)(offsetEntry + PICTURE_BANK_LOW_OFFSET_WORD));
        if (!MwDosSeekAbsolute(fileOffset, campaignCore.indexedPictureFileHandle)) break;
        MwFailGameFileRead(indexedPictureFilename);
    }
    for (;;)
    {
        uint16_t sizeEntry = INDEXED_PICTURE_SIZE_TABLE + (uint16_t)(campaignCore.indexedPictureCachedIndex * 2);
        if (sizeEntry + PICTURE_BANK_LOW_OFFSET_WORD > INDEXED_PICTURE_DIRECTORY_BYTES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: sizeEntry + PICTURE_BANK_LOW_OFFSET_WORD > INDEXED_PICTURE_DIRECTORY_BYTES");
        campaignCore.indexedPictureStoredBytes = MwReadNativeWord(campaignCore.indexedPictureDirectory, sizeEntry);
        if (!MwDosReadFile(MwResolveNativeSegment(campaignCore.compressedPictureBufferSelector),
            campaignCore.indexedPictureStoredBytes, campaignCore.indexedPictureFileHandle, &bytesReadOrError)) break;
        MwFailGameFileRead(indexedPictureFilename);
    }
    MwCloseIndexedPictureBank();
    MwDecodePictureBank(campaignCore.indexedPictureStoredBytes, campaignCore.indexedPictureOutputSelector);
    return campaignCore.indexedPictureOutputSelector; /* ES result; native SI is zero. */
}
/* MW_MAIN 081B:113D. Genuine native RET-only hook, not a placeholder. */
void MwIndexedPictureCloseHook(void) { }
/* MW_MAIN 081B:113E..1152. JNC skips the prompt on SUCCESS. Stored handle is
 * never cleared, including close failure; do not repair native ownership here. */
void MwCloseIndexedPictureBank(void)
{
    if (campaignCore.indexedPictureFileHandle == 0) return;
    if (!MwDosCloseFile(campaignCore.indexedPictureFileHandle)) return;
    MwFailGameFileRead(indexedPictureFilename);
    MwIndexedPictureCloseHook(); /* Native shared RET tail. */
}
/* MW_MAIN 081B:1153..1188. Read failure leaks the handle; close failure is ignored. */
bool MwLoadGeneralPictureBank(void)
{
    uint16_t fileHandleOrError, bytesReadOrError;
    if (MwDosOpenResourceReadWrite(generalPictureFilename, &fileHandleOrError)) return true;
    campaignCore.generalPictureFileHandle = fileHandleOrError;
    if (MwDosReadFile(MwResolveNativeSegment(campaignCore.compressedPictureBufferSelector),
        GENERAL_PICTURE_READ_BYTES, fileHandleOrError, &bytesReadOrError)) return true;
    MwDosCloseFile(campaignCore.generalPictureFileHandle);
    MwDecodePictureBank(bytesReadOrError, campaignCore.generalPictureBankSelector);
    return false;
}
/* MW_MAIN 081B:1189..11BE. */
bool MwLoadCockpitPictureBank(void)
{
    uint16_t fileHandleOrError, bytesReadOrError;
    if (MwDosOpenResourceReadWrite(cockpitPictureFilename, &fileHandleOrError)) return true;
    campaignCore.selectedPictureFileHandle = fileHandleOrError;
    if (MwDosReadFile(MwResolveNativeSegment(campaignCore.compressedPictureBufferSelector),
        WHOLE_PICTURE_READ_BYTES, fileHandleOrError, &bytesReadOrError)) return true;
    MwDosCloseFile(campaignCore.selectedPictureFileHandle);
    MwDecodePictureBank(bytesReadOrError, campaignCore.cockpitPictureBankSelector);
    return false;
}
/* MW_MAIN 081B:11BF..11C1 / 11C2..11C5, shared entry/fallthrough. */
bool MwLoadPrimaryPictureBank(void) { return MwSelectAndLoadPictureBank(primaryPictureFilename); }
bool MwSelectAndLoadPictureBank(const char *filename)
{ campaignCore.selectedPictureFilename = filename; return MwLoadSelectedPictureBank(); }
/* MW_MAIN 081B:11C6..120D. */
bool MwLoadSelectedPictureBank(void)
{
    uint16_t fileHandleOrError, bytesReadOrError;
    while (MwDosOpenResourceReadWrite(campaignCore.selectedPictureFilename, &fileHandleOrError))
    {
        MwFailGameFileRead(campaignCore.selectedPictureFilename);
    }
    campaignCore.selectedPictureFileHandle = fileHandleOrError;
    if (MwDosReadFile(MwResolveNativeSegment(campaignCore.compressedPictureBufferSelector),
        WHOLE_PICTURE_READ_BYTES, fileHandleOrError, &bytesReadOrError)) return true;
    MwDosCloseFile(campaignCore.selectedPictureFileHandle);
    MwDecodePictureBank(bytesReadOrError, campaignCore.activePictureBankSelector);
    return false;
}
/* MW_MAIN 081B:120E..1243. */
bool MwLoadSecondaryPictureBank(void)
{
    uint16_t fileHandleOrError, bytesReadOrError;
    if (MwDosOpenResourceReadWrite(secondaryPictureFilename, &fileHandleOrError)) return true;
    campaignCore.selectedPictureFileHandle = fileHandleOrError;
    if (MwDosReadFile(MwResolveNativeSegment(campaignCore.compressedPictureBufferSelector),
        WHOLE_PICTURE_READ_BYTES, fileHandleOrError, &bytesReadOrError)) return true;
    MwDosCloseFile(campaignCore.selectedPictureFileHandle);
    MwDecodePictureBank(bytesReadOrError, campaignCore.secondaryPictureBankSelector);
    return false;
}
/* MW_MAIN 081B:1244..1249 / 124A..124F / 1250..1255: native near tails. */
bool MwLoadTPictureBank(void) { return MwSelectAndLoadPictureBank(tPictureFilename); }
bool MwLoadAlternatePictureBank(void) { return MwSelectAndLoadPictureBank(alternatePictureFilename); }
bool MwLoadFPictureBank(void) { return MwSelectAndLoadPictureBank(fPictureFilename); }

/* MW_MAIN 081B:078D..079D; saved coordinates, ES:0000, pageCL01. */
void MwDrawIndexedPicture(uint16_t x, uint16_t y, uint16_t pictureIndex)
{
    EgaDrawPackedSprite(x, y, EGA_DRAW_PAGE, 0, MwLoadIndexedPicture(pictureIndex));
}
/* MW_MAIN 081B:079E..07B5. Only the LOW offset WORD is consumed. */
uint16_t MwDrawGeneralPicture(uint16_t x, uint16_t y, uint8_t page, uint16_t pictureIndex)
{
    uint8_t *pictureBank = MwResolveNativeSegment(campaignCore.generalPictureBankSelector);
    uint16_t pictureOffset = MwReadNativeWord(pictureBank, (uint16_t)((uint16_t)(pictureIndex * PICTURE_OFFSET_ENTRY_BYTES) + PICTURE_BANK_LOW_OFFSET_WORD));
    return EgaDrawPackedSprite(x, y, page, pictureOffset, campaignCore.generalPictureBankSelector);
}
/* MW_MAIN081B:07B6..07CD. SI doubles twice as a WORD; only directory+02
 * is read.0165 holds MW_CPICS.BIN despite its earlier cockpit-bank name. */
uint16_t MwDrawAlternateBankPicture(uint16_t x, uint16_t y, uint8_t page, uint16_t pictureIndex)
{
    uint8_t *pictureBank=MwResolveNativeSegment(campaignCore.cockpitPictureBankSelector);
    uint16_t pictureOffset=MwReadNativeWord(pictureBank,(uint16_t)((uint16_t)(pictureIndex*PICTURE_OFFSET_ENTRY_BYTES)+PICTURE_BANK_LOW_OFFSET_WORD));
    return EgaDrawPackedSprite(x,y,page,pictureOffset,campaignCore.cockpitPictureBankSelector);
}
/* MW_MAIN 081B:07CE..07E5. */
uint16_t MwDrawActivePicture(uint16_t x, uint16_t y, uint8_t page, uint16_t pictureIndex)
{
    uint8_t *pictureBank = MwResolveNativeSegment(campaignCore.activePictureBankSelector);
    uint16_t pictureOffset = MwReadNativeWord(pictureBank, (uint16_t)((uint16_t)(pictureIndex * PICTURE_OFFSET_ENTRY_BYTES) + PICTURE_BANK_LOW_OFFSET_WORD));
    return EgaDrawPackedSprite(x, y, page, pictureOffset, campaignCore.activePictureBankSelector);
}
/* MW_MAIN 081B:07E6..07FD. The native bank identity is selector0169; keep the
 * neutral ordinal-era name until its installed filename role is proven. */
uint16_t MwDrawFourthBankPicture(uint16_t x, uint16_t y, uint8_t page, uint16_t pictureIndex)
{
    uint8_t *pictureBank=MwResolveNativeSegment(campaignCore.secondaryPictureBankSelector);
    uint16_t pictureOffset=MwReadNativeWord(pictureBank,
        (uint16_t)((uint16_t)(pictureIndex*PICTURE_OFFSET_ENTRY_BYTES)+PICTURE_BANK_LOW_OFFSET_WORD));
    return EgaDrawPackedSprite(x,y,page,pictureOffset,campaignCore.secondaryPictureBankSelector);
}
