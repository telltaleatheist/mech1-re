#pragma once
/* Where the 320x200 frame is in the window (docs/WINDOW.md). The method is
 * Inception Rebuild's (its src/platform/renderer.cpp). */
#include <SDL3/SDL.h>
#include <stdint.h>

/* The frame's place in the window's pixels, and how many pixels a window
 * point has: the mouse is in points, what is drawn is in pixels. */
struct MwSdlPicture
{
    SDL_Rect rect;
    SDL_FPoint pixelsPerPoint;
};
/* The largest 16:10 picture in width x height pixels, in the middle. Up to 3
 * pixels of rounding are filled, not left as a bar. */
SDL_Rect MwSdlPictureRect(int width,int height);
/* The window's picture as it now is. Left as it was for a window with no
 * pixels (minimised). */
bool MwSdlReadPicture(SDL_Window *window,SDL_Renderer *renderer,MwSdlPicture *picture);
/* The frame on the renderer's output, scaled once, nearest-neighbour. */
bool MwSdlDrawPicture(SDL_Renderer *renderer,SDL_Texture *frame);

/* The mouse both ways: window points and the frame's pixels. */
SDL_FPoint MwSdlWindowToFrame(const MwSdlPicture &picture,SDL_FPoint point);
SDL_FPoint MwSdlFrameToWindow(const MwSdlPicture &picture,SDL_FPoint point);
SDL_FPoint MwSdlMotionToFrame(const MwSdlPicture &picture,SDL_FPoint motion);
/* The frame's pixel a window point is in; the nearest one, outside it. */
SDL_Point MwSdlFramePixel(const MwSdlPicture &picture,SDL_FPoint point);
/* A window point inside a frame pixel: a whole one if the pixel has any, as
 * some systems keep no fraction, and the same pixel is then read back. */
SDL_FPoint MwSdlPointInFramePixel(const MwSdlPicture &picture,uint16_t x,uint16_t y);
