#include "SDLSetup.h"
#include "SDLBackend.h"
#include "Setup/Setup.h"

#include <rekit/sdl/pickers.h>
#include <rekit/sources.h>
#include <rekit/status.h>

#include <SDL3/SDL.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>

#ifndef MECHWARRIOR_VERSION
#define MECHWARRIOR_VERSION "dev"
#endif

namespace setup = mw1o::setup;

namespace {

std::string program_dir() {
  const char* base = SDL_GetBasePath();
  return base ? base : "";
}

// No player at this run: a hidden or scripted one (MW1O_HIDDEN, an offscreen or dummy video driver). No picker,
// message box or window that takes focus ever opens then.
bool unattended() {
  if (std::getenv("MW1O_HIDDEN")) return true;
  const char* driver = std::getenv("SDL_VIDEO_DRIVER");
  return driver && (!std::strcmp(driver, "offscreen") || !std::strcmp(driver, "dummy"));
}

std::string json_text(const std::string& s) {
  std::string o;
  for (char c : s) {
    if (c == '"' || c == '\\') o += '\\', o += c;
    else if (static_cast<unsigned char>(c) < 0x20) {
      char b[8];
      std::snprintf(b, sizeof(b), "\\u%04x", static_cast<unsigned char>(c));
      o += b;
    } else o += c;
  }
  return o;
}

void say_error(bool setupOnly, const std::string& why) {
  std::fprintf(stderr, "setup: %s\n", why.c_str());
  if (setupOnly) std::printf("{\"error\": \"%s\"}\n", json_text(why).c_str()), std::fflush(stdout);
}

// The player's pick, from the kit's file picker (SDL3 has no file-or-folder dialog: a folder is picked by one of
// its files, as mech3-re's setup does).
struct Pick {
  std::mutex m;
  bool answered = false;
  std::string path;
  static void SDLCALL done(void* user, const char* const* list, int) {
    auto* p = static_cast<Pick*>(user);
    std::lock_guard<std::mutex> g(p->m);
    p->answered = true;
    if (list && list[0]) p->path = list[0];
  }
};

// Interim, until the kit's setup screen (re-setup-kit DESIGN step 5) replaces it: the file picker alone.
std::string ask_for_copy(const std::string& start) {
  static const SDL_DialogFileFilter filters[] = {
      {"MechWarrior's BTECH.EXE, or the archive it came in", "exe;EXE;zip;7z;rar;tar;gz;tgz;xz"},
      {"All files", "*"},
  };
  Pick pick;
  rekit::sdl::PickerAsk ask;
  ask.kind = rekit::sdl::PickerKind::kOpenFile;
  ask.done = Pick::done, ask.user = &pick;
  ask.filters = filters, ask.filter_count = 2;
  ask.start = start.empty() ? nullptr : start.c_str();
  if (!rekit::sdl::open_picker(ask)) return "";
  for (;;) {
    {
      std::lock_guard<std::mutex> g(pick.m);
      if (pick.answered) return pick.path;
    }
    SDL_WaitEventTimeout(nullptr, 50);
  }
}

// What the player picked, walked by the kit and copied into the player's folder. "" with `why` when not.
std::string take_copy(const std::string& picked, const std::string& user, std::string* why) {
  const rekit::Found f = rekit::find_source(picked, setup::rules());
  std::printf("setup: looked at %s: %s\n", picked.c_str(), f.kind == rekit::Found::kNothing ? "nothing" : f.version.c_str());
  if (!f.detail.empty()) std::fprintf(stderr, "%s", f.detail.c_str());
  if (f.kind != rekit::Found::kInstall) {
    *why = "No copy of " + std::string(setup::kTitle) + " this program knows was found in " + picked + ".";
    return "";
  }
  const setup::Installed r = setup::install(f, setup::default_install(user));
  if (!r.ok) {
    *why = r.error;
    return "";
  }
  std::printf("setup: copied %d files into %s\n", r.files, r.dir.c_str());
  return r.dir;
}

}  // namespace

int MwSetupPrintStatus(void) {
  // Anything else printed while answering goes to stderr: stdout carries the one JSON line.
  std::fflush(stdout);
  const std::string user = setup::user_folder();
  const rekit::Status st = setup::status(setup::remembered_folder(user), user, program_dir(), MECHWARRIOR_VERSION);
  std::printf("%s\n", rekit::status_json(st).c_str());
  std::fflush(stdout);
  return 0;
}

void MwSetupCarryOver(void) {
  if (std::getenv("MECHWARRIOR_SAVE_DIRECTORY")) return;  // a folder named by the run: nothing to carry
  const std::string from = setup::pref_folder(setup::kOldOrganisation, setup::kOldApplication);
  const setup::Carried c = setup::carry_over(from, setup::user_folder());
  if (c.done) std::fprintf(stderr, "setup: copied %d files once from %s (left as it was)\n", c.files, from.c_str());
  if (!c.error.empty()) std::fprintf(stderr, "setup: %s\n", c.error.c_str());
}

int MwSetupResolveGame(bool setupOnly, char* folder, size_t size) {
  rekit::sdl::unattended() = [] { return unattended(); };
  const std::string user = setup::user_folder();
  std::string game;
  bool searched = false;
  const std::string found = setup::find_install(mwSdlGameFolder, user, program_dir(), &searched);
  if (!setupOnly && !found.empty() && !searched) game = found;
  // A test's stand-in for the player's pick (no picker in an unattended run).
  const char* scripted = std::getenv("MECHWARRIOR_SETUP_PICK");
  while (game.empty()) {
    std::string picked;
    if (scripted) picked = scripted;
    else {
      if (unattended()) {
        say_error(setupOnly, "No copy of " + std::string(setup::kTitle) + " is set up, and no player is at this run.");
        return setupOnly ? rekit::kSetupFailed : rekit::kSetupCancelled;
      }
      if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        say_error(setupOnly, std::string("SDL could not start: ") + SDL_GetError());
        return rekit::kSetupFailed;
      }
      picked = ask_for_copy(searched ? found : std::string());
      if (picked.empty()) {
        std::printf("setup: cancelled\n");
        return rekit::kSetupCancelled;
      }
    }
    std::string why;
    game = take_copy(picked, user, &why);
    if (game.empty()) {
      say_error(setupOnly, why);
      if (scripted) return rekit::kSetupFailed;
    }
  }
  if (game.size() >= size) return rekit::kSetupFailed;
  std::strcpy(folder, game.c_str());
  if (std::strcmp(folder, mwSdlGameFolder)) MwSdlSaveUserText("game_folder", folder);
  std::printf("setup: game files %s\n", folder);
  std::fflush(stdout);
  return rekit::kSetupReady;
}
