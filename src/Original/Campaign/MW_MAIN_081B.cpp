#include "mechwarrior/campaign.h"
#include "mechwarrior/native_address.h"

/* Original MW_MAIN 081B:0C17..0C3D. Used by stock, contracts, recruits and
 * planet selection: CLC; RCR AX twice; RCL BX five; XOR; high WORD of MUL
 * by wrapping (maximum+1). Carry participates in both rotations. */
uint16_t MwRandomUpTo(uint16_t maximum)
{
    uint16_t rotatedWord1 = (campaignCore.randomStateWord1 >> RANDOM_WORD1_SHIFT)
        | ((campaignCore.randomStateWord1 & 1) != 0 ? NATIVE_WORD_HIGH_BIT : 0);
    uint16_t carryIntoWord0 = (campaignCore.randomStateWord1 >> 1) & 1;
    campaignCore.randomStateWord0 = (uint16_t)(((uint32_t)campaignCore.randomStateWord0 << RANDOM_WORD0_ROTATE)
        | (carryIntoWord0 << RANDOM_CARRY_INSERT_SHIFT)
        | (campaignCore.randomStateWord0 >> RANDOM_ROTATE_REINSERT_SHIFT));

    campaignCore.randomStateWord1 = rotatedWord1 ^ campaignCore.randomStateWord0;
    uint16_t rangeSize = maximum + 1;
    return (uint16_t)(((uint32_t)campaignCore.randomStateWord1 * rangeSize) >> NATIVE_WORD_BITS);
}

/* Original MW_MAIN 081B:12B8..12FD. Whole-bank and indexed-picture callers
 * share this decoder. No size clamp, output clear or bounded replacement RLE:
 * native cursor wrap and partial work remain. */
void MwDecodePictureBank(uint16_t compressedByteCount, MwCampaignBuffer *output)
{
    campaignCore.pictureCompressedByteCount = compressedByteCount;
    campaignCore.pictureOutput = output;
    campaignCore.pictureReadHighNibbleNext = 0;
    campaignCore.pictureReadByteOffset = 0;
    campaignCore.pictureWriteLowNibbleNext = 0;
    campaignCore.pictureWriteByteOffset = 0;

    uint8_t token;
    while (!MwReadPictureNibble(&token))
    {
        if (token != PICTURE_RLE_ESCAPE)
        {
            MwWritePictureNibble(token);
            continue;
        }
        uint8_t repeatCountMinusOne;
        uint8_t repeatedNibble;
        if (MwReadPictureNibble(&repeatCountMinusOne))
            return;
        uint16_t repeatCount = repeatCountMinusOne + 1;
        if (MwReadPictureNibble(&repeatedNibble))
            return;
        do
        {
            MwWritePictureNibble(repeatedNibble);
            --repeatCount;
        } while (repeatCount != 0);
    }
}

/* Original MW_MAIN 081B:12FE..1338. Carry result becomes bool; AL is written
 * even at EOF. The byte read precedes the check and the WORD cursor wraps: at a
 * picture's end the low nibble of the byte after its data is decoded too. That byte
 * is in the buffer whatever is loaded (a count is a WORD, the buffer 10000h bytes;
 * the largest file, 7F51h bytes, leaves 80AFh): zero after a first load, an earlier
 * picture's after it. */
bool MwReadPictureNibble(uint8_t *nibble)
{
    uint8_t *compressedData = MwCampaignBufferBytes(campaignCore.compressedPictureBuffer);
    uint8_t packedNibbles = compressedData[campaignCore.pictureReadByteOffset];
    if (campaignCore.pictureReadHighNibbleNext == 0)
    {
        *nibble = packedNibbles & PICTURE_NIBBLE_MASK;
        campaignCore.pictureReadHighNibbleNext = 1;
        return false;
    }
    *nibble = packedNibbles >> PICTURE_NIBBLE_SHIFT;
    campaignCore.pictureReadHighNibbleNext = 0;
    campaignCore.pictureReadByteOffset++;
    return campaignCore.pictureReadByteOffset > campaignCore.pictureCompressedByteCount;
}

/* Original MW_MAIN 081B:1339..136B. First write replaces the entire byte;
 * second ORs the low nibble. An unfinished byte is high-only, not stale RAM. The
 * nibble decoded past the end lands here too, after the picture; the WORD cursor
 * wraps inside the buffer (the largest bank, FB70h bytes, leaves 490h). */
void MwWritePictureNibble(uint8_t nibble)
{
    uint8_t *outputData = MwCampaignBufferBytes(campaignCore.pictureOutput);
    if (campaignCore.pictureWriteLowNibbleNext == 0)
    {
        outputData[campaignCore.pictureWriteByteOffset] = (uint8_t)((nibble << PICTURE_NIBBLE_SHIFT)
            & PICTURE_HIGH_NIBBLE_MASK);
        campaignCore.pictureWriteLowNibbleNext = 1;
    }
    else
    {
        outputData[campaignCore.pictureWriteByteOffset] |= nibble & PICTURE_NIBBLE_MASK;
        campaignCore.pictureWriteByteOffset++;
        campaignCore.pictureWriteLowNibbleNext = 0;
    }
}
