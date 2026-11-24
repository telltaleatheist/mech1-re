#include "SDLSetup.h"
#include "SDLBackend.h"
#include "Setup/Setup.h"

#include <rekit/sdl/pickers.h>
#include <rekit/sdl/setup_screen.h>
#include <rekit/sources.h>
#include <rekit/status.h>

#include <SDL3/SDL.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
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

// The words for a find: which BTECH.EXE it holds.
std::string describe(const rekit::Found& f) {
  const std::string what = f.version == "1990-01-22" ? " (the 1990-01-22 revision)" : f.version == "reference" ? " (the 1989 release)" : "";
  return std::string(f.on_disk ? "An installed copy of " : "A download of ") + setup::kTitle + what + ".";
}

// MW1's look on the kit's screen: the rebuild's badge, its screens' face (Saira Condensed Bold, SIL OFL, beside it
// in data/setup) and colours. Nothing in it is the original game's.
rekit::sdl::Theme theme() {
  rekit::sdl::Theme t;
  const std::filesystem::path setup = std::filesystem::path(MwSdlDataDirectory()) / "setup";
  t.header = "MECHWARRIOR 1";
  t.font = (setup / "SairaCondensed-Bold.ttf").string();
  t.badge = (setup / "badge.png").string();
  t.bg = {10, 11, 13, 255}, t.band_colour = {24, 26, 30, 255}, t.rule = {255, 176, 64, 255};
  t.text = {233, 236, 238, 255}, t.dim = {150, 156, 162, 255}, t.accent = {255, 176, 64, 255};
  t.plate_colour = {52, 56, 62, 255}, t.plate_off = {34, 36, 40, 255};
  t.good = {255, 176, 64, 255}, t.bad = {240, 150, 130, 255};
  return t;
}

// The kit's screen with MW1's recipe: the find offered or looked for, copied in, the game's folder (or "").
std::string run_screen(bool setupOnly, const std::string& found, bool searched, const std::string& user, int* exit) {
  rekit::sdl::Game g;
  g.id = setup::kId, g.title = setup::kTitle, g.folder_key = setup::kFolderKey, g.folder_name = setup::kFolderName;
  g.program = "BTECH.EXE";
  g.rules = setup::rules(), g.hints = setup::hints();
  const std::string pdir = program_dir();
  g.search = [user, pdir] {
    bool s = false;
    return setup::find_install("", user, pdir, &s);
  };
  g.describe = describe;
  g.recognise = setup::recognise;
  g.install = [](const rekit::Found& f, const std::string& out, const std::string&, const rekit::sdl::ProgressFn& progress) {
    const setup::Installed r = setup::install(f, out, [&](double at, const std::string& file) {
      return progress(rekit::sdl::Progress{"Copying the game files", at, file});
    });
    rekit::sdl::InstallResult o;
    o.ok = r.ok, o.cancelled = r.cancelled, o.install_dir = r.dir;
    o.summary = r.ok ? std::string(setup::kTitle) + " is set up: " + std::to_string(r.files) + " files copied into " + r.dir + "."
                : r.cancelled ? "Stopped. Nothing was changed."
                              : "Nothing was copied: " + r.error;
    return o;
  };
  rekit::sdl::SetupOptions o;
  o.hidden = unattended();
  o.user_folder = user, o.default_out = setup::default_install(user), o.program_dir = pdir;
  if (searched) o.look_first = found;  // found by the search: offered, to copy in or pass over
  else if (!found.empty()) o.in_use = found, o.play_button = !setupOnly;
  const rekit::sdl::SetupResult r = rekit::sdl::run_setup(g, theme(), o);
  *exit = r.exit;
  if (r.exit == rekit::kSetupFailed) say_error(setupOnly, r.error);
  return r.exit == rekit::kSetupReady ? r.install_dir : std::string();
}

// What the player picked, walked by the kit and copied into the player's folder. "" with `why` when not.
std::string take_copy(const std::string& picked, const std::string& user, std::string* why) {
  const rekit::Found f = rekit::find_source(picked, setup::rules());
  std::printf("setup: looked at %s: %s\n", picked.c_str(), f.kind == rekit::Found::kNothing ? "nothing" : f.version.c_str());
  if (!f.detail.empty()) std::fprintf(stderr, "%s", f.detail.c_str());
  if (f.kind != rekit::Found::kInstall) {
    *why = setup::recognise(picked);
    if (why->empty()) *why = "No copy of " + std::string(setup::kTitle) + " this program knows was found in " + picked + ".";
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

bool MwSetupUnattended(void) { return unattended(); }

int MwSetupPrintStatus(void) {
  // Anything else printed while answering goes to stderr: stdout carries the one JSON line.
  std::fflush(stdout);
  const std::string user = setup::user_folder();
  const rekit::Status st = setup::status(setup::remembered_folder(user), user, program_dir(), MECHWARRIOR_VERSION);
  std::printf("%s\n", rekit::status_json(st).c_str());
  std::fflush(stdout);
  return 0;
}

void MwSetupSettleUserFolder(void) {
  if (std::getenv("MECHWARRIOR_SAVE_DIRECTORY")) return;  // a folder named by the run: nothing to settle
  const setup::Settled s =
      setup::settle_user_folder(setup::pref_folder(setup::kOrganisation, setup::kApplication),
                                setup::pref_folder(setup::kOldOrganisation, setup::kOldApplication));
  if (s.kind == setup::Settled::kMoved)
    std::fprintf(stderr, "setup: moved %s to %s\n", s.from.c_str(), s.folder.c_str());
  else if (s.kind == setup::Settled::kMoveFailed)
    std::fprintf(stderr, "setup: %s; this start uses %s, the move is tried again at the next\n", s.error.c_str(),
                 s.folder.c_str());
  else if (s.kind == setup::Settled::kCarried)
    std::fprintf(stderr, "setup: copied %d files once from %s (left as it was)\n", s.files, s.from.c_str());
  if (!s.error.empty() && s.kind != setup::Settled::kMoveFailed) std::fprintf(stderr, "setup: %s\n", s.error.c_str());
}

int MwSetupResolveGame(bool setupOnly, char* folder, size_t size) {
  rekit::sdl::unattended() = [] { return unattended(); };
  const std::string user = setup::user_folder();
  std::string game;
  bool searched = false;
  const std::string found = setup::find_install(mwSdlGameFolder, user, program_dir(), &searched);
  if (!setupOnly && !found.empty() && !searched) game = found;
  // A test's stand-in for the player's pick: walked and copied in with no screen.
  if (const char* scripted = std::getenv("MECHWARRIOR_SETUP_PICK"); game.empty() && scripted) {
    std::string why;
    game = take_copy(scripted, user, &why);
    if (game.empty()) return say_error(setupOnly, why), rekit::kSetupFailed;
  }
  // A check's hidden screen (REKIT_SETUP_FRAMES, the kit's hooks driving it; still no picker), else none unattended.
  const bool checked = std::getenv("REKIT_SETUP_FRAMES") != nullptr;
  if (game.empty() && unattended() && !checked) {
    say_error(setupOnly, "No copy of " + std::string(setup::kTitle) + " is set up, and no player is at this run.");
    return setupOnly ? rekit::kSetupFailed : rekit::kSetupCancelled;
  }
  if (game.empty()) {  // a player is here (or a check): the kit's setup screen
    int exit = rekit::kSetupCancelled;
    game = run_screen(setupOnly, found, searched, user, &exit);
    if (game.empty()) {
      if (exit == rekit::kSetupCancelled) std::printf("setup: cancelled\n");
      return exit;
    }
  }
  if (game.size() >= size) return rekit::kSetupFailed;
  std::strcpy(folder, game.c_str());
  if (std::strcmp(folder, mwSdlGameFolder)) MwSdlSaveUserText("game_folder", folder);
  std::printf("setup: game files %s\n", folder);
  std::fflush(stdout);
  return rekit::kSetupReady;
}
