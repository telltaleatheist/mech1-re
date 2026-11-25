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
/* The game draws on the canvas and sets its palette and mode there (G5); the
 * host shows the canvas's presented page, drawn with the canvas's palette,
 * while the canvas is the screen (set up, not closed). */
static uint8_t displayedIndices[EGA_COLUMNS*EGA_ROWS];
static uint32_t rgbaPixels[EGA_COLUMNS*EGA_ROWS];
static Canvas::Palette lastRgb; /* The canvas palette the presented frame was drawn with. */
static int lastPage;
static unsigned long lastSetUps; /* The canvas set-up the presented frame belongs to. */
static char captureDirectory[4096];
static bool haveFrame,presenting;
bool mwSdlCombatActive;
void (*mwSdlFrameObserver)(void);

/* A frame of the canvas as it is now set up: a mode set starts a new screen. */
static bool HaveFrame(void) {return haveFrame && lastSetUps==GameCanvas().setUps();}
/* The pointer in the frame's pixels (H3; INT 33h's x was twice them). */
void MwSdlSetMousePosition(uint16_t x,uint16_t y)
{
    float windowX,windowY;
    if (!renderer || !window || !SDL_RenderCoordinatesToWindow(renderer,(float)x,(float)y,&windowX,&windowY))
        MwSdlFail("Position mouse in EGA coordinates");
    SDL_WarpMouseInWindow(window,windowX,windowY);
}
void MwSdlReadMousePosition(uint16_t *x,uint16_t *y)
{
    float windowX,windowY,logicalX,logicalY;
    if (!x || !y || !renderer) MwSdlFail("Read mouse in EGA coordinates");
    SDL_GetMouseState(&windowX,&windowY);
    if (!SDL_RenderCoordinatesFromWindow(renderer,windowX,windowY,&logicalX,&logicalY))
        MwSdlFail("Read mouse in EGA coordinates");
    if (logicalX<0) logicalX=0;else if (logicalX>EGA_COLUMNS-1) logicalX=EGA_COLUMNS-1;
    if (logicalY<0) logicalY=0;else if (logicalY>EGA_ROWS-1) logicalY=EGA_ROWS-1;
    *x=(uint16_t)logicalX;
    *y=(uint16_t)logicalY;
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
static void PresentCanvasPage(const Canvas &canvas,int page,bool force);
bool MwSdlInitializeVideo(const char *directory)
{
    if (!directory) directory=".";
    if (strlen(directory)>=sizeof(captureDirectory)) return false;
    strcpy(captureDirectory,directory);
    /* mw1-original: MW1O_HIDDEN keeps the window hidden for headless test runs
     * (macOS's offscreen video driver has no renderer, as mw3-re's --hidden). */
    const SDL_WindowFlags hidden=getenv("MW1O_HIDDEN")?SDL_WINDOW_HIDDEN:0;
    window=SDL_CreateWindow("MechWarrior 1 Rebuild",960,600,SDL_WINDOW_RESIZABLE|hidden);
    if (!window) return false;
    renderer=SDL_CreateRenderer(window,NULL);
    if (!renderer || !SDL_SetRenderLogicalPresentation(renderer,EGA_COLUMNS,EGA_ROWS,SDL_LOGICAL_PRESENTATION_INTEGER_SCALE)) return false;
    texture=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,EGA_COLUMNS,EGA_ROWS);
    if (!texture || !SDL_SetTextureScaleMode(texture,SDL_SCALEMODE_NEAREST)) return false;
    GameCanvas().setPresenter([](const Canvas &canvas,int page) {PresentCanvasPage(canvas,page,true);});
    haveFrame=presenting=false;
    return true;
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
    if (!renderer || presenting || !GameCanvas().shown()) return;
    PresentCanvasPage(GameCanvas(),GameCanvas().presentedPage(),force);
}
/* The presenter: a canvas page, drawn with the canvas's palette. */
static void PresentCanvasPage(const Canvas &canvas,int page,bool force)
{
    if (!renderer || presenting || !canvas.shown()) return;
    const uint8_t *pixels=canvas.page(page);
    bool changed=force || !HaveFrame() || lastPage!=page || lastRgb!=canvas.palette();
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
        if (!HaveFrame() || !repeatPresentAt || now<repeatPresentAt) return;
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
    lastPage=page;lastRgb=palette;lastSetUps=canvas.setUps();haveFrame=true;
    repeatPresentAt=SDL_GetTicksNS()+HOST_REPEAT_SETTLE_NS;
    if (mwSdlFrameObserver) mwSdlFrameObserver();
    presenting=false;
}
const char *MwSdlCaptureDirectory(void) {return captureDirectory;}
/* A combat page as the capture's file has always printed it: the original's
 * segment (A000h and 200h more a page), 0 for none. */
static uint16_t CombatPageSegment(CombatPage page)
{return page==CombatPage::None?0:(uint16_t)(0xA000+((int)page-(int)CombatPage::First)*0x200);}
bool MwSdlCapture(const char *stem)
{
    if (!HaveFrame() || !stem || strpbrk(stem,"/\\:")) return false;
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
    /* The frame's colours: 17 entries (16, then the border) of red, green, blue. */
    if (snprintf(path,sizeof(path),"%s/%s.rgb",captureDirectory,stem)>=(int)sizeof(path)) return false;
    file=fopen(path,"wbx");if (!file) return false;
    for (const mw1o::video::Rgb &colour:lastRgb)
    {
        const uint8_t bytes[3]={colour.r,colour.g,colour.b};
        saved=saved && fwrite(bytes,1,3,file)==3;
    }
    if (fclose(file)) saved=false;
    if (snprintf(path,sizeof(path),"%s/%s.json",captureDirectory,stem)>=(int)sizeof(path)) return false;
    file=fopen(path,"wx");if (!file) return false;
    fprintf(file,"{\"kind\":\"Actual SDL renderer readback; not a matched original replay\",\"mode\":13,\"logicalWidth\":320,\"logicalHeight\":200,\"combat\":%s,\"displayPage\":%u,\"drawPage\":%u,\"displayByteOffset\":%u,\"SDL\":\"%s\",\"acceptance\":\"Pending\"}\n",mwSdlCombatActive?"true":"false",mwSdlCombatActive?CombatPageSegment(combatVideo.frontPage):egaOverlay.egaDisplayPage,mwSdlCombatActive?CombatPageSegment(combatVideo.drawPage):egaOverlay.egaDrawPage,
        (unsigned)(lastPage*(Canvas::PAGE_STRIDE/EGA_PIXELS_PER_BYTE)),SDL_GetRevision());
    if (fclose(file)) saved=false;
    return saved;
}
void MwSdlShutdownVideo(void)
{
    SDL_DestroyTexture(texture);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);
    texture=NULL;renderer=NULL;window=NULL;
}
