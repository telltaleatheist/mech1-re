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
static SDL_Texture *texture; /* The canvas page. */
static SDL_Texture *frame;   /* It and the speed notice, before they are scaled to the window. */
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
static Canvas::Palette lastRgb; /* The canvas palette the presented frame was drawn with. */
static int lastPage;
static unsigned long lastSetUps; /* The canvas set-up the presented frame belongs to. */
static char captureDirectory[4096];
static bool haveFrame,presenting;
bool mwSdlCombatActive;
void (*mwSdlFrameObserver)(void);

/* A frame of the canvas as it is now set up: a mode set starts a new screen. */
static bool HaveFrame(void) {return haveFrame && lastSetUps==GameCanvas().setUps();}
/* The picture as the window now is (docs/WINDOW.md). */
static const MwSdlPicture &Picture(const char *what)
{
    if (!renderer || !window || !MwSdlReadPicture(window,renderer,&picture)) MwSdlFail(what);
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
/* The whole frame as a renderer target, for the window or for a capture. */
static SDL_Texture *MakeTarget(int scale)
{
    SDL_Texture *target=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_TARGET,EGA_COLUMNS*scale,EGA_ROWS*scale);
    if (target && SDL_SetTextureScaleMode(target,SDL_SCALEMODE_NEAREST) && SDL_SetTextureBlendMode(target,SDL_BLENDMODE_NONE)) return target;
    SDL_DestroyTexture(target);
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
    texture=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,EGA_COLUMNS,EGA_ROWS);
    if (!texture || !SDL_SetTextureScaleMode(texture,SDL_SCALEMODE_NEAREST)) return false;
    frame=MakeTarget(1);
    if (!frame) return false;
    GameCanvas().setPresenter([](const Canvas &canvas,int page) {PresentCanvasPage(canvas,page,true);});
    haveFrame=presenting=false;
    return true;
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
/* The frame to the window. It is put together at its own size and scaled
 * once, so the notice's pixels are the picture's at any size. */
static void Render(const char *notice)
{
    if (!SDL_SetRenderTarget(renderer,frame) || !SDL_RenderTexture(renderer,texture,NULL,NULL)) MwSdlFail("Render EGA texture");
    if (notice) DrawNotice(notice);
    if (!SDL_SetRenderTarget(renderer,NULL) || !MwSdlDrawPicture(renderer,frame)) MwSdlFail("Render EGA texture");
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
        Render(showNotice?notice:NULL);
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
    Render(showNotice?notice:NULL);
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
    /* 3 pixels to each of the frame's, whatever the window or the display. */
    SDL_Texture *target=MakeTarget(3);
    if (!target) return false;
    SDL_Surface *surface=SDL_SetRenderTarget(renderer,target) && SDL_RenderTexture(renderer,texture,NULL,NULL)
        ?SDL_RenderReadPixels(renderer,NULL):NULL;
    bool saved=SDL_SetRenderTarget(renderer,NULL);
    SDL_DestroyTexture(target);
    if (!surface) return false;
    if (!SDL_SaveBMP(surface,path)) saved=false;
    SDL_DestroySurface(surface);
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
    SDL_DestroyTexture(frame);SDL_DestroyTexture(texture);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);
    frame=NULL;texture=NULL;renderer=NULL;window=NULL;
}
