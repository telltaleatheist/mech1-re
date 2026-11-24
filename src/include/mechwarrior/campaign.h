#ifndef MECHWARRIOR_CAMPAIGN_H
#define MECHWARRIOR_CAMPAIGN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum
{
    PICTURE_RLE_ESCAPE = 0x0F,
    PICTURE_NIBBLE_MASK = 0x0F,
    PICTURE_HIGH_NIBBLE_MASK = 0xF0,
    PICTURE_NIBBLE_SHIFT = 0x04,
    RANDOM_WORD1_SHIFT = 0x02,
    RANDOM_WORD0_ROTATE = 0x05,
    RANDOM_CARRY_INSERT_SHIFT = 0x04,
    RANDOM_ROTATE_REINSERT_SHIFT = 0x0C,
    NATIVE_WORD_HIGH_BIT = 0x8000,
    NATIVE_WORD_BITS = 0x10,
    NATIVE_WORD_HIGH_BYTE_MASK = 0xFF00
};

enum { INDEXED_PICTURE_SLOTS = 0x0064, PICTURE_OFFSET_ENTRY_BYTES = 0x04,
    INDEXED_PICTURE_SIZE_TABLE = 0x0190, INDEXED_PICTURE_DIRECTORY_BYTES = 0x0258,
    GENERAL_PICTURE_READ_BYTES = 0x8000, WHOLE_PICTURE_READ_BYTES = 0xFFFF,
    PICTURE_BANK_LOW_OFFSET_WORD = 0x0002, PLANET_NAME_PANEL_WIDTH = 0x0066,
    PLANET_NAME_HEADER_BYTES = 0x000A, CAMPAIGN_GLYPH_ADVANCE = 0x0006,
    CALENDAR_YEAR_FIELD_WIDTH = 0x001E, CALENDAR_YEAR_GAP = 0x0004 };
extern const char indexedPictureFilename[], generalPictureFilename[];
extern const char cockpitPictureFilename[], primaryPictureFilename[];
extern const char secondaryPictureFilename[], tPictureFilename[];
extern const char alternatePictureFilename[], fPictureFilename[];

uint16_t MwLoadIndexedPicture(uint16_t pictureIndex);
void MwIndexedPictureCloseHook(void);
bool MwLoadGeneralPictureBank(void);
bool MwLoadCockpitPictureBank(void);
bool MwLoadPrimaryPictureBank(void);
bool MwSelectAndLoadPictureBank(const char *filename);
bool MwLoadSelectedPictureBank(void);
bool MwLoadSecondaryPictureBank(void);
bool MwLoadTPictureBank(void);
bool MwLoadAlternatePictureBank(void);
bool MwLoadFPictureBank(void);
[[noreturn]] void MwFailGameFileRead(const char *filename);
void MwDrawIndexedPicture(uint16_t x, uint16_t y, uint16_t pictureIndex);
uint16_t MwDrawGeneralPicture(uint16_t x, uint16_t y, uint8_t page, uint16_t pictureIndex);
uint16_t MwDrawAlternateBankPicture(uint16_t x, uint16_t y, uint8_t page, uint16_t pictureIndex);
uint16_t MwDrawActivePicture(uint16_t x, uint16_t y, uint8_t page, uint16_t pictureIndex);
uint16_t MwDrawFourthBankPicture(uint16_t x, uint16_t y, uint8_t page, uint16_t pictureIndex);

enum { CAMPAIGN_PLANET_COUNT = 0x0092, PLANET_RECORDS_START = 0x67B6,
    PLANET_RECORDS_END = 0x8A16, CAMPAIGN_MONTH_COUNT = 0x000C, DECIMAL_DIGIT_COUNT = 0x000A,
    DECIMAL_LAST_DIGIT = 0x0009, ASCII_DIGIT_ZERO = 0x30, ASCII_SPACE = 0x20,
    ASCII_MINUS = 0x2D, ASCII_COMMA = 0x2C,
    DECIMAL_BILLION = 0x3B9ACA00, DECIMAL_HUNDRED_MILLION = 0x05F5E100,
    DECIMAL_TEN_MILLION = 0x00989680, DECIMAL_MILLION = 0x000F4240,
    DECIMAL_HUNDRED_THOUSAND = 0x000186A0, DECIMAL_TEN_THOUSAND = 0x00002710,
    DECIMAL_THOUSAND = 0x03E8, DECIMAL_HUNDRED = 0x0064, DECIMAL_TEN = 0x000A };

enum { CAMPAIGN_HOUSE_COUNT = 0x0005, PLANET_LANDSCAPE_COUNT = 0x0003,
    PLANET_MARKET_PREMIUM_STEP = 0x0012, PLANET_PREMIUM_RANDOM_MAX = 0x0007,
    PLANET_NEGOTIATION_RANDOM_MAX = 0x0002, PERCENT_DIVISOR = 0x0064,
    AMMUNITION_PRICE_COUNT = 0x000C, REPAIR_ATTEMPT_KIND_COUNT = 0x000A,
    REPAIR_ATTEMPT_SLOTS_PER_KIND = 0x0016 };

/* Serialized, variable-length planet records: this prefix is followed by two
 * NUL-terminated strings. Character fields allow odd native record offsets;
 * population is a little-endian WORD, not an aligned host WORD member. */
typedef struct MwPlanetHeader
{
    unsigned char planetNumber;
    unsigned char houseIndex;
    unsigned char landscapeAndMarketCode;
    unsigned char maximumMissionType;
    unsigned char facilityDisplayFlag;
    unsigned char mapPositionX;
    unsigned char mapPositionY;
    unsigned char travelTimeWeight;
    unsigned char populationMillions[0x0002];
} MwPlanetHeader;
static_assert(sizeof(MwPlanetHeader) == PLANET_NAME_HEADER_BYTES, "Native planet prefix");
static_assert(offsetof(MwPlanetHeader, populationMillions) == 0x0008, "Native population offset");
static_assert(alignof(MwPlanetHeader) == 1, "Unaligned planet records");

typedef struct CampaignRepairAttemptRow
{
    uint8_t failedByDamageIndex[REPAIR_ATTEMPT_SLOTS_PER_KIND];
} CampaignRepairAttemptRow;
static_assert(sizeof(CampaignRepairAttemptRow) == 0x0016, "Native repair-attempt row");
void MwApplyCurrentPlanetState(void);
void MwResetRepairAttemptFailures(void);
void MwDrawPlanetNameCentered(uint16_t panelX, uint16_t y, uint16_t color, uint16_t planetIndex);
void MwDrawCampaignCalendar(uint16_t panelX, uint8_t yearDigits, uint8_t background,
    uint8_t foreground, uint16_t y);
void MwDrawSignedDecimalField(uint32_t signedValue, uint16_t x, uint8_t suppressZeros,
    uint8_t fieldDigits, uint8_t foreground, uint8_t background, uint16_t y);
void MwBuildDecimalDigitBuffer(uint32_t magnitude);
uint16_t MwDivideDecimalWordPlace(uint16_t value, uint16_t place, uint16_t *remainder);
uint32_t MwExtractDecimalDoubleWordPlace(uint32_t value, uint32_t place, uint16_t *digitCount);
void MwDrawNumericDigitsUnpadded(void);
void MwDrawNumericDigitsPadded(void);
void MwDrawFormattedNumericGlyphs(uint16_t digitIndex);
/* Campaign state owned by campaign.h (plan, slice B2). */
struct CampaignCoreState
{
    /* Native allocated-buffer WORDs; backing buffers belong to the allocator. */
    uint16_t indexedPictureOutputSelector, generalPictureBankSelector;
    uint16_t cockpitPictureBankSelector, activePictureBankSelector;
    uint16_t secondaryPictureBankSelector;
    uint8_t indexedPictureDirectoryLoaded;
    uint16_t indexedPictureCachedIndex, indexedPictureStoredBytes;
    uint8_t indexedPictureDirectory[INDEXED_PICTURE_DIRECTORY_BYTES];
    /* Native private filename pointer; identity is tested only against 0E2C.
     * Host pointer keeps that identity; never serialize this as a native WORD. */
    const char *selectedPictureFilename;
    /* Native record region, not another full DS mirror. Initial data/near offsets
     * come from the original image; future mutable views must use this same owner. */
    uint16_t planetRecordOffsets[CAMPAIGN_PLANET_COUNT];
    uint8_t planetRecordStorage[PLANET_RECORDS_END - PLANET_RECORDS_START];
    const uint8_t *calendarMonthNames[CAMPAIGN_MONTH_COUNT];
    uint16_t calendarMonthNameWidths[CAMPAIGN_MONTH_COUNT];
    uint16_t calendarMonthIndex, calendarYear;
    uint16_t calendarTextX, calendarTextY;
    uint8_t calendarBackground, calendarForeground;
    uint8_t numericLeadingDigitSeen, numericGroupingEnabled;
    uint8_t numericDecimalDigits[DECIMAL_DIGIT_COUNT];
    uint16_t numericRemainderHigh, numericRemainderLow;
    uint8_t numericSuppressLeadingZeros, numericNegativeSign;
    uint16_t numericFieldDigitCount, numericCursorX, numericCursorY;
    uint8_t numericBackgroundColour, numericForegroundColour;
    /* 1ACE's planet/market outputs are original WORDs even for BYTE record fields.
     * Original image tables are populated by native data initialization, not by a
     * substitute safe/default market. The compiled startup is still unconverted. */
    uint16_t currentPlanetIndex, currentPlanetLandscapeIndex;
    uint16_t currentPlanetHouseIndex, currentPlanetMarketTier;
    uint16_t currentPlanetFacilityDisplayFlag;
    uint16_t currentPlanetMapPositionX, currentPlanetMapPositionY;
    uint16_t planetMarketPremiumPercent, contractNegotiationAttemptLimit;
    uint8_t houseNegotiationAttemptBase[CAMPAIGN_HOUSE_COUNT];
    /* Six purchase prices followed by six sale prices, not twelve weapon prices. */
    uint16_t ammunitionBasePrices[AMMUNITION_PRICE_COUNT];
    uint16_t ammunitionTransactionPrices[AMMUNITION_PRICE_COUNT];
    /* Shared by kind/component, NOT by owned Mech slot. 3FFD clears all 00DC bytes,
     * including the two trailing slots per row not reached by normal repair work. */
    CampaignRepairAttemptRow repairAttemptFailuresByKind[REPAIR_ATTEMPT_KIND_COUNT];
    /* Canonical owner: src/Original/Memory/MW_MAIN_1090.c. Selectors remain
     * native WORD values; offsets wrap as WORDs, not host pointer arithmetic.
     * This is only the implemented slice of 1090, not its serialized layout. */
    /* Original 1090:016D. The allocator/loader assigns this selector later. */
    uint16_t compressedPictureBufferSelector;
    uint16_t randomStateWord0;
    uint16_t randomStateWord1;
    /* Original 1090:0F6D..0F76; decoder-private storage. BYTE phases remain BYTEs,
     * not bools: direct helpers treat every nonzero phase alike. One canonical
     * owner, not a second copy of a raw DS image or serialized host structure. */
    uint16_t pictureCompressedByteCount;
    uint16_t pictureOutputSelector;
    uint8_t pictureReadHighNibbleNext;
    uint16_t pictureReadByteOffset;
    uint8_t pictureWriteLowNibbleNext;
    uint16_t pictureWriteByteOffset;
};
extern CampaignCoreState campaignCore;

uint16_t MwRandomUpTo(uint16_t maximum);
/* Original AX=count, BX=output selector. Reads low nibble at source[count]
 * before rejecting its high nibble. Supply complete live segment windows;
 * do not zero a made-up EOF sentinel. */
void MwDecodePictureBank(uint16_t compressedByteCount, uint16_t outputSelector);
bool MwReadPictureNibble(uint8_t *nibble);
void MwWritePictureNibble(uint8_t nibble);

#endif
