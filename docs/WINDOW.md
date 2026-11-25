# The window

The game has one screen, 320x200. It is shown with square pixels, 16:10, in a window of any size
(`src/SDL/SDLPicture.cpp`, `src/SDL/SDLBackend_Video.cpp`). The method is Inception Rebuild's.

## The picture

- The picture is as large as the window allows at 16:10, in the middle. Up to 3 pixels of rounding are filled, not
  left as a bar.
- The frame is put together in memory at its own size, with the speed notice on it, and is then scaled once to the
  window, nearest-neighbour. Nothing is smoothed, and the notice's pixels are as large as the game's.
- At a size that isn't a whole multiple of 320x200, some of the game's pixels are one window pixel wider than others.
  The window has the display's own pixels (`SDL_WINDOW_HIGH_PIXEL_DENSITY`), so on a dense display that is hard to see.
- The mouse is in the window's points, which on a dense display are more than one pixel each. It is converted to the
  game's pixels and back. A position the game sets is put on a whole point inside its pixel, so the same one is read
  back. A point on a bar is the nearest pixel of the picture.
- A capture (F12, `@snap`) is the frame at 3 pixels to each of the game's, 960x600, without the speed notice, whatever
  the window or the display.

## When the device is reset or lost

A driver update, or a graphics processor taken away, can take every texture with it. SDL says so in events after a
present that failed, and the handling is Inception Rebuild's.

- `SDL_EVENT_RENDER_DEVICE_RESET`: the texture is made again and the last frame is uploaded and shown, with the notice.
  There is only the one texture. The notice is drawn into it in memory and not on the device, because SDL keeps its
  font's texture itself and doesn't make it again after a reset.
- `SDL_EVENT_RENDER_TARGETS_RESET`: the last frame is shown again. The game's own frame isn't in a target.
- `SDL_EVENT_RENDER_DEVICE_LOST`: the device can't be made again. It is said once in the log and nothing more is drawn.
  The game goes on, blind, with its keys and the last picture's mouse, so it can be saved.
- A frame that can't be drawn is skipped, the error is said once in the log, and the next frame starts again from
  nothing (50 ms later, if the game draws none before). The game's frames and the traces go on.
- A capture that can't be drawn fails, and leaves no file. A device that can't be drawn on at the start is a refused
  start, as before.

From SDL's source, Direct3D 11 and 12 and Vulkan send the device's events; Direct3D 9 sends the targets'. Nobody has
seen any of them on a real graphics processor. The test sends them.

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
- The device's events through the host's event loop, a texture refused and then there again, and a device that fails
  everything: what is shown, the captures, the mouse, and what the log says.

A real window, and a real dense display, are for a person to look at
([VISUAL_ACCEPTANCE.md](VISUAL_ACCEPTANCE.md#the-window)).
