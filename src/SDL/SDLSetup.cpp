#include "SDLSetup.h"
#include "SDLBackend.h"
#include "SDLMenu.h"
#include "SDLNotice.h"
#include "Setup/FileCheck.h"
#include "Setup/ProgramRecord.h"
#include "Setup/Setup.h"

#include "mechwarrior/problem.h"

#include <rekit/sdl/game_menu.h>
#include <rekit/sdl/pickers.h>
#include <rekit/sdl/setup_screen.h>
#include <rekit/sdl/setup_serve.h>
#include <rekit/sources.h>
#include <rekit/status.h>

#include <SDL3/SDL.h>

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <exception>
#include <filesystem>
#include <iterator>
#include <string>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

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
  else MwNoteProblem(why.c_str());  // the start's message (SDLMain)
}

// A file check's one line, on stdout (and in the log); on stderr when stdout carries the launcher's answer. "" nothing
// was amiss; else what the player is told.
std::string say_files(const setup::FilesReport& r, std::FILE* to = stdout) {
  std::fprintf(to, "setup: %s\n", setup::files_log(r).c_str());
  std::fflush(to);
  return setup::files_note(r);
}

// The words for a find: which BTECH.EXE it holds, and what is amiss with its files.
std::string describe(const rekit::Found& f, bool imported, std::FILE* log) {
  const std::string which = setup::edition(f.version);
  const std::string what = which == "1989" ? " (the 1989 release)" : which.empty() ? "" : " (the " + which + " revision)";
  const setup::FilesReport report = setup::check_found(f, imported);
  say_files(report, log);
  if (const std::string files = setup::files_pick_note(report); !files.empty())
    return std::string(setup::kTitle) + (which.empty() ? "" : " (" + which + ")") + ". " + files;
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

// MW1's recipe for the kit's setup, on its screen or on the launcher's (`served`: stdout is the launcher's then).
// `in_use`: the game folder in use, "" none.
rekit::sdl::Game kit_game(const std::string& user, const std::string& in_use, bool served) {
  rekit::sdl::Game g;
  g.id = setup::kId, g.title = setup::kTitle, g.folder_key = setup::kFolderKey, g.folder_name = setup::kFolderName;
  g.program = "BTECH.EXE";
  g.rules = setup::rules(), g.hints = setup::hints();
  // A fresh setup searches this computer first and offers the catalog's download only when it finds no copy.
  g.original_catalogs.assign(std::begin(setup::kOriginalCatalogs), std::end(setup::kOriginalCatalogs));
  g.catalog_keys = {setup::kCatalogKey}, g.original_key = setup::kOriginalKey;
  g.version = setup::build_tag();
  const std::string pdir = program_dir();
  const bool imported = setup::story_imported(user);
  g.search = [user, pdir, in_use, imported, served] {
    bool s = false;
    if (in_use.empty()) return setup::find_install("", user, pdir, &s);
    // Choosing again: another copy than the one in use, or the player's own. But for the launcher a ready one of the
    // player's own is the answer while the one in use can't be played: its look finds that in place, and it is used.
    const std::string own = setup::default_install(user);
    if (served && setup::check_install(in_use, imported).state != rekit::Status::kReady &&
        setup::check_install(own, imported).state == rekit::Status::kReady)
      return own;
    return setup::find_other_install({in_use, own}, user, pdir);
  };
  std::FILE* const log = served ? stderr : stdout;
  g.describe = [imported, log](const rekit::Found& f) { return describe(f, imported, log); };
  // A copy found on this computer is shown first, with describe's words, when its edition is unknown or its files
  // aren't all the known ones; else it is installed at once.
  g.asks_first = [imported](const rekit::Found& f) {
    return setup::edition(f.version).empty() || !setup::files_pick_note(setup::check_found(f, imported)).empty();
  };
  g.recognise = setup::recognise;
  g.install = [imported](const rekit::Found& f, const std::string& out, const std::string&, const rekit::sdl::ProgressFn& progress) {
    const setup::Installed r = setup::install(f, out, [&](double at, const std::string& file) {
      return progress(rekit::sdl::Progress{"Copying the game files", at, file});
    });
    rekit::sdl::InstallResult o;
    o.ok = r.ok, o.cancelled = r.cancelled, o.install_dir = r.dir;
    o.summary = r.ok ? std::string(setup::kTitle) + " is set up: " + std::to_string(r.files) + " files copied into " + r.dir + "."
                : r.cancelled ? "Stopped. Nothing was changed."
                              : "Nothing was copied: " + r.error;
    if (r.ok)
      if (const std::string files = setup::files_note(setup::check_folder(r.dir, imported)); !files.empty()) o.summary += " " + files;
    return o;
  };
  return g;
}

// The kit's screen: the find offered or looked for, copied in, the game's folder (or "").
std::string run_screen(bool setupOnly, const std::string& found, bool searched, const std::string& user, int* exit) {
  const rekit::sdl::Game g = kit_game(user, searched ? std::string() : found, false);
  rekit::sdl::SetupOptions o;
  o.hidden = unattended();
  o.user_folder = user, o.default_out = setup::default_install(user), o.program_dir = program_dir();
  if (searched) o.look_first = found;  // found by the search: offered, to copy in or pass over
  else if (!found.empty()) {
    o.in_use = found, o.play_button = !setupOnly;
    const std::string files = say_files(setup::check_folder(found, setup::story_imported(user)));
    if (!files.empty()) o.notice = "The files in use: " + files;
    o.same_words = "Those are the files in use. Pick another copy.";
  }
  const rekit::sdl::SetupResult r = rekit::sdl::run_setup(g, theme(), o);
  *exit = r.exit;
  if (r.exit == rekit::kSetupFailed) say_error(setupOnly, r.error);
  return r.exit == rekit::kSetupReady ? r.install_dir : std::string();
}

// What the player picked, walked by the kit and copied into the player's folder. "" with `why` when not.
std::string take_copy(const std::string& picked, const std::string& user, std::string* why) {
  const rekit::Found f = rekit::find_source(picked, setup::rules());
  std::printf("setup: looked at %s: %s\n", picked.c_str(), f.kind == rekit::Found::kNothing ? "nothing" : setup::edition(f.version).c_str());
  if (!f.detail.empty()) std::fprintf(stderr, "%s", f.detail.c_str());
  if (f.kind != rekit::Found::kInstall) {
    *why = setup::recognise(picked);
    if (why->empty()) *why = "No copy of " + std::string(setup::kTitle) + " this program knows was found in " + picked + ".";
    return "";
  }
  const std::string files = say_files(setup::check_found(f, setup::story_imported(user)));
  if (!files.empty()) std::fprintf(stderr, "setup: %s\n", files.c_str());
  const setup::Installed r = setup::install(f, setup::default_install(user));
  if (!r.ok) {
    *why = r.error;
    return "";
  }
  std::printf("setup: copied %d files into %s\n", r.files, r.dir.c_str());
  return r.dir;
}

// What is wrong with a served ask's arguments, else "". The kit passes over arguments it doesn't know, and would
// install into the default folder for a --folder without its folder.
std::string wrong_ask(const std::vector<std::string>& args) {
  const std::string& ask = args[0];
  const std::size_t n = args.size();
  if (ask == "--describe" || ask == "--search") return n == 1 ? "" : "--setup " + ask + " takes nothing after it.";
  if (n < 2 || args[1].empty()) return "--setup " + ask + " needs the pick.";
  if (ask == "--look") return n == 2 ? "" : "--setup --look takes the pick and nothing after it.";
  if (n == 2 || (n == 4 && args[2] == "--folder" && !args[3].empty())) return "";
  return "--setup --install takes the pick, then --folder and a folder or nothing.";
}

// A test's hook without the test's own player's folder: why a served ask is refused, else "".
std::string hook_without_folder() {
  if (const char* own = std::getenv("MECHWARRIOR_SAVE_DIRECTORY"); own && *own) return "";
  for (const char* hook : {"MECHWARRIOR_SETUP_PICK", "REKIT_SETUP_GO"})
    if (const char* v = std::getenv(hook); v && *v)
      return std::string(hook) + " is a test's, so it needs MECHWARRIOR_SAVE_DIRECTORY to name the test's own folder. Nothing was done.";
  return "";
}

// A served ask that isn't answered: its last line and exit.
int refuse_ask(const std::string& why) {
  rekit::SetupMessage m;
  m.done = true, m.summary = why;
  std::printf("%s\n", rekit::setup_message_json(m).c_str());
  std::fflush(stdout);
  std::fprintf(stderr, "setup: %s\n", why.c_str());
  return rekit::kSetupFailed;
}

// stderr of a served look or install, into setup.log in the player's folder, anew each time: the launcher shows
// nobody what is said there. Left where it is when the file can't be written.
void open_served_log(const std::string& user) {
  std::error_code ec;
  std::filesystem::create_directories(user, ec);
  const std::string path = (std::filesystem::path(user) / "setup.log").string();
  std::FILE* can = std::fopen(path.c_str(), "w");  // freopen closes a stream it can't open: see first that it can
  if (!can) return;
  std::fclose(can);
  if (std::freopen(path.c_str(), "w", stderr)) std::setvbuf(stderr, nullptr, _IONBF, 0);
}

#if defined(_WIN32)
// Inno Setup's record of an install (tools/package/installer.iss: its AppId) in `root` (this user's registry or the
// machine's), naming `folder`.
bool setup_record_names(HKEY root, const std::filesystem::path& folder) {
  HKEY key;
  if (RegOpenKeyExW(root, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{56BFB0C4-541E-42DD-909E-874C8EDA0D47}_is1", 0,
                    KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS)
    return false;
  wchar_t value[4096] = {};
  DWORD size = sizeof(value) - sizeof(wchar_t), type = 0;
  const bool read = RegQueryValueExW(key, L"InstallLocation", nullptr, &type, reinterpret_cast<BYTE*>(value), &size) == ERROR_SUCCESS &&
                    type == REG_SZ;
  RegCloseKey(key);
  std::error_code ec;
  return read && std::filesystem::equivalent(std::filesystem::path(value), folder, ec);
}
#endif

// How this program was installed, for the menu's UPDATES (rekit/update.h): a build that is no release is never
// updated; a Mac app; Inno Setup's install (its uninstaller beside the program), for this user or every user; an
// AppImage; else the folder a zip or tarball was unpacked into.
rekit::InstallKind install_kind() {
  using K = rekit::InstallKind;
  if (!*setup::build_tag()) return K::kDevelopment;
  const std::string self = setup::self_path();
  if (self.empty()) return K::kUnknown;
  const std::filesystem::path dir = std::filesystem::path(self).parent_path();
#if defined(__APPLE__)
  const std::filesystem::path contents = dir.parent_path();
  return dir.filename() == "MacOS" && contents.filename() == "Contents" && contents.parent_path().extension() == ".app" ? K::kMacApp
                                                                                                                      : K::kUnknown;
#elif defined(_WIN32)
  std::error_code ec;
  if (!std::filesystem::exists(dir / "unins000.exe", ec)) return K::kFolder;
  if (setup_record_names(HKEY_CURRENT_USER, dir)) return K::kWindowsSetupUser;
  return setup_record_names(HKEY_LOCAL_MACHINE, dir) ? K::kWindowsSetupMachine : K::kUnknown;
#else
  const char *appimage = std::getenv("APPIMAGE"), *appdir = std::getenv("APPDIR");
  if (appimage && appdir && setup::started_file(self, appimage, appdir) == appimage) return K::kAppImage;
  return K::kFolder;
#endif
}

// The release's file an install of this kind updates from (tools/build_package.py names them).
std::string release_file(rekit::InstallKind kind) {
  using K = rekit::InstallKind;
  switch (kind) {
    case K::kWindowsSetupUser:
    case K::kWindowsSetupMachine: return "MW1-Rebuild-{tag}-Setup.exe";
    case K::kMacApp: return "MW1-Rebuild-{tag}-mac.dmg";
    case K::kAppImage: return "MW1-Rebuild-{tag}-x86_64.AppImage";
#if defined(_WIN32)
    case K::kFolder: return "MW1-Rebuild-{tag}-win64.zip";
#else
    case K::kFolder: return "MW1-Rebuild-{tag}-linux-x86_64.tar.gz";
#endif
    default: return "";
  }
}

// What the menu says and does for MW1 (SDLMenu.h), for the game folder in use.
rekit::sdl::MenuHooks menu_hooks(const std::string& game) {
  namespace fs = std::filesystem;
  rekit::sdl::MenuHooks h;
  const std::string user = setup::user_folder();  // MECHWARRIOR_SAVE_DIRECTORY when the run names one
  const bool imported = setup::story_imported(user);
  h.title = setup::kTitle;
  h.version = MECHWARRIOR_VERSION;
  h.user_folder = user;
  h.log_file = MwSdlLogPath();
  h.files_in_use = game;
  h.report_url = "https://github.com/telltaleatheist/mech1-re/issues";
  h.install = install_kind();
  // The program's folder: on the Mac the app's Contents/Resources, where the package's record is (the signature
  // seals the rest); an AppImage's usr/bin.
  h.program_dir = program_dir();
  h.leave_out = setup::leave_out();
  // A copy installed before the kit kept records: the program's own check of the files it knows.
  h.check_game_files = [game, imported] {
    std::vector<std::string> problems;
    if (const std::string note = setup::files_note(setup::check_folder(game, imported)); !note.empty()) problems.push_back(note);
    return problems;
  };
  const std::string settings = MwSdlUserConfigurationPath();
  if (!settings.empty()) h.settings = {settings};
  h.reset_settings = [] { return MwSdlResetUserConfiguration(); };
  // UPDATES: the Rebuild's releases (mw1r-vX.Y.Z on mech1-re), found through the launcher's signed catalog; before an
  // update the saves, the story and the settings are copied to Backups. A build that is no release has no version to
  // compare: no check at all (an empty repo).
  if (!*setup::build_tag()) return h;
  rekit::sdl::UpdateSource& u = h.update;
  u.recipe = {setup::kOriginalKey, "mech1-re", "mw1r-v", setup::build_tag(), release_file(h.install)};
  u.catalogs.assign(std::begin(setup::kOriginalCatalogs), std::end(setup::kOriginalCatalogs));
  u.keys = {setup::kCatalogKey};
  std::error_code ec;
  for (fs::directory_iterator it(user, ec), end; !ec && it != end; it.increment(ec)) {
    std::string ext = it->path().extension().string();
    for (char& c : ext) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (ext == ".GAM" && it->is_regular_file(ec)) u.keep.push_back(it->path().string());
  }
  u.keep.push_back((fs::path(user) / "story").string());
  if (!settings.empty()) u.keep.push_back(settings);
  return h;
}

}  // namespace

MwSdlMenuRecipe MwSetupMenuRecipe(void) { return MwSdlMenuRecipe{menu_hooks, theme}; }

rekit::sdl::Game MwSetupKitGame(const std::string& user) { return kit_game(user, "", false); }

bool MwSetupUnattended(void) { return unattended(); }

int MwSetupPrintStatus(void) {
  // Anything else printed while answering goes to stderr: stdout carries the one JSON line.
  std::fflush(stdout);
  const std::string user = setup::user_folder();
  rekit::Status st = setup::status(setup::remembered_folder(user), user, program_dir(), MECHWARRIOR_VERSION);
  // The files, for a copy that is ready and has nothing else to be said of it.
  if (st.state == rekit::Status::kReady && st.note.empty())
    st.note = setup::files_status_note(setup::check_folder(st.where, setup::story_imported(user)));
  st.setup = rekit::kServedSetup;  // MwSetupServe answers the launcher
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

void MwSetupRecordProgram(void) {
  const char *appimage = std::getenv("APPIMAGE"), *appdir = std::getenv("APPDIR");
  setup::ProgramStart s;
  s.hidden = unattended(), s.tag = setup::build_tag(), s.self = setup::self_path();
  s.appimage = appimage ? appimage : "", s.appdir = appdir ? appdir : "";
  s.user = setup::user_folder();
  setup::record_program(s);
}

bool MwSetupServedAsk(const char* arg) {
  for (const char* ask : {"--describe", "--search", "--look", "--install"})
    if (!std::strcmp(arg, ask)) return true;
  return false;
}

int MwSetupServe(int count, char** words) {
  const std::vector<std::string> args(words, words + count);
#if !defined(_WIN32)
  std::signal(SIGPIPE, SIG_IGN);  // a launcher that has gone: the work goes on
#endif
  if (const std::string why = wrong_ask(args); !why.empty()) return refuse_ask(why);
  if (const std::string why = hook_without_folder(); !why.empty()) return refuse_ask(why);
  MwSetupSettleUserFolder();
  const std::string user = setup::user_folder();
  if (user.empty()) return refuse_ask("There is no folder for the player's files.");
  try {
    if (args[0] == "--look" || args[0] == "--install") open_served_log(user);
    const std::string remembered = setup::remembered_folder(user);
    std::string in_use;
    if (args[0] == "--search") {
      bool searched = false;
      in_use = setup::find_install(remembered, user, program_dir(), &searched);
      if (searched) in_use.clear();
    }
    rekit::sdl::SetupOptions o;
    o.hidden = true, o.user_folder = user, o.default_out = setup::default_install(user), o.program_dir = program_dir();
    const rekit::sdl::SetupResult r = rekit::sdl::serve_setup(kit_game(user, in_use, true), o, args);
    if (!r.install_dir.empty() && remembered != r.install_dir) {
      MwSdlLoadUserConfiguration();  // names the file; what it holds isn't used here
      MwSdlSaveUserText("game_folder", r.install_dir.c_str());
      if (setup::remembered_folder(user) != r.install_dir)
        std::fprintf(stderr, "setup: couldn't save the game folder in MechWarrior.ini\n");
    }
    return r.exit;
  } catch (const std::exception& e) {
    return refuse_ask(std::string("The setup failed: ") + e.what());
  }
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
