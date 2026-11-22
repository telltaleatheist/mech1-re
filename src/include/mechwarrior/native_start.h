#ifndef MECHWARRIOR_NATIVE_START_H
#define MECHWARRIOR_NATIVE_START_H
#include <stddef.h>
#include <stdint.h>

/* Start values that are the program's own constants, not game state, and the
 * file names it opens (docs/phase2/PLAN.md 8.3, classes B, C and E; slice
 * 6b). The importers set them from here, not from the exe; start_exe_test
 * checks every row of mwNativeStartRows against the player's exes, by id. */
enum {
    MW_JOYSTICK_PREVIOUS_BUTTONS_START=0xFF,   /* MW_MAIN 1090:02EC */
    MW_JOYSTICK_DEFLECTION_START=1,            /* 1090:02F1 */
    MW_INDEXED_PICTURE_NO_CACHED_INDEX=0xFFFF, /* 1090:0B63 */
    MW_HIRED_TRANSPORT_REVERSE_END=0xFFFF      /* 1090:5AE8, before the frames at 5AEA */
};
/* The joystick's navigation keys (scan codes in the high byte), 1090:02F6. */
extern const uint16_t mwJoystickNavigationKeysStart[9];
/* File names: BTECH for the deployment's check (1090:0EF6), the saves'
 * search pattern (1090:A056), and the launcher's two children (MW.EXE
 * 0806:07E3, 07EF). */
extern const char mwCombatExecutableFilename[10], mwSaveFindPattern[6];
extern const char mwLauncherCampaignFilename[12], mwLauncherCombatFilename[10];
/* The travel distance table (1090:4CE6, TRAVEL_ROOT_TABLE_COUNT words):
 * round(sqrt(10*i)) capped at 99, except where the exe holds another value
 * (class C, generated; start_exe_test checks every entry). */
void MwBuildTravelSquareRoots(uint16_t *values,size_t count);

/* One checked row: `fill` writes the row's `size` bytes as the exe lays them
 * out (little-endian words). */
typedef struct MwNativeStartRow
{
    const char *id,*exe;
    uint16_t segment,offset,size;
    void (*fill)(uint8_t *bytes);
} MwNativeStartRow;
extern const MwNativeStartRow mwNativeStartRows[];
extern const size_t mwNativeStartRowCount;
#endif
