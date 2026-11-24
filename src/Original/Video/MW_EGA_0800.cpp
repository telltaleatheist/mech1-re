#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/campaign_buffer.h"
#include "mechwarrior/ega_campaign.h"
#include "mechwarrior/game_files.h"
#include "mechwarrior/video_mode.h"
#include "Canvas.h"
#include <stdio.h>
#include <stdlib.h>

using mw1o::video::Canvas;
using mw1o::video::GameCanvas;
using mw1o::video::Logic;

/* The overlay draws on the game's canvas by pixel (src/Video/Canvas.h, G5).
 * It keeps its own addressing: a page selector, a byte offset of eight pixels
 * into the page, and a mask with the leftmost pixel at bit 7. A page selector
 * (A000h, A200h) is a canvas page, and a byte offset is the eight pixels from
 * (offset mod 40)*8 on row offset/40, so its 16-bit offsets run on past row
 * 199 into the page's tail and the page after, as in the original's linear
 * display memory. Past the canvas is refused. */
static_assert(Canvas::MEMORY_BYTES == EGA_VIDEO_PIXEL_COUNT, "The overlay's pages fill the canvas");
static_assert(Canvas::PAGE_STRIDE == 0x200 * EGA_SELECTOR_PARAGRAPH_BYTES * EGA_PIXELS_PER_BYTE,
    "A page selector step (200h) is one canvas page");
static_assert(Canvas::WIDTH == EGA_ROW_BYTES * EGA_PIXELS_PER_BYTE, "A row is 40 bytes of 8 pixels");
enum { EGA_PAGE_SELECTOR_STEP = 0x200 };
/* The function the overlay's next sprite or pixel draws by: what its last
 * routine left, as the original leaves it in the adapter (copy after a mode
 * set, OR after transparent text). */
uint8_t egaLogicalFunction;

static int EgaCanvasPage(uint16_t selector)
{
    uint16_t paragraphs = (uint16_t)(selector - EGA_FIRST_PAGE_SELECTOR);
    if (paragraphs % EGA_PAGE_SELECTOR_STEP) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("EGA page selector between canvas pages");
    return paragraphs / EGA_PAGE_SELECTOR_STEP;
}
/* Eight pixels at a byte offset: `colour` where `mask` has a 1, by `logic`. */
static void EgaDrawByte(int page, uint16_t byteOffset, uint8_t mask, uint8_t colour, Logic logic = Logic::Copy)
{
    GameCanvas().bitPattern(page, (byteOffset % EGA_ROW_BYTES) * EGA_PIXELS_PER_BYTE, byteOffset / EGA_ROW_BYTES,
        mask, colour, logic);
}
/* Eight pixels at a byte offset from one page to another, all read first. */
static void EgaCopyByte(int fromPage, int toPage, uint16_t byteOffset)
{
    Canvas &canvas = GameCanvas();
    int x = (byteOffset % EGA_ROW_BYTES) * EGA_PIXELS_PER_BYTE, y = byteOffset / EGA_ROW_BYTES;
    uint8_t pixels[EGA_PIXELS_PER_BYTE];
    for (int pixel = 0; pixel < EGA_PIXELS_PER_BYTE; ++pixel) pixels[pixel] = canvas.pixelAt(fromPage, x + pixel, y);
    for (int pixel = 0; pixel < EGA_PIXELS_PER_BYTE; ++pixel) canvas.pixel(toPage, x + pixel, y, pixels[pixel]);
}
static Logic EgaFunctionLogic(void)
{
    switch (egaLogicalFunction)
    {
    case EGA_COPY: return Logic::Copy;
    case EGA_AND: return Logic::And;
    case EGA_OR: return Logic::Or;
    case EGA_XOR: return Logic::Xor;
    default: /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Unknown EGA logical function");
    }
}
static void EgaSetPaletteCodes(const uint8_t *paletteCodes)
{
    for (uint8_t entry = 0; entry < EGA_PALETTE_ENTRIES; ++entry)
        GameCanvas().setPaletteEntry(entry, mw1o::video::PaletteCodeColour(paletteCodes[entry]));
}

/* The unchecked paths: a row past 200, a column past 320, a font index past
 * 5, a glyph outside the font storage, a font name at another offset. The
 * exe reads its neighbouring bytes there, machine code among them, which
 * the start state leaves zero (slice 6e, an intended difference). Each path
 * says so once on stderr, so a game that reaches one is seen, not drawn
 * silently from zeros. Host diagnostics only: the read itself is unchanged. */
enum { EGA_UNCHECKED_ROW, EGA_UNCHECKED_COLUMN, EGA_UNCHECKED_FONT, EGA_UNCHECKED_GLYPH, EGA_UNCHECKED_NAME,
    EGA_UNCHECKED_PATHS };
static bool egaUncheckedPathSeen[EGA_UNCHECKED_PATHS];
static void EgaNoteUncheckedPath(int path, uint16_t offset)
{
    static const char *const names[EGA_UNCHECKED_PATHS] = {"row", "column", "font index", "glyph", "font name"};
    if (egaUncheckedPathSeen[path]) return;
    egaUncheckedPathSeen[path] = true;
    fprintf(stderr, "MechWarrior: the EGA overlay's unchecked %s path read overlay offset %04X (MW_EGA.EXE's code bytes start zero here)\n",
        names[path], (unsigned)offset);
}
static uint8_t EgaUncheckedByte(int path, uint16_t offset)
{ EgaNoteUncheckedPath(path, offset); return EgaReadOverlayByte(offset); }
static uint16_t EgaUncheckedWord(int path, uint16_t offset)
{ EgaNoteUncheckedPath(path, offset); return EgaReadOverlayWord(offset); }

/* MW_EGA0800:003D..0057. Scanline generation is a separate ordinal01. The
 * mode set is the canvas's set-up, and the BIOS palette calls (INT 10h
 * AX=1000h and 1002h) are the canvas's palette entries for the codes (G5). */
void EgaInitializeGraphics(void)
{
    MwSetVideoMode(EGA_BIOS_MODE_320X200);
    EgaClearDrawPage(1);
    GameCanvas().setPaletteEntry(Campaign_Pal_EGA_Black, mw1o::video::PaletteCodeColour(EGA_PALETTE_BLACK));
    EgaFlipPages();
    EgaLoadDefaultPalette();
}
/* MW_EGA0800:13CE..13D9;13DA..13DE is the alternate entry. */
void EgaLoadDefaultPalette(void)
{ EgaSetPaletteCodes(egaOverlay.defaultPaletteCodes); }
void EgaLoadAlternatePalette(void)
{ EgaSetPaletteCodes(egaOverlay.alternatePaletteCodes); }

/* MW_EGA 0800:0233..0248, ordinal01; startup invokes it separately from mode setup. */
void EgaBuildScanlineOffsets(void)
{
    for (uint16_t row = 0; row < EGA_ROWS; ++row)
        egaOverlay.scanlineOffsets[row] = (uint16_t)(row * EGA_ROW_BYTES);
}

/* MW_EGA 0800:107E..1095. Failure of the first font stops the second load. */
bool EgaLoadFonts(void)
{
    if (EgaLoadFontFile(EGA_SMALL_FONT_NAME, EGA_SMALL_FONT_BITMAP)) return true;
    if (EgaLoadFontFile(EGA_LARGE_FONT_NAME, EGA_LARGE_FONT_BITMAP)) return true;
    return false;
}
/* MW_EGA 0800:1096..10B7. Short reads succeed. The read can overwrite nearby
 * overlay data, including the font tables. */
bool EgaLoadFontFile(uint16_t filenameOffset, uint16_t bufferOffset)
{
    egaOverlay.fontReadBufferOffset = bufferOffset;
    mw1o::files::GameFile file = EgaOpenFontFile(filenameOffset);
    if (!file) return true;
    egaOverlay.fontHandleBytes = EGA_FONT_HANDLE_VALUE;
    if (EgaReadFontFile(file, egaOverlay.fontReadBufferOffset, EGA_FONT_READ_BYTES)) return true;
    return !file.close();
}
/* MW_EGA 0800:10B8..10C0. */
mw1o::files::GameFile EgaOpenFontFile(uint16_t filenameOffset)
{
    if (filenameOffset == EGA_SMALL_FONT_NAME)
        return mw1o::files::open_game_file(egaOverlay.smallFontFilename);
    if (filenameOffset == EGA_LARGE_FONT_NAME)
        return mw1o::files::open_game_file(egaOverlay.largeFontFilename);
    /* An arbitrary native near string may cross named objects. Marshal only
     * at the file boundary; ordinary filename access uses its named owner. */
    char filename[EGA_DATA_BYTE_COUNT];
    uint32_t filenameBytes = 0;
    do
    {
        filename[filenameBytes] = (char)EgaUncheckedByte(EGA_UNCHECKED_NAME, (uint16_t)(filenameOffset + filenameBytes));
    } while (filename[filenameBytes++] != 0 && filenameBytes < EGA_DATA_BYTE_COUNT);
    if (filename[filenameBytes - 1] != 0) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: filename[filenameBytes - 1] != 0"); /* Unsupported unterminated DOS name. */
    return mw1o::files::open_game_file(filename);
}
/* MW_EGA 0800:10C1..10C7. The byte count is not tested by its caller. */
bool EgaReadFontFile(mw1o::files::GameFile &file, uint16_t bufferOffset, uint16_t byteCount)
{
    if ((uint32_t)bufferOffset + byteCount > EGA_DATA_BYTE_COUNT) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: (uint32_t)bufferOffset + byteCount > EGA_DATA_BYTE_COUNT"); /* Unsupported DOS transfer window. */
    std::optional<uint16_t> bytesRead;
    if (bufferOffset >= EGA_SMALL_FONT_BITMAP
        && (uint32_t)bufferOffset + byteCount <= EGA_FONT_DATA_END)
    {
        bytesRead = file.read(&egaOverlay.fontBitmapStorage[bufferOffset - EGA_SMALL_FONT_BITMAP], byteCount);
    }
    else
    {
        /* DOS transfers may cross named owners. Preserve stale bytes and
         * partial/error writes, then scatter back into the canonical owners.
         * This temporary IO window is not persistent game state or a DS mirror. */
        uint8_t *transferBuffer = static_cast<uint8_t *>(malloc(byteCount != 0 ? byteCount : 1));
        if (transferBuffer == NULL) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: transferBuffer == NULL");
        for (uint32_t byteIndex = 0; byteIndex < byteCount; ++byteIndex)
            transferBuffer[byteIndex] = EgaReadOverlayByte((uint16_t)(bufferOffset + byteIndex));
        bytesRead = file.read(transferBuffer, byteCount);
        for (uint32_t byteIndex = 0; byteIndex < byteCount; ++byteIndex)
            EgaWriteOverlayByte((uint16_t)(bufferOffset + byteIndex), transferBuffer[byteIndex]);
        free(transferBuffer);
    }
    /* A long second font can overwrite the executing loader's machine code.
     * Compiled C cannot pretend to execute those changed opcodes. The bytes
     * have been written, but this native self-modification is unsupported. */
    if (bytesRead && *bytesRead != 0 && (uint32_t)bufferOffset + *bytesRead > EGA_FONT_DATA_END) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !readFailed && bytesReadOrError != 0 && (uint32_t)bufferOffset + bytesReadOrError > EGA_FONT_DATA_END");
    return !bytesRead;
}

/* MW_EGA 0800:10EF..110A. No clamp: shifted table offset wraps as a WORD. */
void EgaSelectFont(uint16_t fontIndex)
{
    egaOverlay.selectedFontIndex = fontIndex;
    uint16_t tableByteOffset = fontIndex * EGA_WORD_BYTES;
    if (tableByteOffset < sizeof(egaOverlay.egaFontHeights))
    {
        uint16_t fontSlot = tableByteOffset / EGA_WORD_BYTES;
        egaOverlay.selectedFontHeight = egaOverlay.egaFontHeights[fontSlot];
        egaOverlay.selectedGlyphAdvance = egaOverlay.fontAdvances[fontSlot];
        egaOverlay.selectedFontBitmapOffset = egaOverlay.fontBitmapOffsets[fontSlot];
    }
    else
    {
        /* Native has no bounds check. Invalid indices read adjacent live
         * objects in this order; this fallback is not a new clamp. */
        egaOverlay.selectedFontHeight = EgaUncheckedWord(EGA_UNCHECKED_FONT, (uint16_t)(EGA_FONT_HEIGHTS + tableByteOffset));
        egaOverlay.selectedGlyphAdvance = EgaUncheckedWord(EGA_UNCHECKED_FONT, (uint16_t)(EGA_FONT_ADVANCES + tableByteOffset));
        egaOverlay.selectedFontBitmapOffset = EgaUncheckedWord(EGA_UNCHECKED_FONT, (uint16_t)(EGA_FONT_BITMAPS + tableByteOffset));
    }
}
/* MW_EGA 0800:110B..110E. */
uint16_t EgaGetFontIndex(void)
{ return egaOverlay.selectedFontIndex; }

/* MW_EGA0800:08C0..08C6, falling through08C7. */
void EgaFillDrawPageRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t colour)
{ EgaFillRectangleOnPage(x,y,width,height,colour,egaOverlay.egaDrawPage); }

/* MW_EGA0800:08C7..09B0. Set/reset mode0 becomes the colour drawn by mask.
 * First/last masks and the unsigned interior underflow are native, not clipping. */
void EgaFillRectangleOnPage(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
    uint16_t colour, uint16_t page)
{
    egaOverlay.rectangleX = x; egaOverlay.rectangleY = y;
    egaOverlay.rectangleWidth = width; egaOverlay.rectangleHeight = height; egaOverlay.rectangleColour = colour;
    uint16_t rowOffset = (y < EGA_ROWS ? egaOverlay.scanlineOffsets[y]
        : EgaUncheckedWord(EGA_UNCHECKED_ROW, (uint16_t)(EGA_SCANLINE_OFFSETS + (uint16_t)(y * 2)))) + (x >> 3);
    egaOverlay.firstByteMask = egaOverlay.rectangleLeadingMasks[x & 7];
    egaOverlay.firstByteWidth = 8 - (x & 7);
    egaOverlay.lastByteWidth = (uint16_t)(width - egaOverlay.firstByteWidth) & 7;
    egaOverlay.lastByteMask = egaOverlay.rectangleTrailingMasks[egaOverlay.lastByteWidth];
    egaLogicalFunction = EGA_COPY;
    uint16_t interiorBytes = (uint16_t)(width - egaOverlay.firstByteWidth - egaOverlay.lastByteWidth) >> 3;
    uint16_t rowsRemaining = height;
    int canvasPage = EgaCanvasPage(page);
    do
    {
        uint16_t destinationOffset = rowOffset;
        for (uint16_t byteIndex = 0; byteIndex < interiorBytes + 2; ++byteIndex)
        {
            uint8_t mask = byteIndex == 0 ? egaOverlay.firstByteMask
                : byteIndex == interiorBytes + 1 ? egaOverlay.lastByteMask : 0xFF;
            EgaDrawByte(canvasPage, destinationOffset, mask, (uint8_t)egaOverlay.rectangleColour);
            destinationOffset++;
        }
        rowOffset += EGA_ROW_BYTES;
        rowsRemaining--;
    } while (rowsRemaining != 0); /* Height zero executes65536 rows. */
}
/* MW_EGA0800:09B1..09BA, JMP08C7. */
void EgaFillDisplayPageRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t colour)
{ EgaFillRectangleOnPage(x,y,width,height,colour,egaOverlay.egaDisplayPage); }

/* MW_EGA0800:111C..1190. Each byte's read and write copy its eight pixels. */
void EgaCopyDrawPageToDisplay(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    /* Native stores precede the scanline lookup, which can alias these words. */
    egaOverlay.copyRectangleX = x; egaOverlay.copyRectangleY = y;
    egaOverlay.copyRectangleWidth = width; egaOverlay.copyRectangleHeight = height;
    uint16_t rowOffset = (y < EGA_ROWS ? egaOverlay.scanlineOffsets[y]
        : EgaUncheckedWord(EGA_UNCHECKED_ROW, (uint16_t)(EGA_SCANLINE_OFFSETS + (uint16_t)(y * 2)))) + (x >> 3);
    uint8_t rowsRemaining = height;
    int sourcePage = EgaCanvasPage(egaOverlay.egaDrawPage), destinationPage = EgaCanvasPage(egaOverlay.egaDisplayPage);
    do
    {
        uint16_t copyOffset = rowOffset, bytesRemaining = width >> 3;
        do
        {
            EgaCopyByte(sourcePage, destinationPage, copyOffset);
            copyOffset++; bytesRemaining--;
        } while (bytesRemaining != 0); /* Zero bytes runs65536 copies. */
        rowOffset += EGA_ROW_BYTES; rowsRemaining--;
    } while (rowsRemaining != 0); /* DL only: zero runs256 rows. */
}

/* MW_EGA0800:1191..1205. Deliberately keep the real reverse-copy procedure;
 * swapping global page owners to reuse111C would change observable state. */
void EgaCopyDisplayToDrawPage(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    /* Native stores precede the scanline lookup, which can alias these words. */
    egaOverlay.copyRectangleX = x; egaOverlay.copyRectangleY = y;
    egaOverlay.copyRectangleWidth = width; egaOverlay.copyRectangleHeight = height;
    uint16_t rowOffset = (y < EGA_ROWS ? egaOverlay.scanlineOffsets[y]
        : EgaUncheckedWord(EGA_UNCHECKED_ROW, (uint16_t)(EGA_SCANLINE_OFFSETS + (uint16_t)(y * 2)))) + (x >> 3);
    uint8_t rowsRemaining = height;
    int sourcePage = EgaCanvasPage(egaOverlay.egaDisplayPage), destinationPage = EgaCanvasPage(egaOverlay.egaDrawPage);
    do
    {
        uint16_t copyOffset = rowOffset, bytesRemaining = width >> 3;
        do
        {
            EgaCopyByte(sourcePage, destinationPage, copyOffset);
            copyOffset++; bytesRemaining--;
        } while (bytesRemaining != 0);
        rowOffset += EGA_ROW_BYTES; rowsRemaining--;
    } while (rowsRemaining != 0);
}

/* MW_EGA 0800:06B5..0764. SI is projected as the returned source cursor, not AX.
 * Mode2 colour expansion becomes the colour drawn by mask; original iteration,
 * odd-row padding, transparent zero, WORD wrap and inherited logic stay here. */
uint16_t EgaDrawPackedSprite(uint16_t x, uint16_t y, uint8_t targetPage,
    uint16_t pictureOffset, MwCampaignBuffer *picture)
{
    uint16_t rowStartOffset = y < EGA_ROWS ? egaOverlay.scanlineOffsets[y]
        : EgaUncheckedWord(EGA_UNCHECKED_ROW, (uint16_t)(EGA_SCANLINE_OFFSETS + y * EGA_WORD_BYTES));
    uint8_t initialMask = (x < sizeof(egaOverlay.xPixelMasks) ? egaOverlay.xPixelMasks[x]
        : EgaUncheckedByte(EGA_UNCHECKED_COLUMN, (uint16_t)(EGA_PIXEL_MASKS + x)));
    rowStartOffset += x < EGA_COLUMNS ? egaOverlay.xByteOffsets[x]
        : EgaUncheckedWord(EGA_UNCHECKED_COLUMN, (uint16_t)(EGA_PIXEL_BYTE_OFFSETS + x * EGA_WORD_BYTES));
    const uint8_t *pictureData = MwCampaignBufferBytes(picture);
    if (pictureOffset <= CAMPAIGN_BUFFER_BYTE_COUNT - sizeof(PackedPictureHeader))
    {
        const PackedPictureHeader *pictureHeader = (const PackedPictureHeader *)&pictureData[pictureOffset];
        egaOverlay.spriteWidth = pictureHeader->width[0] | ((uint16_t)pictureHeader->width[1] << EGA_BYTE_BITS);
        egaOverlay.spriteRowsRemaining = pictureHeader->height[0] | ((uint16_t)pictureHeader->height[1] << EGA_BYTE_BITS);
        pictureOffset += sizeof(PackedPictureHeader);
    }
    else
    {
        /* Only a boundary-crossing record needs the wrapping byte reader.
         * Preserve width-store before height-read, not an aggregate snapshot. */
        egaOverlay.spriteWidth = MwCampaignBufferWord(pictureData,pictureOffset);
        pictureOffset += EGA_WORD_BYTES;
        egaOverlay.spriteRowsRemaining = MwCampaignBufferWord(pictureData,pictureOffset);
        pictureOffset += EGA_WORD_BYTES;
    }
    int canvasPage = EgaCanvasPage(targetPage == EGA_DISPLAY_PAGE ? egaOverlay.egaDisplayPage : egaOverlay.egaDrawPage);
    do
    {
        uint16_t destinationOffset = rowStartOffset;
        uint16_t remainingPixels = egaOverlay.spriteWidth;
        uint8_t pixelMask = initialMask;
        do
        {
            uint8_t packedPixels = pictureData[pictureOffset]; pictureOffset++;
            for (uint8_t pixelInPair = 0; pixelInPair < EGA_PACKED_PIXELS_PER_BYTE; ++pixelInPair)
            {
                uint8_t color = pixelInPair == 0 ? packedPixels >> EGA_NIBBLE_SHIFT : packedPixels & EGA_NIBBLE_MASK;
                /* Mode2 ignores rotate/set-reset, but inherits the function. */
                if (color != 0)
                    EgaDrawByte(canvasPage, destinationOffset, pixelMask, color, pixelMask ? EgaFunctionLogic() : Logic::Copy);
                bool advanceByte = (pixelMask & 1) != 0;
                pixelMask = (uint8_t)((pixelMask >> 1) | (pixelMask << EGA_BYTE_LAST_BIT));
                if (advanceByte) destinationOffset++;
                remainingPixels--;
                if (remainingPixels == 0) break;
            }
        } while (remainingPixels != 0);
        rowStartOffset += EGA_ROW_BYTES;
        egaOverlay.spriteRowsRemaining--;
    } while (egaOverlay.spriteRowsRemaining != 0);
    return pictureOffset;
}

/* MW_EGA 0800:078B..089D. Native always paints eight glyph bits, even when the
 * advance is six. Enable-set/reset0 and full bit mask are caller preconditions.
 * The hardware clear/background/foreground passes become the two colours drawn
 * by the glyph's bits and their complement within its coverage. */
uint16_t EgaDrawCharacter(uint16_t x, uint16_t *y, uint8_t character,
    uint8_t foreground, uint8_t background)
{
    if (x >= EGA_GLYPH_WRAP_X) { x = 0; *y += egaOverlay.selectedFontHeight + 1; }
    egaOverlay.glyphX = x;
    egaOverlay.glyphY = *y;
    egaOverlay.characterCode = character; /* Native clears CH then stores CX. */
    egaOverlay.glyphForeground = foreground;
    egaOverlay.glyphBackground = background;
    egaOverlay.glyphBitOffset = egaOverlay.glyphX & EGA_BYTE_LAST_BIT;
    uint16_t destinationOffset = egaOverlay.glyphY < EGA_ROWS ? egaOverlay.scanlineOffsets[egaOverlay.glyphY]
        : EgaUncheckedWord(EGA_UNCHECKED_ROW, (uint16_t)(EGA_SCANLINE_OFFSETS + egaOverlay.glyphY * EGA_WORD_BYTES));
    destinationOffset += egaOverlay.glyphX >> EGA_PIXEL_BYTE_SHIFT;
    uint16_t glyphOffset = (uint8_t)(egaOverlay.characterCode - EGA_FIRST_CHARACTER) * (uint8_t)egaOverlay.selectedFontHeight + EGA_FONT_HEADER_BYTES
        + egaOverlay.selectedFontBitmapOffset;
    int canvasPage = EgaCanvasPage(egaOverlay.egaDisplayPage);
    uint16_t rowsRemaining = egaOverlay.selectedFontHeight;
    do
    {
        uint16_t shiftedGlyphBits = (uint16_t)(glyphOffset >= EGA_SMALL_FONT_BITMAP && glyphOffset < EGA_FONT_DATA_END
            ? egaOverlay.fontBitmapStorage[glyphOffset - EGA_SMALL_FONT_BITMAP] : EgaUncheckedByte(EGA_UNCHECKED_GLYPH, glyphOffset)) << EGA_BYTE_BITS;
        glyphOffset++;
        shiftedGlyphBits >>= egaOverlay.glyphBitOffset;
        uint16_t glyphCoverageMask = EGA_GLYPH_COVERAGE_WORD >> egaOverlay.glyphBitOffset;
        for (uint8_t byteIndex = 0; byteIndex < EGA_WORD_BYTES; ++byteIndex)
        {
            /* SI wraps before the page base is added; adjacent bytes need not
             * be adjacent pixels at FFFF. This is not screen clipping. */
            uint16_t byteOffset = (uint16_t)(destinationOffset + byteIndex);
            uint8_t shift = (uint8_t)(EGA_BYTE_BITS * (EGA_WORD_BYTES - 1 - byteIndex));
            uint8_t coverage = (uint8_t)(glyphCoverageMask >> shift), bits = (uint8_t)(shiftedGlyphBits >> shift);
            EgaDrawByte(canvasPage, byteOffset, coverage & bits, egaOverlay.glyphForeground);
            EgaDrawByte(canvasPage, byteOffset, coverage & (uint8_t)~bits, egaOverlay.glyphBackground);
        }
        destinationOffset += EGA_ROW_BYTES; rowsRemaining--;
    } while (rowsRemaining != 0);
    egaLogicalFunction = EGA_COPY;
    return (uint16_t)(egaOverlay.glyphX + egaOverlay.selectedGlyphAdvance);
}

/* MW_EGA0800:0765..0777. ES:DI is saved around each real078B call.
 * The original leaves DI at the NUL; no caller reads it. AX/BX are live cursors. */
uint16_t EgaDrawString(uint16_t x, uint16_t *y, uint16_t colours,
    MwText text)
{
    for (;;)
    {
        uint8_t character = MwTextByte(text);
        if (character == 0) break;
        x = EgaDrawCharacter(x,y,character,(uint8_t)colours,(uint8_t)(colours >> EGA_BYTE_BITS));
        text.cursor++;
    }
    return x;
}

/* MW_EGA 0800:09C2..09C7 and 0AC7..0ACE. */
const uint8_t *EgaDrawTransparentText(uint16_t x, uint16_t y, uint8_t color, MwText text)
{ egaOverlay.textExtraLeading = 0; return EgaDrawTextWithLeading(x, y, color, text); }
const uint8_t *EgaDrawTextWithLineSpacing(uint16_t x, uint16_t y, uint8_t color, MwText text)
{ egaOverlay.textExtraLeading = 1; return EgaDrawTextWithLeading(x, y, color, text); }

/* MW_EGA 0800:09C8..0A38. CR re-enters setup; FF consumes no glyph/advance. */
const uint8_t *EgaDrawTextWithLeading(uint16_t x, uint16_t y, uint8_t color, MwText text)
{
setup:
    egaOverlay.textStartX = x;
    egaOverlay.textCurrentY = y;
    egaOverlay.egaTextColour = color;
    egaOverlay.glyphBitOffset = (uint8_t)(x & EGA_BYTE_LAST_BIT);
    uint16_t destinationOffset = y < EGA_ROWS ? egaOverlay.scanlineOffsets[y]
        : EgaUncheckedWord(EGA_UNCHECKED_ROW, (uint16_t)(EGA_SCANLINE_OFFSETS + y * EGA_WORD_BYTES));
    destinationOffset += x >> EGA_PIXEL_BYTE_SHIFT;
    for (;;)
    {
        uint8_t character = MwTextByte(text);
        if (character == 0) break;
        if (character == EGA_CARRIAGE_RETURN)
        {
            x = egaOverlay.textStartX;
            y = (uint16_t)(egaOverlay.textCurrentY
                + egaOverlay.selectedFontHeight
                + egaOverlay.textExtraLeading);
            color = egaOverlay.egaTextColour; text.cursor++; goto setup;
        }
        if (character != EGA_SKIP_CHARACTER) destinationOffset = EgaDrawTransparentCharacter(character, destinationOffset);
        text.cursor++;
    }
    egaLogicalFunction = EGA_COPY;
    return text.cursor; /* Native DI at the NUL; not AX/source return ABI. */
}
/* MW_EGA 0800:0A39..0AC6. BYTE height in MUL, WORD row LOOP. At most one byte
 * of horizontal advance; CL shifts >=16 clear the glyph, not masked host UB.
 * Hardware AND/OR paints only set glyph bits; zero bits leave pixels alone. */
uint16_t EgaDrawTransparentCharacter(uint8_t character, uint16_t destinationOffset)
{
    uint16_t glyphOffset = (uint8_t)(character - EGA_FIRST_CHARACTER) * (uint8_t)egaOverlay.selectedFontHeight + EGA_FONT_HEADER_BYTES
        + egaOverlay.selectedFontBitmapOffset;
    int canvasPage = EgaCanvasPage(egaOverlay.egaDisplayPage);
    uint16_t rowDestinationOffset = destinationOffset, rowsRemaining = egaOverlay.selectedFontHeight;
    do
    {
        uint16_t shiftedGlyphBits = (uint16_t)(glyphOffset >= EGA_SMALL_FONT_BITMAP && glyphOffset < EGA_FONT_DATA_END
            ? egaOverlay.fontBitmapStorage[glyphOffset - EGA_SMALL_FONT_BITMAP] : EgaUncheckedByte(EGA_UNCHECKED_GLYPH, glyphOffset)) << EGA_BYTE_BITS;
        glyphOffset++;
        shiftedGlyphBits = egaOverlay.glyphBitOffset < EGA_BYTE_BITS * EGA_WORD_BYTES
            ? (uint16_t)(shiftedGlyphBits >> egaOverlay.glyphBitOffset) : 0;
        for (uint8_t byteIndex = 0; byteIndex < EGA_WORD_BYTES; ++byteIndex)
        {
            uint8_t shift = (uint8_t)(EGA_BYTE_BITS * (EGA_WORD_BYTES - 1 - byteIndex));
            EgaDrawByte(canvasPage, (uint16_t)(rowDestinationOffset + byteIndex), (uint8_t)(shiftedGlyphBits >> shift), egaOverlay.egaTextColour);
        }
        rowDestinationOffset += EGA_ROW_BYTES; rowsRemaining--;
    } while (rowsRemaining != 0);
    uint8_t nextBitOffset = egaOverlay.glyphBitOffset
        + egaOverlay.selectedGlyphAdvance;
    if (nextBitOffset >= EGA_BYTE_BITS) { destinationOffset++; nextBitOffset -= EGA_BYTE_BITS; }
    egaOverlay.glyphBitOffset = nextBitOffset;
    egaLogicalFunction = EGA_OR;
    return destinationOffset;
}
/* MW_EGA 0800:005C..0094. Page identity retained. The new display page is
 * presented where the original sets the CRTC start (G5). */
void EgaFlipPages(void)
{
    if (egaOverlay.egaDisplayPage == EGA_FIRST_PAGE_SELECTOR)
    {
        egaOverlay.egaDisplayPage = EGA_SECOND_PAGE_SELECTOR;
        egaOverlay.egaDrawPage = EGA_FIRST_PAGE_SELECTOR;
    }
    else
    {
        egaOverlay.egaDisplayPage = EGA_FIRST_PAGE_SELECTOR;
        egaOverlay.egaDrawPage = EGA_SECOND_PAGE_SELECTOR;
    }
    GameCanvas().present(EgaCanvasPage(egaOverlay.egaDisplayPage));
}

/* MW_EGA0800:0249..0280. Mode0 with a full mask: every pixel of the draw
 * page's 200 rows. A clear under another function is not converted. */
void EgaClearDrawPage(uint8_t colour)
{
    if (egaLogicalFunction!=EGA_COPY) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: egaLogicalFunction!=EGA_COPY"); /* Unsupported inherited function. */
    int canvasPage=EgaCanvasPage(egaOverlay.egaDrawPage);
    for (uint16_t offset=0;offset<EGA_ROWS*EGA_ROW_BYTES;++offset) EgaDrawByte(canvasPage,offset,0xFF,colour);
}
/* MW_EGA0800:0641..06B0. Masked write by the inherited function, then native
 * COPY cleanup. Multi-bit lookup masks remain live. */
void EgaDrawPixel(uint16_t x,uint16_t y,uint8_t targetPage,uint8_t colour)
{
    uint16_t offset=y<EGA_ROWS?egaOverlay.scanlineOffsets[y]:
        EgaUncheckedWord(EGA_UNCHECKED_ROW, (uint16_t)(EGA_SCANLINE_OFFSETS+(uint16_t)(y*EGA_WORD_BYTES)));
    uint8_t mask=x<sizeof(egaOverlay.xPixelMasks)?egaOverlay.xPixelMasks[x]:
        EgaUncheckedByte(EGA_UNCHECKED_COLUMN, (uint16_t)(EGA_PIXEL_MASKS+x));
    offset+=x<EGA_COLUMNS?egaOverlay.xByteOffsets[x]:
        EgaUncheckedWord(EGA_UNCHECKED_COLUMN, (uint16_t)(EGA_PIXEL_BYTE_OFFSETS+(uint16_t)(x*EGA_WORD_BYTES)));
    int canvasPage=EgaCanvasPage(targetPage==EGA_DISPLAY_PAGE?egaOverlay.egaDisplayPage:egaOverlay.egaDrawPage);
    EgaDrawByte(canvasPage,offset,mask,colour,mask?EgaFunctionLogic():Logic::Copy);
    egaLogicalFunction=EGA_COPY;
}

/* MW_EGA0800:1219; original line/raster procedure. */
void EgaDrawLine(uint8_t colour, uint16_t startY, uint16_t endX, uint16_t startX, uint16_t endY)
{
    // Native saves/restores AX,BX,CX,DX,SI; BP/DI are arithmetic scratch, NOT a frame.
    // No clipping. Endpoint ordering and major-axis selection are UNSIGNED.
    egaOverlay.egaLineColour=colour;
    if (startX>=endX)
    {
        uint16_t savedX=startX,savedY=startY;
        startX=endX;startY=endY;endX=savedX;endY=savedY;
    }
    egaOverlay.egaCurrentX=startX;egaOverlay.currentY=startY;
    egaOverlay.egaFinalX=endX;egaOverlay.egaFinalY=endY;
    if (egaOverlay.egaCurrentX==egaOverlay.egaFinalX)
    {
        if (egaOverlay.currentY!=egaOverlay.egaFinalY)EgaDrawVerticalLine();
        else EgaDrawPixel(egaOverlay.egaCurrentX,egaOverlay.currentY,EGA_DISPLAY_PAGE,egaOverlay.egaLineColour);
        return;
    }
    if (egaOverlay.currentY==egaOverlay.egaFinalY)
    {EgaDrawHorizontalLine();return;}
    bool descendingY=egaOverlay.currentY>egaOverlay.egaFinalY;
    uint16_t horizontalDistance=egaOverlay.egaFinalX-egaOverlay.egaCurrentX;
    uint16_t verticalDistance=descendingY?(uint16_t)(egaOverlay.currentY-egaOverlay.egaFinalY):(uint16_t)(egaOverlay.egaFinalY-egaOverlay.currentY);
    bool yMajor=verticalDistance>horizontalDistance;
    uint16_t majorDistance=yMajor?verticalDistance:horizontalDistance;
    uint16_t minorDistance=yMajor?horizontalDistance:verticalDistance;
    uint16_t majorPixelCount=majorDistance+0x0001;
    uint16_t remainingPixels=majorPixelCount;
    uint16_t halfMajorDistance=majorDistance>>0x0001; // Native BP, logical SHR.
    uint16_t errorAccumulator=0x0000;
    uint16_t yStep=descendingY?0xFFFF:0x0001;
    do
    {
        EgaDrawSavedLinePixel();
        if (yMajor)egaOverlay.currentY += yStep;
        else ++egaOverlay.egaCurrentX;
        errorAccumulator += minorDistance;
        // Native CMP DX,BP / JLE: signed comparison AFTER16-bit ADD wrapping.
        if ((int16_t)errorAccumulator>(int16_t)halfMajorDistance)
        {
            errorAccumulator -= majorPixelCount;
            if (yMajor)++egaOverlay.egaCurrentX;
            else egaOverlay.currentY += yStep;
        }
        --remainingPixels;
    }while (remainingPixels!=0x0000);
    // The last pixel still advances state. Subtract major+1, NOT major.
    // A wrapped major+1=0000 still draws65536 pixels; do not widen the counter.
}

/* MW_EGA0800:1342; original line/raster procedure. */
void EgaDrawSavedLinePixel(void)
{
    // Native PUSH/POP AX,BX,CX,DX,DI,BP preserves the caller's line arithmetic.
    // Flags/ES are not saved; SI is untouched by the underlying pixel routine.
    EgaDrawPixel(egaOverlay.egaCurrentX,egaOverlay.currentY,EGA_DISPLAY_PAGE,egaOverlay.egaLineColour);
}

/* MW_EGA0800:1360; original line/raster procedure. */
void EgaDrawVerticalLine(void)
{
    // Implicit0800 state. No coordinate arguments; do not invent stack fields.
    uint16_t firstY=egaOverlay.currentY,lastY=egaOverlay.egaFinalY;
    if (firstY>=lastY){egaOverlay.currentY=lastY;egaOverlay.egaFinalY=firstY;}
    uint16_t pixelY=egaOverlay.currentY;
    for (;;)
    {
        EgaDrawPixel(egaOverlay.egaCurrentX,pixelY,EGA_DISPLAY_PAGE,egaOverlay.egaLineColour);
        if (pixelY==egaOverlay.egaFinalY)break;
        ++pixelY;
    }
    // Inclusive endpoints, unsigned Y sort. Only local BX advances, not stored Y.
    // Native saves BX across pixel calls; other pixel register clobbers survive.
}

/* MW_EGA0800:138F; original line/raster procedure. */
void EgaDrawHorizontalLine(void)
{
    uint16_t pixelX=egaOverlay.egaCurrentX;
    for (;;)
    {
        EgaDrawPixel(pixelX,egaOverlay.currentY,EGA_DISPLAY_PAGE,egaOverlay.egaLineColour);
        if (pixelX==egaOverlay.egaFinalX)break;
        ++pixelX;
    }
    // Inclusive endpoints; NO sorting here. Reversed direct calls wrap throughFFFF.
    // Native saves AX across pixel calls. Stored currentX is never advanced.
}
