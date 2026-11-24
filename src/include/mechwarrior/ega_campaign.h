#ifndef MECHWARRIOR_EGA_CAMPAIGN_H
#define MECHWARRIOR_EGA_CAMPAIGN_H
#include "mechwarrior/game_files.h"
#include "mechwarrior/native_address.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Picture-bank record prefix: two little-endian WORDs, followed by packed
 * pixels. Character fields permit unaligned byte storage without WORD-pointer
 * aliasing or host-endian assumptions. This view owns no duplicate data. */
typedef struct PackedPictureHeader
{
    unsigned char width[0x02];
    unsigned char height[0x02];
} PackedPictureHeader;
static_assert(sizeof(PackedPictureHeader) == 0x04, "Native picture header size");
static_assert(offsetof(PackedPictureHeader, height) == 0x02, "Native height offset");
static_assert(alignof(PackedPictureHeader) == 1, "Unaligned picture headers");

enum
{
    EGA_ROWS = 0x00C8, EGA_COLUMNS = 0x0140, EGA_ROW_BYTES = 0x0028, EGA_PIXELS_PER_BYTE = 0x08,
    EGA_DATA_BYTE_COUNT = 0x10000, EGA_VIDEO_PIXEL_COUNT = 0x80000,
    EGA_FIRST_PAGE_SELECTOR = 0xA000, EGA_SECOND_PAGE_SELECTOR = 0xA200,
    EGA_DISPLAY_PAGE = 0x00, EGA_DRAW_PAGE = 0x01,
    EGA_DISPLAY_SELECTOR = 0x0058, EGA_DRAW_SELECTOR = 0x005A,
    EGA_SCANLINE_OFFSETS = 0x00A3, EGA_PIXEL_BYTE_OFFSETS = 0x0281,
    EGA_PIXEL_MASKS = 0x0501, EGA_SPRITE_WIDTH = 0x06B1,
    EGA_SPRITE_ROWS = 0x06B3, EGA_GLYPH_X = 0x0778,
    EGA_GLYPH_Y = 0x077A, EGA_CHARACTER = 0x077C,
    EGA_FOREGROUND = 0x0782, EGA_BACKGROUND = 0x0783,
    EGA_GLYPH_BIT_OFFSET = 0x0784, EGA_FONT_HEIGHT = 0x0785,
    EGA_GLYPH_ADVANCE = 0x0787, EGA_FONT_BITMAP = 0x0789,
    EGA_TEXT_X = 0x09BB, EGA_TEXT_Y = 0x09BD,
    EGA_TEXT_COLOR = 0x09BF, EGA_TEXT_LEADING = 0x09C0,
    EGA_FONT_BUFFER = 0x0AD0, EGA_FONT_HANDLE = 0x0AD2,
    /* The overlay's font-handle word: DOS's first free handle. Glyph and column reads that run
     * past their tables can reach overlay bytes, so the word keeps the value it had. */
    EGA_FONT_HANDLE_VALUE = 0x0005,
    EGA_SMALL_FONT_NAME = 0x0AD4, EGA_LARGE_FONT_NAME = 0x0AEE,
    EGA_SMALL_FONT_BITMAP = 0x0B03, EGA_LARGE_FONT_BITMAP = 0x0D5D,
    EGA_FONT_INDEX = 0x10CF, EGA_FONT_HEIGHTS = 0x10D1,
    EGA_FONT_ADVANCES = 0x10DB, EGA_FONT_BITMAPS = 0x10E5,
    EGA_FONT_READ_BYTES = 0x03E8, EGA_FONT_DATA_END = 0x107E, EGA_GLYPH_WRAP_X = 0x0136,
    EGA_NIBBLE_MASK = 0x0F, EGA_NIBBLE_SHIFT = 0x04,
    EGA_BYTE_HIGH_BIT = 0x80, EGA_BYTE_BITS = 0x08, EGA_BYTE_LAST_BIT = 0x07,
    EGA_PIXEL_BYTE_SHIFT = 0x03, EGA_WORD_BYTES = 0x02, EGA_PACKED_PIXELS_PER_BYTE = 0x02,
    EGA_FIRST_CHARACTER = 0x20, EGA_FONT_HEADER_BYTES = 0x04,
    EGA_GLYPH_COVERAGE_WORD = 0xFF00, EGA_CARRIAGE_RETURN = 0x0D, EGA_SKIP_CHARACTER = 0xFF,
    EGA_SELECTOR_PARAGRAPH_BYTES = 0x10,
    EGA_COPY = 0, EGA_AND = 1, EGA_OR = 2, EGA_XOR = 3
};
enum { EGA_BIOS_MODE_320X200=0x0D, EGA_PALETTE_ENTRIES=0x11 };
/* BIOS palette-register values, not campaign pixel indices. */
enum { EGA_PALETTE_BLACK=0x00, EGA_PALETTE_LIGHT_GREY=0x07,
    EGA_PALETTE_DARK_GREY=0x10, EGA_PALETTE_WHITE=0x17 };
/* Pixel indices in MechWarrior's default campaign palette (MW_EGA:13AC),
 * NOT BIOS colour codes or the standard EGA index order. Palette effects and
 * the alternate palette can temporarily change their displayed colours. */
enum
{
    Campaign_Pal_EGA_Magenta=0x00, Campaign_Pal_EGA_Black=0x01, Campaign_Pal_EGA_White=0x02,
    Campaign_Pal_EGA_LightGrey=0x03, Campaign_Pal_EGA_DarkGrey=0x04, Campaign_Pal_EGA_LightCyan=0x05,
    Campaign_Pal_EGA_LightBlue=0x06, Campaign_Pal_EGA_Blue=0x07, Campaign_Pal_EGA_LightRed=0x08,
    Campaign_Pal_EGA_Red=0x09, Campaign_Pal_EGA_LightGreen=0x0A, Campaign_Pal_EGA_Cyan=0x0B,
    Campaign_Pal_EGA_Green=0x0C, Campaign_Pal_EGA_Yellow=0x0D, Campaign_Pal_EGA_Brown=0x0E,
    Campaign_Pal_EGA_LightMagenta=0x0F
};
void EgaInitializeGraphics(void);
void EgaLoadDefaultPalette(void);
void EgaLoadAlternatePalette(void);

/* Only binary transfers and unchecked neighbouring-byte reads use this view.
 * It projects the named owners; it must not become ordinary scalar access. */
uint8_t EgaReadOverlayByte(uint16_t offset);
void EgaWriteOverlayByte(uint16_t offset, uint8_t value);
uint16_t EgaReadOverlayWord(uint16_t offset);
/* Sets every overlay owner as MW_EGA.EXE holds it when the overlay loads,
 * without the exe (slice 6e, Memory/MW_EGA_0800_STATE.cpp). The ranges are
 * the exe's bytes it leaves zero: machine code and the font names' unread
 * tails. */
void EgaLoadStartState(void);
/* Then the one span of the overlay's code play reads, from the story. */
bool EgaLoadStartStoryBytes(void);
typedef struct EgaOverlayRange { uint16_t offset, byteCount; } EgaOverlayRange;
extern const EgaOverlayRange egaStartZeroedRanges[];
extern const size_t egaStartZeroedRangeCount;
/* The function the overlay's next sprite or pixel draws by, as its last routine
 * left it (Video/MW_EGA_0800.cpp; not a serialized overlay field). The overlay
 * draws on the game canvas (src/Video), EGA_VIDEO_PIXEL_COUNT pixels. */
extern uint8_t egaLogicalFunction;

bool EgaLoadFonts(void);
bool EgaLoadFontFile(uint16_t filenameOffset, uint16_t bufferOffset);
mw1o::files::GameFile EgaOpenFontFile(uint16_t filenameOffset);
bool EgaReadFontFile(mw1o::files::GameFile &file, uint16_t bufferOffset, uint16_t byteCount);
void EgaBuildScanlineOffsets(void);
void EgaSelectFont(uint16_t fontIndex);
uint16_t EgaGetFontIndex(void);
uint16_t EgaDrawPackedSprite(uint16_t x, uint16_t y, uint8_t targetPage,
    uint16_t pictureOffset, uint16_t pictureBufferSelector);
uint16_t EgaDrawCharacter(uint16_t x, uint16_t *y, uint8_t character,
    uint8_t foreground, uint8_t background);
uint16_t EgaDrawTransparentText(uint16_t x, uint16_t y, uint8_t color,
    const MwNativeByteView *text, uint16_t textOffset);
uint16_t EgaDrawTextWithLeading(uint16_t x, uint16_t y, uint8_t color,
    const MwNativeByteView *text, uint16_t textOffset);
uint16_t EgaDrawTransparentCharacter(uint8_t character, uint16_t destinationOffset);
uint16_t EgaDrawTextWithLineSpacing(uint16_t x, uint16_t y, uint8_t color,
    const MwNativeByteView *text, uint16_t textOffset);
void EgaFlipPages(void);
/* Packed palette indices: foreground in the low byte, background in the high
 * byte. These are palette slots, not fixed RGB colours. */
uint16_t EgaDrawString(uint16_t x, uint16_t *y, uint16_t colours,
    const MwNativeByteView *text, uint16_t *textOffset);
void EgaFillDrawPageRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t colour);
void EgaFillRectangleOnPage(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
    uint16_t colour, uint16_t page);
void EgaFillDisplayPageRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t colour);
void EgaCopyDrawPageToDisplay(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void EgaCopyDisplayToDrawPage(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
/* Campaign state owned by ega_campaign.h (plan, slice B2). */
struct EgaOverlayState
{
    uint8_t defaultPaletteCodes[EGA_PALETTE_ENTRIES];
    uint8_t alternatePaletteCodes[EGA_PALETTE_ENTRIES];
    /* Named canonical owners in Memory/MW_EGA_0800_STATE.c. No raw DS mirror. */
    /* MW_EGA CS/DS 0800 owns these recovered globals and tables. Ordinary game
     * access uses their names, not a synthetic segment pointer or WORD adapter. */
    uint16_t egaDisplayPage;
    uint16_t egaDrawPage;
    uint16_t scanlineOffsets[0x00C8];
    uint16_t xByteOffsets[0x0140];
    uint8_t xPixelMasks[0x0140];
    uint16_t spriteWidth;
    uint16_t spriteRowsRemaining;
    uint16_t glyphX;
    uint16_t glyphY;
    uint16_t characterCode;
    uint8_t glyphForeground;
    uint8_t glyphBackground;
    uint8_t glyphBitOffset;
    uint16_t selectedFontHeight;
    uint16_t selectedGlyphAdvance;
    uint16_t selectedFontBitmapOffset;
    uint16_t rectangleX, rectangleY, rectangleWidth, rectangleHeight;
    uint16_t rectangleColour, firstByteWidth, lastByteWidth;
    uint8_t firstByteMask, lastByteMask;
    uint8_t rectangleLeadingMasks[8], rectangleTrailingMasks[8];
    uint16_t copyRectangleX, copyRectangleY;
    uint16_t copyRectangleWidth, copyRectangleHeight;
    uint16_t textStartX;
    uint16_t textCurrentY;
    uint8_t egaTextColour;
    uint16_t textExtraLeading;
    uint16_t fontReadBufferOffset;
    uint16_t fontHandleBytes; /* 0AD2, EGA_FONT_HANDLE_VALUE once a font has loaded */
    char smallFontFilename[0x001A];
    char largeFontFilename[0x0015];
    uint8_t fontBitmapStorage[0x057B];
    uint16_t selectedFontIndex;
    uint16_t egaFontHeights[0x0005];
    uint16_t fontAdvances[0x0005];
    uint16_t fontBitmapOffsets[0x0005];
    uint8_t egaLineColour;
    uint16_t egaCurrentX, currentY, egaFinalX, egaFinalY;
};
extern EgaOverlayState egaOverlay;
void EgaClearDrawPage(uint8_t colour);
void EgaDrawPixel(uint16_t x,uint16_t y,uint8_t targetPage,uint8_t colour);
void EgaDrawLine(uint8_t colour,uint16_t startY,uint16_t endX,uint16_t startX,uint16_t endY);
void EgaDrawSavedLinePixel(void);
void EgaDrawVerticalLine(void);
void EgaDrawHorizontalLine(void);
#endif
