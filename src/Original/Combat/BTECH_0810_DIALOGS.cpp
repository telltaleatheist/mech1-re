#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_input.h"
#include <stdlib.h>
#include <string.h>

enum { DIALOG_LEFT=0x6B,DIALOG_TOP=0x57,DIALOG_WIDTH=0x6B,DIALOG_HEIGHT=0x1B,
    DIALOG_CENTRE_X=0xA0,DIALOG_FIRST_TEXT_Y=0x5E,DIALOG_SECOND_TEXT_Y=0x66,
    DIALOG_HALF_CHARACTER_WIDTH=3,DIALOG_DELAY=0x18,DIALOG_TEXT_COLOUR=Combat_Pal_EGA_LightGrey,
    DIALOG_MONOCHROME_TEXT_COLOUR=Combat_Pal_EGA_White,DIALOG_Y_SCAN_CODE=0x15 };

/*0810:137D. Original four-byte local is two near text references. */
void BtechShowSimulationPausedDialog(void)
{
    BtechDialogMessagePair message={combatCockpit.simulationText,combatCockpit.pausedText};
    BtechShowCapturedTwoLineDialog(&message);
}

/*0810:1398. Genuine CALL137D/RETF veneer. */
void BtechShowSimulationPausedDialogFar(void)
{
    BtechShowSimulationPausedDialog();
}

/*0810:27C7. The saved scan, not ASCII or the later release poll. */
uint16_t BtechConfirmRetreatDialog(void)
{
    BtechDialogMessagePair message={combatCockpit.retreatQuestionText,combatCockpit.retreatChoiceText};
    return BtechShowCapturedTwoLineDialog(&message)==DIALOG_Y_SCAN_CODE;
}

/*0810:27F4. Allocation failure still waits. Success deliberately leaks the
 * capture allocation until DOS child disposal: native has NO1F10 here. */
uint16_t BtechShowCapturedTwoLineDialog(const BtechDialogMessagePair *message)
{
    BtechSaveDrawingState();
    BtechWaitScaledRuntimeDelay(DIALOG_DELAY);
    uint16_t captureBytes=(uint16_t)BtechGetCaptureStorageSize(DIALOG_WIDTH,DIALOG_HEIGHT);
    BtechResourceAllocation capture={BtechAllocateDosMemoryBlock(captureBytes).bytes,captureBytes};
    if (capture.bytes)
    {
        drawPageSelector=frontPageSelector;copySourcePageSelector=frontPageSelector;
        BtechCaptureScreenRectangle(capture,DIALOG_LEFT,DIALOG_TOP,DIALOG_WIDTH,DIALOG_HEIGHT);
        screenPolygonNeedsClip=false;drawPolygonFilled=true;textTransparentBackground=true;
        textForegroundColour=hudColourSchemeIndex?DIALOG_MONOCHROME_TEXT_COLOUR:DIALOG_TEXT_COLOUR;
        if (!promptBitmaps || !promptBitmaps->images || !promptBitmaps->imageCount) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !promptBitmaps || !promptBitmaps->images || !promptBitmaps->imageCount");
        BtechDrawBitmap(&promptBitmaps->images[0],DIALOG_LEFT,DIALOG_TOP);
        uint16_t textHalfWidth=strlen(message->firstText)*DIALOG_HALF_CHARACTER_WIDTH;
        BtechDrawNearText(message->firstText,(int16_t)(DIALOG_CENTRE_X-textHalfWidth),DIALOG_FIRST_TEXT_Y);
        textHalfWidth=(uint16_t)(strlen(message->secondText)*DIALOG_HALF_CHARACTER_WIDTH);
        BtechDrawNearText(message->secondText,(int16_t)(DIALOG_CENTRE_X-textHalfWidth),DIALOG_SECOND_TEXT_Y);
        textTransparentBackground=false;
    }
    uint16_t scanCode=BtechWaitForDialogKey();
    if (capture.bytes) BtechRestoreScreenRectangle(capture,DIALOG_LEFT,DIALOG_TOP,DIALOG_WIDTH,DIALOG_HEIGHT);
    BtechRestoreDrawingState();
    return scanCode;
}

/*0810:2962. CBW sign-extends each BYTE. Transparency, font and clip bounds
 * are NOT saved by the original. Keep these independent native owners. */
void BtechSaveDrawingState(void)
{
    combatCockpit.savedPolygonFilled=(int8_t)drawPolygonFilled;
    combatCockpit.savedPolygonClip=(int8_t)screenPolygonNeedsClip;
    combatCockpit.savedTextBackground=(int8_t)textBackgroundColour;
    combatCockpit.savedTextForeground=(int8_t)textForegroundColour;
    combatCockpit.savedFillColour=(int8_t)drawFillColour;
    combatCockpit.savedLineColour=(int8_t)drawLineColour;
    combatCockpit.savedCopySourcePage=copySourcePageSelector;combatCockpit.savedDrawPage=drawPageSelector;
}

/*0810:2999. Restore LOW BYTEs in original order, then page WORDs. */
void BtechRestoreDrawingState(void)
{
    drawPolygonFilled=(uint8_t)combatCockpit.savedPolygonFilled;screenPolygonNeedsClip=(uint8_t)combatCockpit.savedPolygonClip;
    textBackgroundColour=(uint8_t)combatCockpit.savedTextBackground;textForegroundColour=(uint8_t)combatCockpit.savedTextForeground;
    drawFillColour=(uint8_t)combatCockpit.savedFillColour;drawLineColour=(uint8_t)combatCockpit.savedLineColour;
    copySourcePageSelector=combatCockpit.savedCopySourcePage;drawPageSelector=combatCockpit.savedDrawPage;
}
