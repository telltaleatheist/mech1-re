#include <stdlib.h>
#include "SDLBackend.h"
#include "mechwarrior/ega_campaign.h"
#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_startup.h"
#include "Canvas.h"
#include <stdio.h>
#include <string.h>

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
using mw1o::video::Canvas;
using mw1o::video::GameCanvas;
static_assert(Canvas::WIDTH==EGA_COLUMNS && Canvas::HEIGHT==EGA_ROWS && Canvas::BORDER+1==EGA_PALETTE_ENTRIES,
    "The canvas is the EGA mode's frame");
/* The game's palette codes as it set them: the trace captures' .palette, and
 * what decides a palette change, exactly as before the canvas. The canvas's
 * RGB palette is what the frame is drawn with. */
static uint8_t videoMode=3,paletteCodes[EGA_PALETTE_ENTRIES];
static uint8_t displayedIndices[EGA_COLUMNS*EGA_ROWS],lastPalette[EGA_PALETTE_ENTRIES];
static uint32_t rgbaPixels[EGA_COLUMNS*EGA_ROWS];
static int lastPage;
static char captureDirectory[4096];
static bool haveFrame,presenting;
void (*mwSdlFrameObserver)(void);

static void SetPaletteCode(uint8_t index,uint8_t code)
{
    paletteCodes[index]=code;GameCanvas().setPaletteEntry(index,mw1o::video::PaletteCodeColour(code));
}
void MwSdlSetVideoMode(uint8_t mode)
{
    if (mode!=3 && mode!=EGA_BIOS_MODE_320X200) MwSdlFail("Unsupported video mode");
    videoMode=mode;
    if (mode==EGA_BIOS_MODE_320X200)
    {
        GameCanvas().clear();egaLogicalFunction=EGA_COPY;
        MwSdlResetCombatVideoHardware();
        for (uint8_t i=0;i<16;++i) SetPaletteCode(i,(i&7)|((i&8)?0x10:0));
        SetPaletteCode(Canvas::BORDER,EGA_PALETTE_BLACK);haveFrame=false;
        MwSdlResetCombatTick();
    }
}
uint8_t MwSdlGetVideoMode(void) {return videoMode;}
void MwSdlSetMousePosition(uint16_t horizontalPosition,uint16_t verticalPosition)
{
    float windowX,windowY;
    if (!renderer || !window || !SDL_RenderCoordinatesToWindow(renderer,
        horizontalPosition/2.0f,(float)verticalPosition,&windowX,&windowY))
        MwSdlFail("Position mouse in EGA coordinates");
    SDL_WarpMouseInWindow(window,windowX,windowY);
}
void MwSdlReadMousePosition(uint16_t *horizontalPosition,uint16_t *verticalPosition)
{
    float windowX,windowY,logicalX,logicalY;
    if (!horizontalPosition || !verticalPosition || !renderer) MwSdlFail("Read mouse in EGA coordinates");
    SDL_GetMouseState(&windowX,&windowY);
    if (!SDL_RenderCoordinatesFromWindow(renderer,windowX,windowY,&logicalX,&logicalY))
        MwSdlFail("Read mouse in EGA coordinates");
    if (logicalX<0) logicalX=0;else if (logicalX>EGA_COLUMNS-1) logicalX=EGA_COLUMNS-1;
    if (logicalY<0) logicalY=0;else if (logicalY>EGA_ROWS-1) logicalY=EGA_ROWS-1;
    *horizontalPosition=(uint16_t)logicalX<<1;
    *verticalPosition=(uint16_t)logicalY;
}
void MwSdlConvertMouseMotion(float windowXMotion,float windowYMotion,
    float *logicalXMotion,float *logicalYMotion)
{
    float originX,originY,movedX,movedY;
    if (!logicalXMotion || !logicalYMotion || !renderer
        || !SDL_RenderCoordinatesFromWindow(renderer,0,0,&originX,&originY)
        || !SDL_RenderCoordinatesFromWindow(renderer,windowXMotion,windowYMotion,&movedX,&movedY))
        MwSdlFail("Convert mouse motion to EGA coordinates");
    *logicalXMotion=movedX-originX;*logicalYMotion=movedY-originY;
}
static void PaletteRegister(uint8_t index,uint8_t code)
{if (index>=EGA_PALETTE_ENTRIES) MwSdlFail("Unsupported EGA palette register");SetPaletteCode(index,code);}
static void PaletteTable(const uint8_t *codes) {for (uint8_t i=0;i<EGA_PALETTE_ENTRIES;++i) SetPaletteCode(i,codes[i]);}
static void OverscanRegister(uint8_t code) {SetPaletteCode(Canvas::BORDER,code);}
/* The canvas page an EGA display start (a byte offset into the aperture) shows.
 * Both the campaign's page selectors and combat's CRTC start (high byte only)
 * fall on a page. */
static int DisplayStartPage(uint16_t byteOffset)
{
    const uint32_t pageBytes=(uint32_t)(Canvas::PAGE_STRIDE/EGA_PIXELS_PER_BYTE);
    if (byteOffset%pageBytes) MwSdlFail("EGA display start is not on a canvas page");
    return (int)(byteOffset/pageBytes);
}
static int DisplayedPage(void)
{
    return DisplayStartPage(mwSdlCombatActive?MwSdlCombatDisplayByteOffset()
        :(uint16_t)((egaDisplayPage-EGA_FIRST_PAGE_SELECTOR)*EGA_SELECTOR_PARAGRAPH_BYTES));
}
static void PresentCanvasPage(const Canvas &canvas,int page,bool force);
static void PresentPage(uint16_t page)
{
    if (page!=egaDisplayPage) MwSdlFail("EGA display-page binding mismatch");
    GameCanvas().present(DisplayedPage());
}
bool MwSdlInitializeVideo(const char *directory)
{
    if (!directory) directory=".";
    if (strlen(directory)>=sizeof(captureDirectory)) return false;
    strcpy(captureDirectory,directory);
    /* mw1-original: MW1O_HIDDEN keeps the window hidden for headless test runs
     * (macOS's offscreen video driver has no renderer, as mw3-re's --hidden). */
    const SDL_WindowFlags hidden=getenv("MW1O_HIDDEN")?SDL_WINDOW_HIDDEN:0;
    window=SDL_CreateWindow("MechWarrior — EGA",960,600,SDL_WINDOW_RESIZABLE|hidden);
    if (!window) return false;
    MwSdlUpdateWindowTitle();
    renderer=SDL_CreateRenderer(window,NULL);
    if (!renderer || !SDL_SetRenderLogicalPresentation(renderer,EGA_COLUMNS,EGA_ROWS,SDL_LOGICAL_PRESENTATION_INTEGER_SCALE)) return false;
    texture=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,EGA_COLUMNS,EGA_ROWS);
    if (!texture || !SDL_SetTextureScaleMode(texture,SDL_SCALEMODE_NEAREST)) return false;
    GameCanvas().setPresenter([](const Canvas &canvas,int page) {PresentCanvasPage(canvas,page,true);});
    egaSetVideoMode=MwSdlSetVideoMode;egaSetPaletteRegister=PaletteRegister;
    egaSetPaletteTable=PaletteTable;egaPresentPage=PresentPage;
    btechHostSetPaletteRegister=PaletteRegister;btechHostSetOverscanColour=OverscanRegister;
    MwSdlBindCombatVideo();
    haveFrame=presenting=false;videoMode=3;
    return true;
}
void MwSdlUpdateWindowTitle(void)
{
    if (!window) return;
    const char *soundDriver=mwSdlMusicSoundfont?"Soundfont":mwSdlSoundSystem==MW_SOUND_MT32?"Roland MT-32"
        :mwSdlSoundSystem==MW_SOUND_ADLIB?"AdLib":"PC speaker";
    char title[128];
    SDL_snprintf(title,sizeof(title),"MechWarrior — EGA / %s",soundDriver);
    if (!SDL_SetWindowTitle(window,title)) MwSdlFail("Update sound driver window title");
}
static void Render(void)
{
    if (!SDL_SetRenderDrawColor(renderer,0,0,0,255) || !SDL_RenderClear(renderer)
        || !SDL_RenderTexture(renderer,texture,NULL,NULL)) MwSdlFail("Render EGA texture");
}
/* A small boxed line at the top right of the 320x200 frame (SDL's 8x8 font). */
static void DrawNotice(const char *text)
{
    float width=(float)(strlen(text)*SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE),x=(float)EGA_COLUMNS-width-6.0f,y=4.0f;
    SDL_FRect box={x-3.0f,y-3.0f,width+6.0f,(float)SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE+6.0f};
    if (!SDL_SetRenderDrawColor(renderer,0,0,0,255) || !SDL_RenderFillRect(renderer,&box)
        || !SDL_SetRenderDrawColor(renderer,255,255,85,255) || !SDL_RenderDebugText(renderer,x,y,text))
        MwSdlFail("Draw the speed notice");
}
/* A frame is presented only when it changes, so a frame the window system
 * drops or holds back (seen on Windows: two presents a few ms apart, the
 * second never shown until the next change) would stay off the screen while
 * the game waits for a key. The same frame is presented again 50 ms after
 * each change, then once a second while nothing changes. A repeat touches
 * neither displayedIndices nor the frame observer: captures and traces see
 * the same frames as before. */
enum { HOST_REPEAT_SETTLE_NS=50000000,HOST_REPEAT_IDLE_NS=1000000000 };
static uint64_t repeatPresentAt;
void MwSdlRepeatPresent(void) {repeatPresentAt=1;}
void MwSdlPresentVideo(bool force)
{
    if (!renderer || presenting || videoMode!=EGA_BIOS_MODE_320X200) return;
    PresentCanvasPage(GameCanvas(),DisplayedPage(),force);
}
/* The presenter: a canvas page, drawn with the canvas's palette. */
static void PresentCanvasPage(const Canvas &canvas,int page,bool force)
{
    if (!renderer || presenting || videoMode!=EGA_BIOS_MODE_320X200) return;
    const uint8_t *pixels=canvas.page(page);
    bool changed=force || !haveFrame || lastPage!=page || memcmp(lastPalette,paletteCodes,sizeof(paletteCodes));
    for (uint32_t pixel=0;pixel<EGA_COLUMNS*EGA_ROWS;++pixel)
    {
        uint8_t index=pixels[pixel];
        if (index>=Canvas::COLOURS) MwSdlFail("Unrepresented EGA index");
        if (displayedIndices[pixel]!=index) changed=true;
        displayedIndices[pixel]=index;
    }
    /* The speed notice is drawn over the frame at present time only: captures
     * and trace frames (displayedIndices) never contain it. */
    static bool noticeShown;
    char notice[32];bool showNotice=mwSdlCombatActive && MwSdlCombatSpeedNotice(notice,sizeof(notice));
    if (showNotice!=noticeShown) changed=true;
    if (!changed)
    {
        uint64_t now=SDL_GetTicksNS();
        if (!haveFrame || !repeatPresentAt || now<repeatPresentAt) return;
        presenting=true;
        Render();
        if (showNotice) DrawNotice(notice);
        if (!SDL_RenderPresent(renderer)) MwSdlFail("Present EGA texture");
        repeatPresentAt=now+HOST_REPEAT_IDLE_NS;
        presenting=false;
        return;
    }
    presenting=true;
    const Canvas::Palette &palette=canvas.palette();
    for (uint32_t pixel=0;pixel<EGA_COLUMNS*EGA_ROWS;++pixel)
    {
        const mw1o::video::Rgb colour=palette[displayedIndices[pixel]];
        rgbaPixels[pixel]=0xFF000000u|((uint32_t)colour.r<<16)|((uint32_t)colour.g<<8)|colour.b;
    }
    if (!SDL_UpdateTexture(texture,NULL,rgbaPixels,EGA_COLUMNS*sizeof(uint32_t))) MwSdlFail("Upload EGA texture");
    Render();
    if (showNotice) DrawNotice(notice);
    noticeShown=showNotice;
    if (!SDL_RenderPresent(renderer)) MwSdlFail("Present EGA texture");
    lastPage=page;memcpy(lastPalette,paletteCodes,sizeof(lastPalette));haveFrame=true;
    repeatPresentAt=SDL_GetTicksNS()+HOST_REPEAT_SETTLE_NS;
    if (mwSdlFrameObserver) mwSdlFrameObserver();
    presenting=false;
}
const char *MwSdlCaptureDirectory(void) {return captureDirectory;}
bool MwSdlCapture(const char *stem)
{
    if (!haveFrame || !stem || strpbrk(stem,"/\\:")) return false;
    char path[4096];
    if (snprintf(path,sizeof(path),"%s/%s.bmp",captureDirectory,stem)>=(int)sizeof(path)) return false;
    /* Reserve exclusively: never overwrite an earlier observation. */
    FILE *file=fopen(path,"wbx");if (!file) return false;fclose(file);
    Render();
    SDL_Surface *surface=SDL_RenderReadPixels(renderer,NULL);
    if (!surface) return false;
    bool saved=SDL_SaveBMP(surface,path);SDL_DestroySurface(surface);
    if (!SDL_RenderPresent(renderer)) saved=false;
    if (!saved) return false;
    if (snprintf(path,sizeof(path),"%s/%s.indices",captureDirectory,stem)>=(int)sizeof(path)) return false;
    file=fopen(path,"wbx");if (!file) return false;
    saved=fwrite(displayedIndices,1,sizeof(displayedIndices),file)==sizeof(displayedIndices);if (fclose(file)) saved=false;
    if (snprintf(path,sizeof(path),"%s/%s.palette",captureDirectory,stem)>=(int)sizeof(path)) return false;
    file=fopen(path,"wbx");if (!file) return false;
    saved=saved && fwrite(lastPalette,1,sizeof(lastPalette),file)==sizeof(lastPalette);if (fclose(file)) saved=false;
    if (snprintf(path,sizeof(path),"%s/%s.json",captureDirectory,stem)>=(int)sizeof(path)) return false;
    file=fopen(path,"wx");if (!file) return false;
    fprintf(file,"{\"kind\":\"Actual SDL renderer readback; not a matched original replay\",\"mode\":13,\"logicalWidth\":320,\"logicalHeight\":200,\"combat\":%s,\"displayPage\":%u,\"drawPage\":%u,\"displayByteOffset\":%u,\"SDL\":\"%s\",\"acceptance\":\"Pending\"}\n",mwSdlCombatActive?"true":"false",mwSdlCombatActive?frontPageSelector:egaDisplayPage,mwSdlCombatActive?drawPageSelector:egaDrawPage,
        (unsigned)(lastPage*(Canvas::PAGE_STRIDE/EGA_PIXELS_PER_BYTE)),SDL_GetRevision());
    if (fclose(file)) saved=false;
    return saved;
}
void MwSdlShutdownVideo(void)
{
    SDL_DestroyTexture(texture);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);
    texture=NULL;renderer=NULL;window=NULL;
}
