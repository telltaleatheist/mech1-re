# The window

The game has one screen, 320x200. It is shown with square pixels, 16:10, in a window of any size
(`src/SDL/SDLPicture.cpp`, `src/SDL/SDLBackend_Video.cpp`). The method is Inception Rebuild's.

## The picture

- The picture is as large as the window allows at 16:10, in the middle. Up to 3 pixels of rounding are filled, not
  left as a bar.
- The frame is put together at its own size, with the speed notice on it, and is then scaled once to the window,
  nearest-neighbour. Nothing is smoothed, and the notice's pixels are as large as the game's.
- At a size that isn't a whole multiple of 320x200, some of the game's pixels are one window pixel wider than others.
  The window has the display's own pixels (`SDL_WINDOW_HIGH_PIXEL_DENSITY`), so on a dense display that is hard to see.
- The mouse is in the window's points, which on a dense display are more than one pixel each. It is converted to the
  game's pixels and back. A position the game sets is put on a whole point inside its pixel, so the same one is read
  back. A point on a bar is the nearest pixel of the picture.
- A capture (F12, `@snap`) is the frame at 3 pixels to each of the game's, 960x600, without the speed notice, whatever
  the window or the display.

## The window's shape

The window is held to 16:10 while it is resized (`SDL_SetWindowAspectRatio`) and is at least 320x200. Where the system
gives it another shape, black bars remain at two sides, and the picture still reaches the other two. That is so in
full screen on a 4:3 or a 16:9 display, and it can be in a maximised or tiled window. From SDL's source, and not tried
on a screen:

| System | While resized | Maximised, tiled, full screen |
| --- | --- | --- |
| macOS | AppKit holds it | AppKit decides whether the shape or the space wins |
| Windows | held | not held |
| X11 | a hint, for the window manager to follow or not | the same |
| Wayland | held, but the compositor's maximised and tiled sizes are limits | as while resized |

The setup screens are the setup kit's own window and keep their own shape the same way: letterboxed, at any scale.

## How it is checked

The `picture` test (`tests/picture_test.cpp`) makes no window on the screen.

- The picture's place and the drawing, with SDL's software renderer on a surface, at 23 sizes: every window pixel must
  be the right pixel of the frame.
- The mouse both ways, for every pixel of the frame, at those sizes and at 1 to 3 window pixels a point.
- The host itself on SDL's dummy video driver, at six sizes: what it presents, with and without the speed notice, its
  captures, and its mouse.

A real window, and a real dense display, are for a person to look at
([VISUAL_ACCEPTANCE.md](VISUAL_ACCEPTANCE.md#the-window)).
