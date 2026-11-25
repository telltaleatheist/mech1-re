#include <stdlib.h>
#include "SDLBackend.h"
#include "SDLPicture.h"
#include "mechwarrior/ega_campaign.h"
#include "mechwarrior/combat_video.h"
#include "mechwarrior/combat_startup.h"
#include "Canvas.h"
#include <stdio.h>
#include <string.h>

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture; /* shownPixels on the device; NULL until it is (re)made. */
static MwSdlPicture picture;
using mw1o::video::Canvas;
using mw1o::video::GameCanvas;
static_assert(Canvas::WIDTH==EGA_COLUMNS && Canvas::HEIGHT==EGA_ROWS && Canvas::BORDER+1==EGA_PALETTE_ENTRIES,
    "The canvas is the EGA mode's frame");
/* The game draws on the canvas and sets its palette and mode there (G5); the
 * host shows the canvas's presented page, drawn with the canvas's palette,
 * while the canvas is the screen (set up, not closed). */
static uint8_t displayedIndices[EGA_COLUMNS*EGA_ROWS];
static uint32_t rgbaPixels[EGA_COLUMNS*EGA_ROWS];
/* The frame with the speed notice on it, as it is shown. The notice is drawn
 * here, by a software renderer, and not on the device: SDL keeps the font's
 * texture itself, and doesn't make it again after a device reset. */
static uint32_t shownPixels[EGA_COLUMNS*EGA_ROWS];
static SDL_Surface *shownSurface;
static SDL_Renderer *noticeRenderer;
static bool deviceLost,drawErrorSaid;
static Canvas::Palette lastRgb; /* The canvas palette the presented frame was drawn with. */
static int lastPage;
static unsigned long lastSetUps; /* The canvas set-up the presented frame belongs to. */
static char captureDirectory[4096];
static bool haveFrame,presenting;
bool mwSdlCombatActive;
void (*mwSdlFrameObserver)(void);

/* A frame of the canvas as it is now set up: a mode set starts a new screen. */
static bool HaveFrame(void) {return haveFrame && lastSetUps==GameCanvas().setUps();}
/* The picture as the window now is (docs/WINDOW.md), or as it last was: a
 * lost device has no size. */
static const MwSdlPicture &Picture(const char *what)
{
    if (!renderer || !window) MwSdlFail(what);
    MwSdlReadPicture(window,renderer,&picture);
    return picture;
}
/* The pointer in the frame's pixels (H3; INT 33h's x was twice them). */
void MwSdlSetMousePosition(uint16_t x,uint16_t y)
{
    const SDL_FPoint point=MwSdlPointInFramePixel(Picture("Position mouse in EGA coordinates"),x,y);
    SDL_WarpMouseInWindow(window,point.x,point.y);
}
void MwSdlReadMousePosition(uint16_t *x,uint16_t *y)
{
    if (!x || !y) MwSdlFail("Read mouse in EGA coordinates");
    SDL_FPoint point;
    SDL_GetMouseState(&point.x,&point.y);
    const SDL_Point pixel=MwSdlFramePixel(Picture("Read mouse in EGA coordinates"),point);
    *x=(uint16_t)pixel.x;
    *y=(uint16_t)pixel.y;
}
void MwSdlConvertMouseMotion(float windowXMotion,float windowYMotion,
    float *logicalXMotion,float *logicalYMotion)
{
    if (!logicalXMotion || !logicalYMotion) MwSdlFail("Convert mouse motion to EGA coordinates");
    const SDL_FPoint motion=MwSdlMotionToFrame(Picture("Convert mouse motion to EGA coordinates"),{windowXMotion,windowYMotion});
    *logicalXMotion=motion.x;*logicalYMotion=motion.y;
}
/* A texture of 320x200 pixels on the device, nearest-neighbour. */
static SDL_Texture *MakeTexture(const uint32_t *pixels)
{
    SDL_Texture *made=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,EGA_COLUMNS,EGA_ROWS);
    if (made && SDL_SetTextureScaleMode(made,SDL_SCALEMODE_NEAREST) && SDL_SetTextureBlendMode(made,SDL_BLENDMODE_NONE)
        && SDL_UpdateTexture(made,NULL,pixels,EGA_COLUMNS*sizeof(uint32_t))) return made;
    if (made) SDL_DestroyTexture(made); /* Not of NULL: that is an error of its own, in place of the one above. */
    return NULL;
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
    enum { WINDOW_WIDTH=EGA_COLUMNS*3,WINDOW_HEIGHT=EGA_ROWS*3 };
    window=SDL_CreateWindow("MechWarrior 1 Rebuild",WINDOW_WIDTH,WINDOW_HEIGHT,SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIGH_PIXEL_DENSITY|hidden);
    if (!window) return false;
    /* Held to the frame's shape while it is resized. A system that can't
     * leaves bars at two of the sides. */
    const float shape=(float)EGA_COLUMNS/(float)EGA_ROWS;
    SDL_SetWindowAspectRatio(window,shape,shape);
    SDL_SetWindowMinimumSize(window,EGA_COLUMNS,EGA_ROWS);
    picture=MwSdlPicture{{0,0,WINDOW_WIDTH,WINDOW_HEIGHT},{1,1}};
    renderer=SDL_CreateRenderer(window,NULL);
    if (!renderer) return false;
    shownSurface=SDL_CreateSurfaceFrom(EGA_COLUMNS,EGA_ROWS,SDL_PIXELFORMAT_ARGB8888,shownPixels,EGA_COLUMNS*sizeof(uint32_t));
    noticeRenderer=shownSurface?SDL_CreateSoftwareRenderer(shownSurface):NULL;
    if (!noticeRenderer) return false;
    /* A device that can't at the start is a refused start. */
    texture=MakeTexture(shownPixels);
    if (!texture) return false;
    GameCanvas().setPresenter([](const Canvas &canvas,int page) {PresentCanvasPage(canvas,page,true);});
    haveFrame=presenting=deviceLost=drawErrorSaid=false;
    return true;
}
/* A small boxed line at the top right of the 320x200 frame (SDL's 8x8 font). */
static void DrawNotice(const char *text)
{
    float width=(float)(strlen(text)*SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE),x=(float)EGA_COLUMNS-width-6.0f,y=4.0f;
    SDL_FRect box={x-3.0f,y-3.0f,width+6.0f,(float)SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE+6.0f};
    if (!SDL_SetRenderDrawColor(noticeRenderer,0,0,0,255) || !SDL_RenderFillRect(noticeRenderer,&box)
        || !SDL_SetRenderDrawColor(noticeRenderer,255,255,85,255) || !SDL_RenderDebugText(noticeRenderer,x,y,text)
        || !SDL_RenderPresent(noticeRenderer))
        MwSdlFail("Draw the speed notice");
}
/* shownPixels, and the notice if there is one, from rgbaPixels. */
static void PrepareShown(const char *notice)
{
    memcpy(shownPixels,rgbaPixels,sizeof(shownPixels));
    if (notice) DrawNotice(notice);
}
/* shownPixels to the window. A frame that can't be drawn (a lost device, a
 * refused texture) is skipped, and the next starts again from nothing: the
 * game goes on, and the error is said once. False if it was skipped. */
static bool Show(bool upload)
{
    if (deviceLost) return true;
    bool shown=true;
    if (!texture) shown=(texture=MakeTexture(shownPixels))!=NULL;
    else if (upload) shown=SDL_UpdateTexture(texture,NULL,shownPixels,EGA_COLUMNS*sizeof(uint32_t));
    shown=shown && SDL_SetRenderTarget(renderer,NULL) && MwSdlDrawPicture(renderer,texture) && SDL_RenderPresent(renderer);
    if (shown) return true;
    if (!drawErrorSaid) fprintf(stderr,"MechWarrior: can't draw: %s\n",SDL_GetError());
    drawErrorSaid=true;
    SDL_DestroyTexture(texture);texture=NULL;
    return false;
}
/* What SDL says about the device. After a reset every texture is gone; the
 * last frame is shown again. A device that is lost for good is not drawn on,
 * and the game goes on, so that the player can save. */
bool MwSdlHandleRenderEvent(const SDL_Event &event)
{
    if (event.type==SDL_EVENT_RENDER_DEVICE_RESET)
    {
        fprintf(stderr,"MechWarrior: the graphics device was reset; the picture is made again.\n");
        SDL_DestroyTexture(texture);texture=NULL;
        deviceLost=false;
    }
    else if (event.type==SDL_EVENT_RENDER_DEVICE_LOST)
    {
        if (!deviceLost)
            fprintf(stderr,"MechWarrior: the graphics device is lost; nothing more can be drawn. The game goes on and can be saved.\n");
        SDL_DestroyTexture(texture);texture=NULL;
        deviceLost=true;
    }
    else if (event.type!=SDL_EVENT_RENDER_TARGETS_RESET) return false;
    MwSdlRepeatPresent();
    return true;
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
        repeatPresentAt=now+(Show(false)?(uint64_t)HOST_REPEAT_IDLE_NS:(uint64_t)HOST_REPEAT_SETTLE_NS);
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
    PrepareShown(showNotice?notice:NULL);
    Show(true);
    noticeShown=showNotice;
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
/* The frame without the notice, drawn by the device at 3 pixels to each of
 * the game's, whatever the window or the display. NULL if it can't. */
static SDL_Surface *ReadBack(void)
{
    if (deviceLost) return NULL;
    SDL_Texture *page=MakeTexture(rgbaPixels);
    SDL_Texture *target=page?SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_TARGET,EGA_COLUMNS*3,EGA_ROWS*3):NULL;
    SDL_Surface *surface=target && SDL_SetRenderTarget(renderer,target) && SDL_RenderTexture(renderer,page,NULL,NULL)
        ?SDL_RenderReadPixels(renderer,NULL):NULL;
    if (target && !SDL_SetRenderTarget(renderer,NULL)) {SDL_DestroySurface(surface);surface=NULL;}
    SDL_DestroyTexture(target);SDL_DestroyTexture(page);
    return surface;
}
bool MwSdlCapture(const char *stem)
{
    if (!HaveFrame() || !stem || strpbrk(stem,"/\\:")) return false;
    char path[4096];
    if (snprintf(path,sizeof(path),"%s/%s.bmp",captureDirectory,stem)>=(int)sizeof(path)) return false;
    /* Reserve exclusively: never overwrite an earlier observation. */
    FILE *file=fopen(path,"wbx");if (!file) return false;fclose(file);
    SDL_Surface *surface=ReadBack();
    bool saved=surface && SDL_SaveBMP(surface,path);
    SDL_DestroySurface(surface);
    if (!saved) {remove(path);return false;} /* The name just reserved, not an earlier capture. */
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
    SDL_DestroyRenderer(noticeRenderer);SDL_DestroySurface(shownSurface);
    SDL_DestroyTexture(texture);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);
    noticeRenderer=NULL;shownSurface=NULL;texture=NULL;renderer=NULL;window=NULL;
}
