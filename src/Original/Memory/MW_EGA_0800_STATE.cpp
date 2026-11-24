#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/ega_campaign.h"
#include "mechwarrior/original_data.h"
#include "mechwarrior/story.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
EgaOverlayState egaOverlay;

/* Two native font destinations overlap. Keep ONE bitmap byte owner; do not
 * create separate small/large font copies. Other unidentified/code bytes remain
 * byte ranges only for native-image import and unchecked adjacent reads.
 * These are not executable host code. */
static uint8_t egaEntryCodeBytes[0x0058];
static uint8_t egaPageFlipCodeBytes[0x0047];
static uint8_t egaPaletteCodeBytes[0x004E];
static uint8_t egaPixelReadCodeBytes[0x0070];
static uint8_t egaPackedSpriteCodeBytes[0x00C3];
static uint8_t egaCharacterCursorCodeBytes[0x0004];
static uint8_t egaCharacterCodeBytes[0x0113];
static uint8_t overlayByte;
static uint8_t egaTextCodeBytes[0x00FC];
static uint8_t egaTextColourCodeBytes[0x010E];
static uint8_t egaFontLoadCodeBytes[0x0051];
static uint8_t egaFontSelectCodeBytes[0x0025];
static uint8_t egaRectangleCopyCodeBytes[0x00EA];
static uint8_t egaLineCodeBytes[0x019D];
static uint8_t egaTailCodeBytes[0xEC32];

/* Representation bridge only: binary IO and out-of-table native offsets must
 * see the same canonical objects as direct assignments. WORDs are projected
 * little-endian explicitly, independent of host alignment or byte order.
 * This table owns no state and is not an additional game procedure. */
typedef struct
{
    uint16_t offset;
    uint32_t byteCount;
    uint8_t *bytes;
    uint16_t *words;
} EgaOverlayByteSpan;
static const EgaOverlayByteSpan overlayByteSpans[] =
{
    { 0x0000, 0x0058, egaEntryCodeBytes, NULL },
    { 0x0058, 0x0002, NULL, &egaOverlay.egaDisplayPage },
    { 0x005A, 0x0002, NULL, &egaOverlay.egaDrawPage },
    { 0x005C, 0x0047, egaPageFlipCodeBytes, NULL },
    { 0x00A3, 0x0190, NULL, egaOverlay.scanlineOffsets },
    { 0x0233, 0x004E, egaPaletteCodeBytes, NULL },
    { 0x0281, 0x0280, NULL, egaOverlay.xByteOffsets },
    { 0x0501, 0x0140, egaOverlay.xPixelMasks, NULL },
    { 0x0641, 0x0070, egaPixelReadCodeBytes, NULL },
    { 0x06B1, 0x0002, NULL, &egaOverlay.spriteWidth },
    { 0x06B3, 0x0002, NULL, &egaOverlay.spriteRowsRemaining },
    { 0x06B5, 0x00C3, egaPackedSpriteCodeBytes, NULL },
    { 0x0778, 0x0002, NULL, &egaOverlay.glyphX },
    { 0x077A, 0x0002, NULL, &egaOverlay.glyphY },
    { 0x077C, 0x0002, NULL, &egaOverlay.characterCode },
    { 0x077E, 0x0004, egaCharacterCursorCodeBytes, NULL },
    { 0x0782, 0x0001, &egaOverlay.glyphForeground, NULL },
    { 0x0783, 0x0001, &egaOverlay.glyphBackground, NULL },
    { 0x0784, 0x0001, &egaOverlay.glyphBitOffset, NULL },
    { 0x0785, 0x0002, NULL, &egaOverlay.selectedFontHeight },
    { 0x0787, 0x0002, NULL, &egaOverlay.selectedGlyphAdvance },
    { 0x0789, 0x0002, NULL, &egaOverlay.selectedFontBitmapOffset },
    { 0x078B, 0x0113, egaCharacterCodeBytes, NULL },
    { 0x089E, 2, NULL, &egaOverlay.rectangleX }, { 0x08A0, 2, NULL, &egaOverlay.rectangleY },
    { 0x08A2, 2, NULL, &egaOverlay.rectangleWidth }, { 0x08A4, 2, NULL, &egaOverlay.rectangleHeight },
    { 0x08A6, 2, NULL, &egaOverlay.rectangleColour },
    { 0x08A8, 1, &egaOverlay.firstByteMask, NULL }, { 0x08A9, 1, &egaOverlay.lastByteMask, NULL },
    { 0x08AA, 2, NULL, &egaOverlay.firstByteWidth }, { 0x08AC, 2, NULL, &egaOverlay.lastByteWidth },
    { 0x08AE, 8, egaOverlay.rectangleLeadingMasks, NULL }, { 0x08B6, 1, &overlayByte, NULL },
    { 0x08B7, 8, egaOverlay.rectangleTrailingMasks, NULL }, { 0x08BF, 0x00FC, egaTextCodeBytes, NULL },
    { 0x09BB, 0x0002, NULL, &egaOverlay.textStartX },
    { 0x09BD, 0x0002, NULL, &egaOverlay.textCurrentY },
    { 0x09BF, 0x0001, &egaOverlay.egaTextColour, NULL },
    { 0x09C0, 0x0002, NULL, &egaOverlay.textExtraLeading },
    { 0x09C2, 0x010E, egaTextColourCodeBytes, NULL },
    { 0x0AD0, 0x0002, NULL, &egaOverlay.fontReadBufferOffset },
    { 0x0AD2, 0x0002, NULL, &egaOverlay.fontHandleBytes },
    { 0x0AD4, 0x001A, (uint8_t *)egaOverlay.smallFontFilename, NULL },
    { 0x0AEE, 0x0015, (uint8_t *)egaOverlay.largeFontFilename, NULL },
    { 0x0B03, 0x057B, egaOverlay.fontBitmapStorage, NULL },
    { 0x107E, 0x0051, egaFontLoadCodeBytes, NULL },
    { 0x10CF, 0x0002, NULL, &egaOverlay.selectedFontIndex },
    { 0x10D1, 0x000A, NULL, egaOverlay.egaFontHeights },
    { 0x10DB, 0x000A, NULL, egaOverlay.fontAdvances },
    { 0x10E5, 0x000A, NULL, egaOverlay.fontBitmapOffsets },
    { 0x10EF, 0x0025, egaFontSelectCodeBytes, NULL },
    { 0x1114, 2, NULL, &egaOverlay.copyRectangleX }, { 0x1116, 2, NULL, &egaOverlay.copyRectangleY },
    { 0x1118, 2, NULL, &egaOverlay.copyRectangleWidth }, { 0x111A, 2, NULL, &egaOverlay.copyRectangleHeight },
    { 0x111C, 0x00EA, egaRectangleCopyCodeBytes, NULL },
    { 0x1206, 1, &egaOverlay.egaLineColour, NULL },
    { 0x1207, 2, NULL, &egaOverlay.egaCurrentX },{ 0x1209, 2, NULL, &egaOverlay.currentY },
    { 0x120B, 2, NULL, &egaOverlay.egaFinalX },{ 0x120D, 2, NULL, &egaOverlay.egaFinalY },
    { 0x120F, 0x019D, egaLineCodeBytes, NULL },
    { 0x13AC, EGA_PALETTE_ENTRIES, egaOverlay.defaultPaletteCodes, NULL },
    { 0x13BD, EGA_PALETTE_ENTRIES, egaOverlay.alternatePaletteCodes, NULL },
    { 0x13CE, 0xEC32, egaTailCodeBytes, NULL },
};

/* MW_EGA's start state (docs/phase2/PLAN.md 8.5, slice 6e): every overlay
 * owner as the exe holds it when the overlay loads, set without reading the
 * exe. MW_EGA.EXE has one revision (both game copies hold the same file).
 * - The pixel tables are generated (x>>3, 0x80>>(x&7)), the rectangle edge
 *   masks too; the scanline table is zero (startup's ordinal 01 builds it).
 * - The pages, the selected font, the font table and the two palettes are
 *   code constants (decision 1, platform tables); the font file names are
 *   file names (class E). The exe's bytes after each name's NUL are never
 *   read (the open copies up to the NUL) and start zero here.
 * - The machine-code spans are zero. An unchecked read reaches them only
 *   past a table; over the 8 traces play reads one word there, the row-200
 *   scanline lookup's (0233), which EgaLoadStartStoryBytes takes from the
 *   story (measured, PLAN 8.5). The exe holds an offset past the page there;
 *   zero would draw on row 0.
 * - Everything else is zero in the exe and zero here.
 * start_exe_test compares every byte with MW_EGA.EXE but the code spans and
 * the name tails. Names and numbers only: no MW1 text. */
static const char kSmallFontFilename[8]="\x36\x58\x36\x2E\x46\x4E\x54";
static const char kLargeFontFilename[10]="\x46\x4F\x58\x38\x38\x2E\x46\x4E\x54";
static const uint16_t kFontHeights[5]={0x0006,0x0008,0x0008,0x0008,0x0008};
static const uint16_t kFontAdvances[5]={0x0006,0x0008,0x0008,0x0008,0x0008};
static const uint16_t kFontBitmapOffsets[5]={0x0B03,0x0D5B,0x0D5C,0x0D5D,0x107D};
static const uint8_t kDefaultPaletteCodes[EGA_PALETTE_ENTRIES]={0x05,0x00,0x17,0x07,0x10,0x13,0x11,0x01,0x14,0x04,0x12,0x03,0x02,0x16,0x06,0x15,0x00};
static const uint8_t kAlternatePaletteCodes[EGA_PALETTE_ENTRIES]={0x00,0x10,0x17,0x03,0x15,0x05,0x07,0x13,0x07,0x07,0x13,0x17,0x07,0x07,0x07,0x07,0x00};

void EgaLoadStartState(void)
{
    for (size_t spanIndex = 0; spanIndex < sizeof(overlayByteSpans) / sizeof(overlayByteSpans[0]); ++spanIndex)
    {
        const EgaOverlayByteSpan *span = &overlayByteSpans[spanIndex];
        if (span->words != NULL) memset(span->words, 0, span->byteCount);
        else memset(span->bytes, 0, span->byteCount);
    }
    egaOverlay.egaDisplayPage = EGA_FIRST_PAGE_SELECTOR;
    egaOverlay.egaDrawPage = EGA_SECOND_PAGE_SELECTOR;
    for (uint16_t x = 0; x < EGA_COLUMNS; ++x)
    {
        egaOverlay.xByteOffsets[x] = (uint16_t)(x >> EGA_PIXEL_BYTE_SHIFT);
        egaOverlay.xPixelMasks[x] = (uint8_t)(EGA_BYTE_HIGH_BIT >> (x & EGA_BYTE_LAST_BIT));
    }
    for (uint16_t bit = 0; bit < EGA_PIXELS_PER_BYTE; ++bit)
    {
        egaOverlay.rectangleLeadingMasks[bit] = (uint8_t)(0xFF >> bit);
        egaOverlay.rectangleTrailingMasks[bit] = (uint8_t)(0xFF00 >> bit);
    }
    egaOverlay.selectedFontHeight = kFontHeights[0];
    egaOverlay.selectedGlyphAdvance = kFontAdvances[0];
    egaOverlay.selectedFontBitmapOffset = kFontBitmapOffsets[0];
    memcpy(egaOverlay.smallFontFilename, kSmallFontFilename, sizeof(kSmallFontFilename));
    memcpy(egaOverlay.largeFontFilename, kLargeFontFilename, sizeof(kLargeFontFilename));
    memcpy(egaOverlay.egaFontHeights, kFontHeights, sizeof(kFontHeights));
    memcpy(egaOverlay.fontAdvances, kFontAdvances, sizeof(kFontAdvances));
    memcpy(egaOverlay.fontBitmapOffsets, kFontBitmapOffsets, sizeof(kFontBitmapOffsets));
    memcpy(egaOverlay.defaultPaletteCodes, kDefaultPaletteCodes, sizeof(kDefaultPaletteCodes));
    memcpy(egaOverlay.alternatePaletteCodes, kAlternatePaletteCodes, sizeof(kAlternatePaletteCodes));
}
/* The row-200 word from the story (ega.row_200_offset, story/native.json):
 * the bytes past the scanline table, which play reads. False if the story
 * has no such span. */
bool EgaLoadStartStoryBytes(void)
{
    return MwStoryCopyBytes("ega.row_200_offset", egaPaletteCodeBytes, EGA_WORD_BYTES);
}
/* Play never opens MW_EGA.EXE: the overlay's owners start as the exe holds
 * them, from code (slice 6e). */
bool MwHostLoadEgaInitialData(void)
{
    EgaLoadStartState();
    return EgaLoadStartStoryBytes();
}
/* The overlay's machine-code spans and the font names' unread tails, which
 * the start state leaves zero (start_exe_test skips them; the row-200 word
 * at 0233 is the story's, filled after it). */
const EgaOverlayRange egaStartZeroedRanges[] =
{
    { 0x0000, 0x0058 }, { 0x005C, 0x0047 }, { 0x0233, 0x004E }, { 0x0641, 0x0070 }, { 0x06B5, 0x00C3 },
    { 0x077E, 0x0004 }, { 0x078B, 0x0113 }, { 0x08BF, 0x00FC }, { 0x09C2, 0x010E },
    { 0x0AD4 + 8, 0x001A - 8 }, { 0x0AEE + 10, 0x0015 - 10 },
    { 0x107E, 0x0051 }, { 0x10EF, 0x0025 }, { 0x111C, 0x00EA }, { 0x120F, 0x019D }, { 0x13CE, 0x0011 },
};
const size_t egaStartZeroedRangeCount = sizeof(egaStartZeroedRanges) / sizeof(egaStartZeroedRanges[0]);

uint8_t EgaReadOverlayByte(uint16_t offset)
{
    for (size_t spanIndex = 0; spanIndex < sizeof(overlayByteSpans) / sizeof(overlayByteSpans[0]); ++spanIndex)
    {
        const EgaOverlayByteSpan *span = &overlayByteSpans[spanIndex];
        if (offset < span->offset || (uint32_t)offset - span->offset >= span->byteCount) continue;
        uint16_t relativeOffset = offset - span->offset;
        if (span->words != NULL)
            return (uint8_t)(span->words[relativeOffset / 2] >> ((relativeOffset & 1) * 8));
        return span->bytes[relativeOffset];
    }
    /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Layout bridge bug, not a new native validation."); /* Layout bridge bug, not a new native validation. */
}
void EgaWriteOverlayByte(uint16_t offset, uint8_t value)
{
    for (size_t spanIndex = 0; spanIndex < sizeof(overlayByteSpans) / sizeof(overlayByteSpans[0]); ++spanIndex)
    {
        const EgaOverlayByteSpan *span = &overlayByteSpans[spanIndex];
        if (offset < span->offset || (uint32_t)offset - span->offset >= span->byteCount) continue;
        uint16_t relativeOffset = offset - span->offset;
        if (span->words != NULL)
        {
            uint16_t *wordPointer = &span->words[relativeOffset / 2];
            uint8_t byteShift = (relativeOffset & 1) * 8;
            *wordPointer = (uint16_t)((*wordPointer & (uint16_t)~((uint16_t)0xFF << byteShift)) | ((uint16_t)value << byteShift));
        }
        else span->bytes[relativeOffset] = value;
        return;
    }
    /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("EGA overlay write has no mapped state owner.");
}
uint16_t EgaReadOverlayWord(uint16_t offset)
{
    return (uint16_t)(EgaReadOverlayByte(offset)
        | ((uint16_t)EgaReadOverlayByte((uint16_t)(offset + 1)) << 8));
}
