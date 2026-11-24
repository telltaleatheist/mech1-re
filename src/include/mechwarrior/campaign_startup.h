#ifndef MECHWARRIOR_CAMPAIGN_STARTUP_H
#define MECHWARRIOR_CAMPAIGN_STARTUP_H
#include "mechwarrior/campaign_input.h"
#include "mechwarrior/native_address.h"
#include "mechwarrior/campaign_exchange.h"

enum { LAUNCHER_IDENTITY_BYTES=11 };
enum { CAMPAIGN_PHASE_NEW=1, CAMPAIGN_PHASE_COMBAT_RETURN=2,
    CAMPAIGN_EXIT_QUIT=0, CAMPAIGN_EXIT_COMBAT=1 };
/* Host projection of the launcher's PSP/FCB contract, not a serialized struct
 * or additional source arguments. The identity is the launcher's eleven bytes
 * beyond the exchange buffer, which 0002 tests at 0, 4 and 10. */
typedef struct MwCampaignLaunch
{
    const uint8_t (&identity)[LAUNCHER_IDENTITY_BYTES];
    uint8_t phase, graphicsCode, memoryTier;
} MwCampaignLaunch;

typedef struct MwCampaignPlatformCalls
{
    bool (*allocateParagraphs)(uint16_t paragraphs,uint16_t *selectorOrError);
    bool (*loadGraphicsOverlay)(const char *filename,uint16_t selector);
    void (*installInterrupts)(void);
    void (*restoreInterrupts)(void);
    uint32_t (*readBiosTicks)(void);
    bool (*mousePresent)(void); /* A mouse to read (H3); the original reset INT 33h to ask. */
    void (*displayDosString)(const char *text);
} MwCampaignPlatformCalls;
extern MwCampaignPlatformCalls mwCampaignPlatform;
/* Original FAR music calls across the campaign/runtime boundary. Host bindings
 * forward to the converted sequencer; they do not choose cues or play files. */
extern void (*campaignStopMusic)(void);
extern void (*campaignStartMusic)(uint16_t selector,uint16_t offset);
uint8_t MwRunCampaign(const MwCampaignLaunch *launch);
void MwInstallCampaignInterruptHandlers(void);
void MwRestoreCampaignInterruptHandlers(void);
void MwSaveCurrentVideoMode(void);
void MwRestoreSavedVideoMode(void);
void MwDetectMouseAndJoystick(void);
void MwDetectAndCalibrateInputDevices(void);
void MwMixBiosTicksIntoRandomState(void);
typedef struct MwIntroPositionedText
{
    uint16_t x,y;
    char text[0x80];
} MwIntroPositionedText;
bool MwLoadMechMusic(void);
bool MwOpenAndLoadMusicFile(const char *filename);
void MwDrawIntroNarrationLines(uint8_t colour);
bool MwWaitTicksOrInputInterruption(uint16_t delayTicks);
void MwSetIntroTextPaletteColour(uint8_t paletteCode);
void MwDrawPositionedIntroText(const MwIntroPositionedText *text);
/* Host projection of a native nonlocal transfer, NOT an AX result ABI. */
uint16_t MwDrawTimedPictureAndTextTransition(uint16_t delayTicks,
    const MwIntroPositionedText *leftText,const MwIntroPositionedText *rightText);
bool MwLoadBarDeathAndLaunchMusic(void);
/* Folded166D/181A flow. Returned code offset is host JMP transport, not AX. */
uint16_t MwStartNewCampaign(uint16_t entryOffset);
/* Campaign state owned by campaign_startup.h (plan, slice B2). */
struct CampaignStartupState
{
    uint16_t campaignMusicBufferSelector, graphicsOverlaySelector;
    uint16_t launcherMemoryTier;
    uint8_t launcherExitCode, savedVideoMode;
    uint8_t keyboardInterruptState;
    char introNarrationLines[10][0x50];
    MwIntroPositionedText introFirstLeftText, introFirstRightText;
    MwIntroPositionedText introSecondLeftText, introSecondRightText;
    MwIntroPositionedText introThirdLeftText, introThirdRightText;
    MwIntroPositionedText introFourthLeftText, introFourthRightText;
    MwIntroPositionedText introFifthLeftText, introFifthRightText;
    MwIntroPositionedText introSixthLeftText, introSixthRightText;
    MwIntroPositionedText introSeventhLeftText, introSeventhRightText;
    uint8_t campaignTextInputBuffer[0x29];
    uint16_t campaignEventPlanetSelectors[6];
    uint8_t eventPlanetChoicePool0[8], eventPlanetChoicePool1[3];
    uint8_t eventPlanetChoicePool2[4], eventPlanetChoicePool3[3];
    uint8_t eventPlanetChoicePool4[4], eventPlanetChoicePool5[3];
};
extern CampaignStartupState campaignStartup;
#endif
