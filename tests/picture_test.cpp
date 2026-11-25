// The frame in the window (docs/WINDOW.md): the picture's place, the drawing,
// and the mouse both ways. The drawing is on SDL's software renderer on a
// surface, and the host's own part runs on SDL's dummy video driver, so no
// window is ever made on the screen. Plain checks, no framework.
#include "SDLBackend.h"
#include "SDLPicture.h"
#include "Canvas.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace {

constexpr int W = 320, H = 200;
int failures = 0;
void Check(bool ok, const std::string &what) {
  if (!ok) {
    std::printf("FAIL %s\n", what.c_str());
    ++failures;
  }
}
std::string Size(int w, int h) { return std::to_string(w) + "x" + std::to_string(h); }

// Window sizes in pixels: whole scales, sizes between them, sizes a point or
// two off 16:10, other shapes, and one smaller than the frame.
const SDL_Point kSizes[] = {{320, 200},   {960, 600},   {1920, 1200}, {640, 400},   {321, 201},   {480, 300},
                            {1000, 625},  {1001, 625},  {999, 625},   {1000, 627},  {1234, 771},  {1884, 1098},
                            {3768, 2196}, {2560, 1440}, {1920, 1080}, {1024, 768},  {800, 1200},  {2000, 320},
                            {1440, 900},  {333, 208},   {160, 100},   {3456, 2160}, {701, 438}};

void Rect() {
  struct Case { int w, h; SDL_Rect want; };
  const Case cases[] = {
      {960, 600, {0, 0, 960, 600}},       {1000, 625, {0, 0, 1000, 625}},  {1001, 625, {0, 0, 1001, 625}},
      {1003, 625, {0, 0, 1003, 625}},     {1004, 625, {2, 0, 1000, 625}},  {1000, 628, {0, 0, 1000, 628}},
      {1000, 629, {0, 2, 1000, 625}},     {1920, 1080, {96, 0, 1728, 1080}}, {1024, 768, {0, 64, 1024, 640}},
      {320, 200, {0, 0, 320, 200}},       {800, 1200, {0, 350, 800, 500}}, {0, 0, {0, 0, 0, 0}},
  };
  for (const Case &c : cases) {
    const SDL_Rect got = MwSdlPictureRect(c.w, c.h);
    Check(SDL_RectsEqual(&got, &c.want), "picture in " + Size(c.w, c.h));
  }
  for (const SDL_Point &s : kSizes) {
    const SDL_Rect r = MwSdlPictureRect(s.x, s.y);
    Check(r.w == s.x || r.h == s.y, "the picture reaches two sides of " + Size(s.x, s.y));
    Check(r.x >= 0 && r.y >= 0 && r.x + r.w <= s.x && r.y + r.h <= s.y, "the picture is inside " + Size(s.x, s.y));
  }
  // Every 16:10 size whose points were rounded has no bar, at 1 or 2 pixels a point (3 can leave 5 pixels).
  bool filled = true;
  for (int w = W; w <= 2000; ++w)
    for (int density = 1; density <= 2; ++density)
      for (int round = 0; round < 2; ++round) {
        const int h = w * H / W + round;
        if (round && w * H % W == 0) continue;
        const SDL_Rect r = MwSdlPictureRect(w * density, h * density);
        filled = filled && r.w == w * density && r.h == h * density;
      }
  Check(filled, "a window held to 16:10 has no bar");
}

Uint32 Pattern(int x, int y) { return 0xFF000000u | Uint32(x & 255) << 16 | Uint32(y) << 8 | Uint32(x >> 8) | 0x40u; }

// Which frame pixel (either, at an edge) the middle of output pixel `at` is in:
// to a twentieth of an output pixel, and the software renderer's step, cut to
// 1/65536 of a frame pixel at each output pixel.
double Slack(int length, int count) { return 0.05 * count / length + length / 65536.0; }
bool Holds(int at, int start, int length, int count, int got) {
  const double unit = (at - start + 0.5) * count / length, slack = Slack(length, count);
  return got >= int(std::floor(unit - slack)) && got <= int(std::floor(unit + slack));
}

void Draw() {
  std::vector<Uint32> pixels(size_t(W) * H);
  for (int y = 0; y < H; ++y)
    for (int x = 0; x < W; ++x) pixels[size_t(y) * W + x] = Pattern(x, y);
  for (const SDL_Point &s : kSizes) {
    SDL_Surface *out = SDL_CreateSurface(s.x, s.y, SDL_PIXELFORMAT_ARGB8888);
    SDL_Renderer *renderer = out ? SDL_CreateSoftwareRenderer(out) : nullptr;
    SDL_Texture *frame =
        renderer ? SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, W, H) : nullptr;
    const std::string name = " at " + Size(s.x, s.y);
    if (!frame || !SDL_SetTextureScaleMode(frame, SDL_SCALEMODE_NEAREST) ||
        !SDL_UpdateTexture(frame, nullptr, pixels.data(), W * 4) ||
        !SDL_FillSurfaceRect(out, nullptr, 0xFFFF00FFu) || !MwSdlDrawPicture(renderer, frame) ||
        !SDL_RenderPresent(renderer)) {
      Check(false, "draw" + name + ": " + SDL_GetError());
    } else {
      const SDL_Rect r = MwSdlPictureRect(s.x, s.y);
      bool inside = true, bars = true;
      for (int y = 0; y < s.y; ++y)
        for (int x = 0; x < s.x; ++x) {
          const Uint32 p = reinterpret_cast<const Uint32 *>(static_cast<const Uint8 *>(out->pixels) + y * out->pitch)[x];
          if (x < r.x || y < r.y || x >= r.x + r.w || y >= r.y + r.h) {
            bars = bars && p == 0xFF000000u;
            continue;
          }
          const int fx = int(p >> 16 & 255) | int(p & 1) << 8, fy = int(p >> 8 & 255);
          inside = inside && (p & 0xFE) == 0x40 && Holds(x, r.x, r.w, W, fx) && Holds(y, r.y, r.h, H, fy);
        }
      Check(inside, "every pixel is the frame's" + name);
      Check(bars, "what is left is black" + name);
    }
    SDL_DestroyTexture(frame);
    SDL_DestroyRenderer(renderer);
    SDL_DestroySurface(out);
  }
}

void Mouse() {
  for (const float density : {1.0f, 1.25f, 1.5f, 2.0f, 3.0f})
    for (const SDL_Point &s : kSizes) {
      const MwSdlPicture p{MwSdlPictureRect(s.x, s.y), {density, density}};
      const float scale = float(p.rect.w) / W / density, scaleY = float(p.rect.h) / H / density;
      bool same = true, whole = true, inverse = true;
      for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
          const SDL_FPoint at = MwSdlPointInFramePixel(p, uint16_t(x), uint16_t(y));
          const SDL_Point back = MwSdlFramePixel(p, at);
          same = same && back.x == x && back.y == y;
          // 1.01: a pixel of just one point can lie between two of them by rounding.
          if (scale >= 1.01f) whole = whole && at.x == std::floor(at.x);
          if (scaleY >= 1.01f) whole = whole && at.y == std::floor(at.y);
          const SDL_FPoint f = MwSdlWindowToFrame(p, MwSdlFrameToWindow(p, {float(x), float(y)}));
          inverse = inverse && std::fabs(f.x - x) < 0.01f && std::fabs(f.y - y) < 0.01f;
        }
      const std::string name = " at " + Size(s.x, s.y) + ", " + std::to_string(density) + " pixels a point";
      Check(same, "a position set is read back" + name);
      Check(whole, "a position set is a whole point" + name);
      Check(inverse, "window and frame are inverses" + name);
      // The corners, and the bars and what is outside the window.
      const SDL_FPoint topLeft{float(p.rect.x) / density, float(p.rect.y) / density};
      const SDL_FPoint size{float(p.rect.w) / density, float(p.rect.h) / density};
      const SDL_Point first = MwSdlFramePixel(p, {topLeft.x + size.x / 1000, topLeft.y + size.y / 1000});
      const SDL_Point last = MwSdlFramePixel(p, {topLeft.x + size.x * 0.999f, topLeft.y + size.y * 0.999f});
      Check(first.x == 0 && first.y == 0 && last.x == W - 1 && last.y == H - 1, "the corners" + name);
      const SDL_Point before = MwSdlFramePixel(p, {-5000, -5000}), after = MwSdlFramePixel(p, {9000, 9000});
      Check(before.x == 0 && before.y == 0 && after.x == W - 1 && after.y == H - 1, "outside is the nearest pixel" + name);
      const SDL_FPoint across = MwSdlMotionToFrame(p, size);
      Check(std::fabs(across.x - W) < 0.01f && std::fabs(across.y - H) < 0.01f, "a motion across the picture" + name);
    }
}

uint8_t CanvasColour(int x, int y) { return uint8_t((x * 7 + y * 3 + x / 16 + y / 16) & 15); }
int presents;
void Presented() { ++presents; }

std::vector<uint8_t> Contents(const std::filesystem::path &path) {
  std::vector<uint8_t> bytes;
  if (FILE *f = std::fopen(path.string().c_str(), "rb")) {
    int c;
    while ((c = std::fgetc(f)) != EOF) bytes.push_back(uint8_t(c));
    std::fclose(f);
  }
  return bytes;
}

// The real host on the dummy driver: its presents at several window sizes,
// with and without the speed notice, its captures, and its mouse.
void Host(const std::filesystem::path &work) {
  std::error_code ignored;
  std::filesystem::remove_all(work, ignored);
  std::filesystem::create_directories(work);
  if (!MwSdlInitializeVideo(work.string().c_str())) {
    Check(false, std::string("start the host's video: ") + SDL_GetError());
    return;
  }
  int count = 0;
  SDL_Window **windows = SDL_GetWindows(&count);
  SDL_Window *window = count == 1 ? windows[0] : nullptr;
  SDL_free(windows);
  Check(window != nullptr, "the host has one window");
  if (!window) return;
  SDL_Renderer *renderer = SDL_GetRenderer(window);
  float least = 0, most = 0;
  int leastW = 0, leastH = 0;
  SDL_GetWindowAspectRatio(window, &least, &most);
  SDL_GetWindowMinimumSize(window, &leastW, &leastH);
  Check(least == 1.6f && most == 1.6f, "the window is held to 16:10");
  Check(leastW == W && leastH == H, "the window is at least 320x200");

  mw1o::video::Canvas &canvas = mw1o::video::GameCanvas();
  canvas.setUp();
  for (int y = 0; y < H; ++y)
    for (int x = 0; x < W; ++x) canvas.pixel(0, x, y, CanvasColour(x, y));
  SDL_SetWindowAspectRatio(window, 0, 0);  // other shapes too, as a full screen has
  mwSdlFrameObserver = Presented;
  std::vector<uint8_t> firstBmp, firstIndices;
  int capture = 0;
  for (const SDL_Point s : {SDL_Point{960, 600}, {1000, 625}, {701, 438}, {320, 200}, {1024, 768}, {1280, 720}}) {
    const std::string name = " at " + Size(s.x, s.y);
    SDL_SetWindowSize(window, s.x, s.y);
    SDL_PumpEvents();
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
    const SDL_Rect r = MwSdlPictureRect(s.x, s.y);
    for (const bool notice : {false, true}) {
      mwSdlCombatActive = notice;
      const int before = presents;
      MwSdlPresentVideo(true);
      Check(presents == before + 1, "presented once" + name);
      if (notice) MwSdlStepCombatSpeed(0);  // the speed as it is: nothing is saved
      char text[32];
      Check(!notice || MwSdlCombatSpeedNotice(text, sizeof text), "the notice is on" + name);
      SDL_Surface *read = SDL_RenderReadPixels(renderer, nullptr);
      SDL_Surface *shown = read ? SDL_ConvertSurface(read, SDL_PIXELFORMAT_ARGB8888) : nullptr;
      SDL_DestroySurface(read);
      if (!shown || shown->w != s.x || shown->h != s.y) {
        Check(false, "read the window" + name);
        SDL_DestroySurface(shown);
        continue;
      }
      // The frame as shown: each of its pixels from the middle of its place.
      auto at = [&](int x, int y) {
        return reinterpret_cast<const Uint32 *>(static_cast<const Uint8 *>(shown->pixels) + y * shown->pitch)[x] & 0xFFFFFFu;
      };
      std::vector<Uint32> frame(size_t(W) * H);
      for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
          frame[size_t(y) * W + x] = at(r.x + int((x + 0.5) * r.w / W), r.y + int((y + 0.5) * r.h / H));
      const int boxW = 8 * 8 + 6, boxX = W - 8 * 8 - 6 - 3, boxY = 1, boxH = 8 + 6;  // "SPEED 1x"
      bool game = true, box = true;
      int black = 0, yellow = 0;
      for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
          const Uint32 p = frame[size_t(y) * W + x];
          if (notice && x >= boxX && x < boxX + boxW && y >= boxY && y < boxY + boxH) {
            black += p == 0, yellow += p == 0xFFFF55u;
            box = box && (p == 0 || p == 0xFFFF55u);
            continue;
          }
          const mw1o::video::Rgb c = canvas.paletteEntry(CanvasColour(x, y));
          game = game && p == (Uint32(c.r) << 16 | Uint32(c.g) << 8 | c.b);
        }
      Check(game, "the window shows the canvas" + name + (notice ? " with the notice" : ""));
      if (notice) Check(box && black > 200 && yellow > 100, "the notice is in its box" + name);
      // Each window pixel belongs to the frame's: the notice's are no finer.
      bool exact = true;
      for (int y = 0; y < r.h; ++y)
        for (int x = 0; x < r.w; ++x) {
          const double fx = (x + 0.5) * W / r.w, fy = (y + 0.5) * H / r.h, slack = Slack(r.w, W);
          bool found = false;
          for (const double dx : {-slack, slack})
            for (const double dy : {-slack, slack}) {
              const int px = SDL_clamp(int(std::floor(fx + dx)), 0, W - 1), py = SDL_clamp(int(std::floor(fy + dy)), 0, H - 1);
              found = found || at(r.x + x, r.y + y) == frame[size_t(py) * W + px];
            }
          exact = exact && found;
        }
      Check(exact, "the window is the frame, scaled" + name + (notice ? " with the notice" : ""));
      SDL_DestroySurface(shown);

      const std::string stem = "c" + std::to_string(++capture);
      Check(MwSdlCapture(stem.c_str()), "capture" + name);
      const std::vector<uint8_t> bmp = Contents(work / (stem + ".bmp")), indices = Contents(work / (stem + ".indices"));
      SDL_Surface *saved = SDL_LoadBMP((work / (stem + ".bmp")).string().c_str());
      Check(saved && saved->w == W * 3 && saved->h == H * 3, "a capture is 960x600" + name);
      SDL_DestroySurface(saved);
      if (firstBmp.empty()) firstBmp = bmp, firstIndices = indices;
      Check(!bmp.empty() && bmp == firstBmp, "a capture doesn't depend on the window" + name);
      bool same = indices.size() == size_t(W) * H && indices == firstIndices;
      for (size_t i = 0; same && i < indices.size(); ++i) same = indices[i] == CanvasColour(int(i) % W, int(i) / W);
      Check(same, "its indices are the canvas's" + name);
    }
    mwSdlCombatActive = false;
    bool mouse = true;
    for (int y = 0; y < H; y += 7)
      for (int x = 0; x < W; x += 3) {
        uint16_t gotX = 0, gotY = 0;
        MwSdlSetMousePosition(uint16_t(x), uint16_t(y));
        MwSdlReadMousePosition(&gotX, &gotY);
        mouse = mouse && gotX == x && gotY == y;
      }
    Check(mouse, "the host's mouse reads where it was set" + name);
    float mx = 0, my = 0;
    MwSdlConvertMouseMotion(float(r.w), float(r.h), &mx, &my);
    Check(std::fabs(mx - W) < 0.01f && std::fabs(my - H) < 0.01f, "the host's motion" + name);
  }
  MwSdlShutdownVideo();
}

// The device reset, refusing textures and lost, through the host's own event
// loop. The software renderer keeps what it has, so the reset is only said,
// and what a lost device does is done: it refuses a texture, then everything.
struct Output {
  int w = 0, h = 0;
  std::vector<Uint32> pixels;
  friend bool operator==(const Output &a, const Output &b) { return a.w == b.w && a.h == b.h && a.pixels == b.pixels; }
};
Output Read(SDL_Renderer *renderer) {
  Output out;
  SDL_Surface *read = SDL_RenderReadPixels(renderer, nullptr);
  SDL_Surface *argb = read ? SDL_ConvertSurface(read, SDL_PIXELFORMAT_ARGB8888) : nullptr;
  SDL_DestroySurface(read);
  if (argb) {
    out.w = argb->w, out.h = argb->h;
    for (int y = 0; y < argb->h; ++y) {
      const Uint32 *row = reinterpret_cast<const Uint32 *>(static_cast<const Uint8 *>(argb->pixels) + y * argb->pitch);
      out.pixels.insert(out.pixels.end(), row, row + argb->w);
    }
    SDL_DestroySurface(argb);
  }
  return out;
}
void Blank(SDL_Renderer *renderer) {  // what is on the window, gone
  SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
  SDL_RenderClear(renderer);
  SDL_RenderPresent(renderer);
}
void Say(Uint32 type) {
  SDL_Event event{};
  event.type = type;
  Check(SDL_PushEvent(&event), "push an event");
  MwSdlPollEvents();
}
void Device(const std::filesystem::path &work) {
  std::error_code ignored;
  std::filesystem::remove_all(work, ignored);
  std::filesystem::create_directories(work);
  if (!MwSdlInitializeVideo(work.string().c_str())) {
    Check(false, std::string("start the host's video: ") + SDL_GetError());
    return;
  }
  SDL_Window **windows = SDL_GetWindows(nullptr);
  SDL_Window *window = windows ? windows[0] : nullptr;
  SDL_free(windows);
  SDL_Renderer *renderer = SDL_GetRenderer(window);
  Check(renderer != nullptr, "the host's renderer");
  if (!renderer) return;
  const auto largest = [&](int side) {
    SDL_SetNumberProperty(SDL_GetRendererProperties(renderer), SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER, side);
  };
  const auto capture = [&](const char *stem) { return MwSdlCapture(stem); };
  const auto there = [&](const char *name) { return std::filesystem::exists(work / name); };
  SDL_SetWindowSize(window, 1000, 625);
  MwSdlPollEvents();
  mw1o::video::Canvas &canvas = mw1o::video::GameCanvas();
  canvas.setUp();
  const auto paint = [&](int add) {
    for (int y = 0; y < H; ++y)
      for (int x = 0; x < W; ++x) canvas.pixel(0, x, y, uint8_t((CanvasColour(x, y) + add) & 15));
  };
  paint(0);
  mwSdlFrameObserver = Presented;
  mwSdlCombatActive = true;
  MwSdlStepCombatSpeed(0);
  MwSdlPresentVideo(true);
  const Output first = Read(renderer);
  Check(first.w == 1000 && first.h == 625, "read the window");

  // Reset, and targets reset: the same frame again, the notice too, from
  // the next pump, with nothing changed in the game.
  for (const Uint32 type : {SDL_EVENT_RENDER_DEVICE_RESET, SDL_EVENT_RENDER_TARGETS_RESET, SDL_EVENT_RENDER_DEVICE_RESET}) {
    Blank(renderer);
    Check(!(Read(renderer) == first), "the window is blank");
    const int before = presents;
    Say(type);
    MwSdlPresentVideo(false);
    Check(Read(renderer) == first, "the same frame after " + std::to_string(type));
    Check(presents == before, "which is no new frame of the game's");
  }
  Check(capture("reset"), "a capture after a reset");
  // A changed frame right after one, before the next pump.
  MwSdlStepCombatSpeed(0);  // the notice for its 1.5 s from here
  Say(SDL_EVENT_RENDER_DEVICE_RESET);
  paint(1);
  MwSdlPresentVideo(false);
  const Output second = Read(renderer);
  Check(!(second == first), "a changed frame after a reset");

  // The texture refused: the frames are skipped and the game's go on.
  Say(SDL_EVENT_RENDER_DEVICE_RESET);
  largest(100);
  Blank(renderer);
  const Output blank = Read(renderer);
  for (int i = 0; i < 3; ++i) {
    const int before = presents;
    MwSdlPresentVideo(true);
    Check(presents == before + 1 && Read(renderer) == blank, "a frame that can't be drawn is skipped");
  }
  Check(!capture("refused") && !there("refused.bmp") && !there("refused.indices"), "no capture, and no file, while refused");
  uint16_t x = 0, y = 0;
  MwSdlSetMousePosition(100, 50);
  MwSdlReadMousePosition(&x, &y);
  Check(x == 100 && y == 50, "the mouse while refused");
  SDL_Delay(60);  // the retry's 50 ms: refused again
  MwSdlPresentVideo(false);
  Check(Read(renderer) == blank, "and asked for again");
  largest(0);
  SDL_Delay(60);
  MwSdlPresentVideo(false);
  Check(Read(renderer) == second, "the frame is back, with nothing changed in the game");
  Check(capture("refused"), "a capture is there again, under the same name");

  // Nothing more can be drawn: first a device that fails everything, unsaid,
  // then one SDL says is lost.
  SDL_DestroyRenderer(renderer);
  for (int i = 0; i < 6; ++i) {
    if (i == 3) Say(SDL_EVENT_RENDER_DEVICE_LOST);
    paint(i);
    const int before = presents;
    MwSdlPresentVideo(false);
    MwSdlPresentVideo(true);
    Check(presents == before + 2, "the game's frames go on without a device");
    Check(!capture("lost") && !there("lost.bmp"), "no capture without a device");
    MwSdlSetMousePosition(uint16_t(10 + i), 60);
    MwSdlReadMousePosition(&x, &y);
    Check(x == 10 + i && y == 60, "the mouse without a device");
    float mx = 0, my = 0;
    MwSdlConvertMouseMotion(1000, 625, &mx, &my);
    Check(std::fabs(mx - W) < 0.01f && std::fabs(my - H) < 0.01f, "the motion without a device");
  }
  Say(SDL_EVENT_RENDER_DEVICE_LOST);
  mwSdlCombatActive = false;
  MwSdlShutdownVideo();
}

// What the program says, each once: run with stderr in a file.
void Said(const std::filesystem::path &log) {
  std::fflush(stderr);
  const std::vector<uint8_t> bytes = Contents(log);
  const std::string text(bytes.begin(), bytes.end());
  const auto times = [&](const char *what) {
    int n = 0;
    for (size_t at = 0; (at = text.find(what, at)) != std::string::npos; ++at) ++n;
    return n;
  };
  Check(times("can't draw: ") == 1, "a frame that can't be drawn is said once");
  Check(times("can't draw: Texture dimensions") == 1, "and why");
  Check(times("device is lost") == 1, "a lost device is said once");
  Check(times("device was reset") == 4, "each reset is said");
}

}  // namespace

int main(int argc, char **argv) {
  if (argc != 2) {
    std::printf("usage: picture_test WORK\n");
    return 2;
  }
  SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "dummy", SDL_HINT_OVERRIDE);
  SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "software", SDL_HINT_OVERRIDE);
  Rect();
  Draw();
  Mouse();
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    std::printf("FAIL SDL_Init: %s\n", SDL_GetError());
    return 1;
  }
  Check(!std::strcmp(SDL_GetCurrentVideoDriver(), "dummy"), "the dummy video driver");
  Host(argv[1]);
  const std::filesystem::path log = std::filesystem::path(argv[1]) / "stderr.txt";
  if (std::freopen(log.string().c_str(), "wb", stderr)) {
    Device(std::filesystem::path(argv[1]) / "device");
    Said(log);
  } else {
    Check(false, "stderr to a file");
  }
  SDL_Quit();
  std::printf(failures ? "%d failed\n" : "All checks passed\n", failures);
  return failures ? 1 : 0;
}
