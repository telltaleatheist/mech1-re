#include "SDLMenu.h"
#include "SDLBackend.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory>
#include <utility>

/* The host's half of the kit's menu (SDLMenu.h). The menu is a picture the
 * kit draws (RGBA at the window's pixel size), put over the game's frame just
 * before each present. While it is open the game's clock stands still
 * (MwSdlPauseClock): the campaign and combat loops go on polling, so the menu
 * gets its events, and MwSdlMenuPump shows its frames, the game drawing none. */
namespace {
MwSdlMenuRecipe recipe;
std::unique_ptr<rekit::sdl::GameMenu> menu;
SDL_Texture *picture;             /* the kit's last picture on the device */
uint64_t pictureSerial;           /* its serial; 0 none */
int pictureWidth,pictureHeight;
SDL_Renderer *drawingOn;          /* the renderer of the present being drawn */
bool drew;                        /* the last present had the menu or the hint over it */
bool inEvent;                     /* inside the kit's event(): no frame() from there */
uint64_t lastShown;               /* the last of the menu's own frames (MwSdlMenuPump) */
enum { MENU_FRAME_NS=16000000 };  /* about 60 frames a second while it shows */
/* The way out the menu asked for: after the event loop, never from inside the
 * kit (its object is destroyed on the way out). */
bool exitAsked,setupAfter;

void DropPicture(void)
{
    if (picture) SDL_DestroyTexture(picture);
    picture=NULL;pictureSerial=0;pictureWidth=pictureHeight=0;
}
/* The kit's picture over the whole output, uploaded only when it changed. */
void Draw(const rekit::sdl::MenuImage &image)
{
    if (!drawingOn || !image.pixels || image.w<=0 || image.h<=0) return;
    if (!picture || image.w!=pictureWidth || image.h!=pictureHeight || SDL_GetRendererFromTexture(picture)!=drawingOn)
    {
        DropPicture();
        picture=SDL_CreateTexture(drawingOn,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STREAMING,image.w,image.h);
        if (!picture || !SDL_SetTextureBlendMode(picture,SDL_BLENDMODE_BLEND)
            || !SDL_SetTextureScaleMode(picture,SDL_SCALEMODE_NEAREST)) {DropPicture();return;}
        pictureWidth=image.w;pictureHeight=image.h;
    }
    if (image.serial!=pictureSerial || !pictureSerial)
    {
        if (!SDL_UpdateTexture(picture,NULL,image.pixels,image.pitch)) {DropPicture();return;}
        pictureSerial=image.serial;
    }
    if (SDL_RenderTexture(drawingOn,picture,NULL,NULL)) drew=true;
}
/* The program closes: the setup started first when CHANGE asked for it (in
 * its own window, as the kit's relaunch_for_setup does), then the host shut
 * down as at the end of a play, and the log flushed. */
[[noreturn]] void ExitNow(void)
{
    printf("menu: the game closes%s\n",setupAfter?" for its setup":"");
    fflush(stdout);
    MwSdlShutdown();
    fflush(stdout);fflush(stderr);
    exit(0);
}
}  // namespace

void MwSdlSetMenuRecipe(MwSdlMenuRecipe given) {recipe=std::move(given);}

void MwSdlStartMenu(const char *gameDirectory)
{
    if (menu || !recipe.hooks || !MwSdlVideoWindow()) return;
    rekit::sdl::MenuHooks hooks=recipe.hooks(gameDirectory?gameDirectory:"");
    hooks.window=MwSdlVideoWindow();
    hooks.draw=Draw;
    hooks.deliver=[](const SDL_Event &event) {MwSdlHandleEvent(event);};
    hooks.pause=MwSdlPauseClock;
    hooks.resume=MwSdlResumeClock;
    hooks.quit=[] {exitAsked=true;};
    hooks.restart_for_setup=[] {
        /* Started now, while this one is still up: if it can't be, the game
         * goes on rather than leave the player with nothing. */
        if (rekit::sdl::relaunch_for_setup({"--setup"})) exitAsked=setupAfter=true;
        else fprintf(stderr,"menu: the setup could not be started; the game goes on\n");
    };
    exitAsked=setupAfter=drew=inEvent=false;lastShown=0;
    menu=std::make_unique<rekit::sdl::GameMenu>(std::move(hooks),recipe.theme?recipe.theme():rekit::sdl::Theme{});
}
void MwSdlStopMenu(void)
{
    menu.reset();
    DropPicture();
    if (MwSdlClockPaused()) MwSdlResumeClock();
}
void MwSdlMenuEvent(const SDL_Event &event)
{
    if (!menu) {MwSdlHandleEvent(event);return;}
    inEvent=true;
    menu->event(event);
    inEvent=false;
}
void MwSdlMenuPump(void)
{
    if (exitAsked) ExitNow();
    if (!menu) return;
    /* Open, or a picture left on the screen to take away (the menu just
     * closed, the hint ending): shown again, at most every MENU_FRAME_NS. */
    if (!menu->is_open() && !drew) return;
    uint64_t now=SDL_GetTicksNS();
    if (now-lastShown<MENU_FRAME_NS) return;
    lastShown=now;
    MwSdlShowAgain();
}
bool MwSdlMenuOverlay(SDL_Renderer *renderer)
{
    if (!menu) return true;
    if (inEvent)
    {
        /* A present from inside the kit's event() (a window exposed): its
         * last picture as it was; frame() runs at the next present. */
        if (drew && picture) SDL_RenderTexture(renderer,picture,NULL,NULL);
        return true;
    }
    drew=false;drawingOn=renderer;
    menu->frame();
    drawingOn=NULL;
    return true;
}
void MwSdlMenuDeviceReset(void) {DropPicture();}
