#include "SDLPicture.h"
#include "mechwarrior/ega_campaign.h"
#include <algorithm>
#include <cmath>

static constexpr int COLUMNS=EGA_COLUMNS,ROWS=EGA_ROWS;

SDL_Rect MwSdlPictureRect(int width,int height)
{
    int w=std::min(width,(int)std::lround((double)height*COLUMNS/ROWS));
    int h=std::min(height,(int)std::lround((double)width*ROWS/COLUMNS));
    /* A window held to 16:10 has a whole number of points at each side, each
     * up to three pixels: what that leaves is filled. */
    enum { ROUNDING_PIXELS=3 };
    if (width-w<=ROUNDING_PIXELS) w=width;
    if (height-h<=ROUNDING_PIXELS) h=height;
    return SDL_Rect{(width-w)/2,(height-h)/2,w,h};
}
bool MwSdlReadPicture(SDL_Window *window,SDL_Renderer *renderer,MwSdlPicture *picture)
{
    int points[2],pixels[2];
    if (!SDL_GetWindowSize(window,&points[0],&points[1]) || !SDL_GetRenderOutputSize(renderer,&pixels[0],&pixels[1])) return false;
    if (points[0]<=0 || points[1]<=0 || pixels[0]<=0 || pixels[1]<=0) return true;
    const SDL_Rect rect=MwSdlPictureRect(pixels[0],pixels[1]);
    if (rect.w<=0 || rect.h<=0) return true;
    *picture=MwSdlPicture{rect,{(float)pixels[0]/(float)points[0],(float)pixels[1]/(float)points[1]}};
    return true;
}
bool MwSdlDrawPicture(SDL_Renderer *renderer,SDL_Texture *frame)
{
    int width,height;
    if (!SDL_GetCurrentRenderOutputSize(renderer,&width,&height) || !SDL_SetRenderDrawColor(renderer,0,0,0,255)
        || !SDL_RenderClear(renderer)) return false;
    const SDL_Rect rect=MwSdlPictureRect(width,height);
    if (rect.w<=0 || rect.h<=0) return true;
    const SDL_FRect place={(float)rect.x,(float)rect.y,(float)rect.w,(float)rect.h};
    return SDL_RenderTexture(renderer,frame,NULL,&place);
}
SDL_FPoint MwSdlWindowToFrame(const MwSdlPicture &picture,SDL_FPoint point)
{
    return {(point.x*picture.pixelsPerPoint.x-(float)picture.rect.x)*COLUMNS/(float)picture.rect.w,
        (point.y*picture.pixelsPerPoint.y-(float)picture.rect.y)*ROWS/(float)picture.rect.h};
}
SDL_FPoint MwSdlFrameToWindow(const MwSdlPicture &picture,SDL_FPoint point)
{
    return {((float)picture.rect.x+point.x*(float)picture.rect.w/COLUMNS)/picture.pixelsPerPoint.x,
        ((float)picture.rect.y+point.y*(float)picture.rect.h/ROWS)/picture.pixelsPerPoint.y};
}
SDL_FPoint MwSdlMotionToFrame(const MwSdlPicture &picture,SDL_FPoint motion)
{
    return {motion.x*picture.pixelsPerPoint.x*COLUMNS/(float)picture.rect.w,
        motion.y*picture.pixelsPerPoint.y*ROWS/(float)picture.rect.h};
}
SDL_Point MwSdlFramePixel(const MwSdlPicture &picture,SDL_FPoint point)
{
    const SDL_FPoint frame=MwSdlWindowToFrame(picture,point);
    return {(int)std::clamp(std::floor(frame.x),0.0f,(float)(COLUMNS-1)),
        (int)std::clamp(std::floor(frame.y),0.0f,(float)(ROWS-1))};
}
SDL_FPoint MwSdlPointInFramePixel(const MwSdlPicture &picture,uint16_t x,uint16_t y)
{
    const SDL_FPoint middle=MwSdlFrameToWindow(picture,{(float)x+0.5f,(float)y+0.5f});
    const SDL_FPoint below={std::floor(middle.x),std::floor(middle.y)};
    const SDL_Point read=MwSdlFramePixel(picture,below);
    const SDL_FPoint above={std::ceil(middle.x),std::ceil(middle.y)};
    const SDL_Point readAbove=MwSdlFramePixel(picture,above);
    return {read.x==x?below.x:readAbove.x==x?above.x:middle.x,
        read.y==y?below.y:readAbove.y==y?above.y:middle.y};
}
