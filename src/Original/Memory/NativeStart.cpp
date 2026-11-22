/* The program's own start constants and file names (native_start.h). */
#include "mechwarrior/native_start.h"
#include <string.h>

const uint16_t mwJoystickNavigationKeysStart[9]={0x0000,0x4800,0x4800,0x4D00,0x5000,0x5000,0x5000,0x4B00,0x4800};
const char mwCombatExecutableFilename[10]="BTECH.EXE",mwSaveFindPattern[6]="*.GAM";
const char mwLauncherCampaignFilename[12]="MW_MAIN.EXE",mwLauncherCombatFilename[10]="BTECH.EXE";

/* The rounded root, in integers: (isqrt(40*i)+1)/2 is round(sqrt(10*i)). */
static uint16_t RoundedRootOfTenTimes(uint32_t i)
{
    const uint32_t n=40*i;
    uint32_t root=0,bit=1u<<30;
    while (bit>n) bit>>=2;
    for (uint32_t rest=n;bit;bit>>=2)
    {
        if (rest>=root+bit) {rest-=root+bit;root=(root>>1)+bit;}
        else root>>=1;
    }
    return (uint16_t)((root+1)/2);
}
void MwBuildTravelSquareRoots(uint16_t *values,size_t count)
{
    /* Measured from the exe: these two hold 98 where the formula gives 99. */
    static const struct {uint16_t index,value;} exceptions[]={{991,98},{992,98}};
    for (size_t i=0;i<count;++i)
    {
        const uint16_t root=RoundedRootOfTenTimes((uint32_t)i);
        values[i]=root>99?99:root;
    }
    for (size_t e=0;e<sizeof(exceptions)/sizeof(exceptions[0]);++e)
        if (exceptions[e].index<count) values[exceptions[e].index]=exceptions[e].value;
}

static void PutWords(uint8_t *bytes,const uint16_t *words,size_t count)
{
    for (size_t i=0;i<count;++i) {bytes[i*2]=(uint8_t)words[i];bytes[i*2+1]=(uint8_t)(words[i]>>8);}
}
static void FillJoystickButtons(uint8_t *bytes) {bytes[0]=MW_JOYSTICK_PREVIOUS_BUTTONS_START;}
static void FillJoystickDeflection(uint8_t *bytes) {const uint16_t w=MW_JOYSTICK_DEFLECTION_START;PutWords(bytes,&w,1);}
static void FillJoystickKeys(uint8_t *bytes) {PutWords(bytes,mwJoystickNavigationKeysStart,9);}
static void FillPictureIndex(uint8_t *bytes) {const uint16_t w=MW_INDEXED_PICTURE_NO_CACHED_INDEX;PutWords(bytes,&w,1);}
static void FillTransportEnd(uint8_t *bytes) {const uint16_t w=MW_HIRED_TRANSPORT_REVERSE_END;PutWords(bytes,&w,1);}
static void FillSquareRoots(uint8_t *bytes) {uint16_t values[1000];MwBuildTravelSquareRoots(values,1000);PutWords(bytes,values,1000);}
static void FillCombatName(uint8_t *bytes) {memcpy(bytes,mwCombatExecutableFilename,sizeof(mwCombatExecutableFilename));}
static void FillSavePattern(uint8_t *bytes) {memcpy(bytes,mwSaveFindPattern,sizeof(mwSaveFindPattern));}
static void FillCampaignName(uint8_t *bytes) {memcpy(bytes,mwLauncherCampaignFilename,sizeof(mwLauncherCampaignFilename));}
static void FillCombatChildName(uint8_t *bytes) {memcpy(bytes,mwLauncherCombatFilename,sizeof(mwLauncherCombatFilename));}

const MwNativeStartRow mwNativeStartRows[]={
    {"joystick.previous_buttons","MW_MAIN.EXE",0x1090,0x02EC,1,FillJoystickButtons},
    {"joystick.deflection","MW_MAIN.EXE",0x1090,0x02F1,2,FillJoystickDeflection},
    {"joystick.navigation_keys","MW_MAIN.EXE",0x1090,0x02F6,18,FillJoystickKeys},
    {"picture.no_cached_index","MW_MAIN.EXE",0x1090,0x0B63,2,FillPictureIndex},
    {"travel.hired_transport_reverse_end","MW_MAIN.EXE",0x1090,0x5AE8,2,FillTransportEnd},
    {"travel.square_roots","MW_MAIN.EXE",0x1090,0x4CE6,2000,FillSquareRoots},
    {"file.combat_executable","MW_MAIN.EXE",0x1090,0x0EF6,10,FillCombatName},
    {"file.save_pattern","MW_MAIN.EXE",0x1090,0xA056,6,FillSavePattern},
    {"file.launcher_campaign","MW.EXE",0x0806,0x07E3,12,FillCampaignName},
    {"file.launcher_combat","MW.EXE",0x0806,0x07EF,10,FillCombatChildName}};
const size_t mwNativeStartRowCount=sizeof(mwNativeStartRows)/sizeof(mwNativeStartRows[0]);
