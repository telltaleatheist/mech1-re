#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/read_word.h"
#include "mechwarrior/campaign_buffer.h"
#include "mechwarrior/campaign.h"
#include "mechwarrior/ega_campaign.h"
#include "mechwarrior/game_files.h"
#include <stdlib.h>

using mw1o::files::GameFile;
using mw1o::files::open_game_file;

/* MW_MAIN 081B:108C..113C. Directory read success does NOT require 600 bytes;
 * indexed record decode uses its STORED size, not the successful read count.
 * A failed open, read or seek is fatal (MwFailGameFileRead): no disk prompts. */
MwCampaignBuffer *MwLoadIndexedPicture(uint16_t pictureIndex)
{
    if (campaignCore.indexedPictureDirectoryLoaded != 0 && pictureIndex == campaignCore.indexedPictureCachedIndex)
        return campaignCore.indexedPictureOutput;
    GameFile file = open_game_file(indexedPictureFilename);
    if (!file) MwFailGameFileRead(indexedPictureFilename);
    if (campaignCore.indexedPictureDirectoryLoaded == 0)
    {
        if (!file.read(campaignCore.indexedPictureDirectory, INDEXED_PICTURE_DIRECTORY_BYTES))
            MwFailGameFileRead(indexedPictureFilename);
        campaignCore.indexedPictureDirectoryLoaded++;
    }
    campaignCore.indexedPictureCachedIndex = pictureIndex;
    uint16_t offsetEntry = campaignCore.indexedPictureCachedIndex * PICTURE_OFFSET_ENTRY_BYTES;
    /* Outside this proved directory object is unresolved live DS storage,
     * not a safe out-of-bounds C access or a silently clamped index. */
    if (offsetEntry + PICTURE_OFFSET_ENTRY_BYTES > INDEXED_PICTURE_DIRECTORY_BYTES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: offsetEntry + PICTURE_OFFSET_ENTRY_BYTES > INDEXED_PICTURE_DIRECTORY_BYTES");
    uint32_t fileOffset = ((uint32_t)MwReadWord(campaignCore.indexedPictureDirectory+offsetEntry) << NATIVE_WORD_BITS)
        | MwReadWord(campaignCore.indexedPictureDirectory+offsetEntry + PICTURE_BANK_LOW_OFFSET_WORD);
    if (!file.seek(fileOffset)) MwFailGameFileRead(indexedPictureFilename);
    uint16_t sizeEntry = INDEXED_PICTURE_SIZE_TABLE + (uint16_t)(campaignCore.indexedPictureCachedIndex * 2);
    if (sizeEntry + PICTURE_BANK_LOW_OFFSET_WORD > INDEXED_PICTURE_DIRECTORY_BYTES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: sizeEntry + PICTURE_BANK_LOW_OFFSET_WORD > INDEXED_PICTURE_DIRECTORY_BYTES");
    campaignCore.indexedPictureStoredBytes = MwReadWord(campaignCore.indexedPictureDirectory+sizeEntry);
    if (!file.read(MwCampaignBufferBytes(campaignCore.compressedPictureBuffer), campaignCore.indexedPictureStoredBytes))
        MwFailGameFileRead(indexedPictureFilename);
    /* 081B:113E..1152: a failed close is an unreadable file too. */
    if (!file.close()) MwFailGameFileRead(indexedPictureFilename);
    MwDecodePictureBank(campaignCore.indexedPictureStoredBytes, campaignCore.indexedPictureOutput);
    return campaignCore.indexedPictureOutput; /* ES result; native SI is zero. */
}
/* MW_MAIN 081B:113D. Genuine native RET-only hook, not a placeholder. */
void MwIndexedPictureCloseHook(void) { }
/* MW_MAIN 081B:1153..1188. A failed close is ignored, here and in the banks below. */
bool MwLoadGeneralPictureBank(void)
{
    GameFile file = open_game_file(generalPictureFilename);
    if (!file) return true;
    const std::optional<uint16_t> bytesRead = file.read(MwCampaignBufferBytes(campaignCore.compressedPictureBuffer), GENERAL_PICTURE_READ_BYTES);
    if (!bytesRead) return true;
    (void)file.close();
    MwDecodePictureBank(*bytesRead, campaignCore.generalPictureBank);
    return false;
}
/* MW_MAIN 081B:1189..11BE. */
bool MwLoadCockpitPictureBank(void)
{
    GameFile file = open_game_file(cockpitPictureFilename);
    if (!file) return true;
    const std::optional<uint16_t> bytesRead = file.read(MwCampaignBufferBytes(campaignCore.compressedPictureBuffer), WHOLE_PICTURE_READ_BYTES);
    if (!bytesRead) return true;
    (void)file.close();
    MwDecodePictureBank(*bytesRead, campaignCore.cockpitPictureBank);
    return false;
}
/* MW_MAIN 081B:11BF..11C1 / 11C2..11C5, shared entry/fallthrough. */
bool MwLoadPrimaryPictureBank(void) { return MwSelectAndLoadPictureBank(primaryPictureFilename); }
bool MwSelectAndLoadPictureBank(const char *filename)
{ campaignCore.selectedPictureFilename = filename; return MwLoadSelectedPictureBank(); }
/* MW_MAIN 081B:11C6..120D. */
bool MwLoadSelectedPictureBank(void)
{
    GameFile file = open_game_file(campaignCore.selectedPictureFilename);
    if (!file) MwFailGameFileRead(campaignCore.selectedPictureFilename);
    const std::optional<uint16_t> bytesRead = file.read(MwCampaignBufferBytes(campaignCore.compressedPictureBuffer), WHOLE_PICTURE_READ_BYTES);
    if (!bytesRead) return true;
    (void)file.close();
    MwDecodePictureBank(*bytesRead, campaignCore.activePictureBank);
    return false;
}
/* MW_MAIN 081B:120E..1243. */
bool MwLoadSecondaryPictureBank(void)
{
    GameFile file = open_game_file(secondaryPictureFilename);
    if (!file) return true;
    const std::optional<uint16_t> bytesRead = file.read(MwCampaignBufferBytes(campaignCore.compressedPictureBuffer), WHOLE_PICTURE_READ_BYTES);
    if (!bytesRead) return true;
    (void)file.close();
    MwDecodePictureBank(*bytesRead, campaignCore.secondaryPictureBank);
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
    uint8_t *pictureBank = MwCampaignBufferBytes(campaignCore.generalPictureBank);
    uint16_t pictureOffset = MwCampaignBufferWord(pictureBank,(uint16_t)((uint16_t)(pictureIndex * PICTURE_OFFSET_ENTRY_BYTES) + PICTURE_BANK_LOW_OFFSET_WORD));
    return EgaDrawPackedSprite(x, y, page, pictureOffset, campaignCore.generalPictureBank);
}
/* MW_MAIN081B:07B6..07CD. SI doubles twice as a WORD; only directory+02
 * is read.0165 holds MW_CPICS.BIN despite its earlier cockpit-bank name. */
uint16_t MwDrawAlternateBankPicture(uint16_t x, uint16_t y, uint8_t page, uint16_t pictureIndex)
{
    uint8_t *pictureBank=MwCampaignBufferBytes(campaignCore.cockpitPictureBank);
    uint16_t pictureOffset=MwCampaignBufferWord(pictureBank,(uint16_t)((uint16_t)(pictureIndex*PICTURE_OFFSET_ENTRY_BYTES)+PICTURE_BANK_LOW_OFFSET_WORD));
    return EgaDrawPackedSprite(x,y,page,pictureOffset,campaignCore.cockpitPictureBank);
}
/* MW_MAIN 081B:07CE..07E5. */
uint16_t MwDrawActivePicture(uint16_t x, uint16_t y, uint8_t page, uint16_t pictureIndex)
{
    uint8_t *pictureBank = MwCampaignBufferBytes(campaignCore.activePictureBank);
    uint16_t pictureOffset = MwCampaignBufferWord(pictureBank,(uint16_t)((uint16_t)(pictureIndex * PICTURE_OFFSET_ENTRY_BYTES) + PICTURE_BANK_LOW_OFFSET_WORD));
    return EgaDrawPackedSprite(x, y, page, pictureOffset, campaignCore.activePictureBank);
}
/* MW_MAIN 081B:07E6..07FD. The native bank identity is selector0169; keep the
 * neutral ordinal-era name until its installed filename role is proven. */
uint16_t MwDrawFourthBankPicture(uint16_t x, uint16_t y, uint8_t page, uint16_t pictureIndex)
{
    uint8_t *pictureBank=MwCampaignBufferBytes(campaignCore.secondaryPictureBank);
    uint16_t pictureOffset=MwCampaignBufferWord(pictureBank,(uint16_t)((uint16_t)(pictureIndex*PICTURE_OFFSET_ENTRY_BYTES)+PICTURE_BANK_LOW_OFFSET_WORD));
    return EgaDrawPackedSprite(x,y,page,pictureOffset,campaignCore.secondaryPictureBank);
}
