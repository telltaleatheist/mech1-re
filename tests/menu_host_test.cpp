// The kit's menu over the game, the host's half (src/SDL/SDLMenu.cpp): its keys open it and stop the game's clock,
// its picture is drawn over the frame, Esc closes it and the clock goes on with nothing to catch up; RESET SETTINGS'
// part (src/SDL/SDLBackend_Config.cpp). A hidden window on SDL's dummy drivers: nothing on the screen, no sound.
#include "SDLBackend.h"
#include "SDLMenu.h"

#include <SDL3/SDL.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

int g_failures = 0;
void check(bool ok, const char* what) {
  std::printf("%s %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok) ++g_failures;
}
// Both the process's environment (getenv: the host's MECHWARRIOR_SAVE_DIRECTORY) and SDL's copy of it (the kit's).
void set_env(const char* name, const char* value) {
#if defined(_WIN32)
  _putenv_s(name, value ? value : "");
#else
  if (value) setenv(name, value, 1);
  else unsetenv(name);
#endif
  SDL_Environment* env = SDL_GetEnvironment();
  if (value) SDL_SetEnvironmentVariable(env, name, value, true);
  else SDL_UnsetEnvironmentVariable(env, name);
}
SDL_Event key(SDL_Keycode code, SDL_Scancode scancode, SDL_Keymod mod) {
  SDL_Event e{};
  e.type = SDL_EVENT_KEY_DOWN;
  e.key.key = code, e.key.scancode = scancode, e.key.mod = mod, e.key.down = true;
  e.key.windowID = SDL_GetWindowID(MwSdlVideoWindow());
  return e;
}
bool has(const std::vector<std::string>& log, const std::string& head) {
  for (const std::string& l : log)
    if (l.rfind(head, 0) == 0) return true;
  return false;
}
// The overlay drawn on a surface of the window's pixel size, filled with one colour first: whether any pixel changed.
bool overlay_draws(int w, int h) {
  SDL_Surface* s = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
  SDL_Renderer* r = s ? SDL_CreateSoftwareRenderer(s) : nullptr;
  if (!r) return SDL_DestroySurface(s), false;
  SDL_SetRenderDrawColor(r, 0, 255, 0, 255);
  SDL_RenderClear(r);
  MwSdlMenuOverlay(r);
  SDL_RenderPresent(r);
  bool changed = false;
  const auto* px = static_cast<const uint8_t*>(s->pixels);
  for (int y = 0; y < h && !changed; y += 7)
    for (int x = 0; x < w && !changed; x += 7) {
      const uint8_t* p = px + y * s->pitch + x * 4;
      changed = p[0] != 0 || p[1] != 255 || p[2] != 0;
    }
  SDL_DestroyRenderer(r);
  SDL_DestroySurface(s);
  return changed;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) return std::fprintf(stderr, "usage: menu_host_test <work folder>\n"), 2;
  const fs::path work = fs::absolute(argv[1]);
  std::error_code ec;
  fs::remove_all(work, ec);
  fs::create_directories(work / "user");
  SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
  SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
  // This test presses the keys, which RE_KIT_TEST turns off; RE_LAUNCHER keeps the hint off instead (no picture
  // while the menu is closed). No update source: nothing asks the network.
  set_env("RE_KIT_TEST", nullptr);
  set_env("RE_LAUNCHER", "1");
  set_env("MW1O_HIDDEN", "1");
  set_env("MECHWARRIOR_SAVE_DIRECTORY", (work / "user").string().c_str());
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) return std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError()), 1;
  if (!MwSdlInitializeVideo(work.string().c_str())) return std::fprintf(stderr, "video: %s\n", SDL_GetError()), 1;
  MwSdlInitializeTimer();

  std::vector<std::string> log;
  std::string files_in_use;
  MwSdlSetMenuRecipe(MwSdlMenuRecipe{[&](const std::string& game) {
                                       rekit::sdl::MenuHooks h;
                                       h.title = "MechWarrior 1";
                                       h.user_folder = (work / "user").string();
                                       h.files_in_use = files_in_use = game;
                                       h.log = [&log](const std::string& l) { log.push_back(l); };
                                       return h;
                                     },
                                     [] { return rekit::sdl::Theme{}; }});
  MwSdlStartMenu("the game folder");
  check(files_in_use == "the game folder", "the recipe is asked for the game folder in use");
  int w = 0, h = 0;
  SDL_GetWindowSizeInPixels(MwSdlVideoWindow(), &w, &h);

  uint64_t before = MwSdlNowNanoseconds();
  SDL_DelayNS(20000000);
  check(!MwSdlClockPaused() && MwSdlNowNanoseconds() - before >= 15000000, "closed: the game's clock runs");
  check(!overlay_draws(w, h), "closed: nothing drawn over the frame");

  MwSdlMenuEvent(key(SDLK_F1, SDL_SCANCODE_F1, rekit::sdl::GameMenu::keys()));
  check(MwSdlClockPaused(), "the menu's keys open it and pause the game's clock");
  check(has(log, "menu: open"), "the kit says the menu opened");
  const uint64_t paused = MwSdlNowNanoseconds();
  SDL_DelayNS(50000000);
  check(MwSdlNowNanoseconds() == paused, "open: the game's time stands still");
  check(overlay_draws(w, h), "open: the menu drawn over the frame");
  MwSdlMenuPump();  // the menu's own frame (the game draws none while paused)

  MwSdlMenuEvent(key(SDLK_ESCAPE, SDL_SCANCODE_ESCAPE, SDL_KMOD_NONE));
  check(!MwSdlClockPaused(), "Esc closes it and the clock goes on");
  const uint64_t after = MwSdlNowNanoseconds();
  check(after >= paused && after - paused < 10000000, "closed: the time it was open is not caught up");
  MwSdlStopMenu();

  // RESET SETTINGS: the player's file keeps only the game folder; the packaged speed is back at once.
  {
    std::ofstream(work / "user" / "MechWarrior.ini") << "combat_speed=3\nmusic=speaker\ngame_folder=/games/mw1\n";
    const uint8_t packaged = mwSdlCombatSpeed;
    check(MwSdlLoadUserConfiguration() && mwSdlCombatSpeed == 3, "the player's settings read");
    check(MwSdlResetUserConfiguration(), "RESET SETTINGS done");
    std::stringstream text;
    text << std::ifstream(work / "user" / "MechWarrior.ini").rdbuf();
    check(text.str() == "game_folder=/games/mw1\n", "the player's file keeps the game folder alone");
    check(mwSdlCombatSpeed == packaged, "the packaged combat speed is back in play");
  }

  MwSdlShutdownVideo();
  SDL_Quit();
  fs::remove_all(work, ec);
  std::printf("%s\n", g_failures ? "FAILED" : "all passed");
  return g_failures ? 1 : 0;
}
