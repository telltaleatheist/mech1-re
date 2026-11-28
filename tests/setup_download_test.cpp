// The original files from the launcher's catalog (the kit's Game::original_catalogs): MW1's recipe names the catalog,
// its key and the Rebuild's entry, and a fresh setup that finds no copy gets the download at once. The catalog
// here is a local one, signed by the kit's test key, its source a zip built in the work folder (file:// stands in for
// https://, REKIT_SETUP_ALLOW_FILE): nothing is fetched from the network. With the player's files (MW1O_GAME,
// MW1O_BTECH_REFERENCE) the zip holds the game and NEV_UNIV.EXE, and the install is ready without it; without them
// the zip holds a stand-in, offered and fetched but no game. Prints names and counts only, never the game's bytes.
#include "SDLSetup.h"
#include "Setup/ProgramRecord.h"
#include "Setup/Setup.h"
#include "test_keys.h"

#include <rekit/files.h>
#include <rekit/hash.h>
#include <rekit/sdl/pickers.h>
#include <rekit/sdl/setup_screen.h>
#include <rekit/sign.h>
#include <SDL3/SDL.h>

#if REKIT_HAVE_LIBARCHIVE
#include <archive.h>
#include <archive_entry.h>
#endif

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;
namespace setup = mw1o::setup;

namespace {

int g_failures = 0;

void check(bool ok, const std::string& what) {
  std::printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
  if (!ok) ++g_failures;
}

#if REKIT_HAVE_LIBARCHIVE
std::string read(const fs::path& p) {
  std::ifstream in(p, std::ios::binary);
  return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

void write(const fs::path& p, const std::string& text) {
  fs::create_directories(p.parent_path());
  std::ofstream(p, std::ios::binary) << text;
}

// A zip of `files` (name in the archive, its bytes).
bool make_zip(const fs::path& out, const std::vector<std::pair<std::string, std::string>>& files) {
  archive* a = archive_write_new();
  archive_write_set_format_zip(a);
  if (archive_write_open_filename(a, out.string().c_str()) != ARCHIVE_OK) return archive_write_free(a), false;
  for (const auto& [name, data] : files) {
    archive_entry* e = archive_entry_new();
    archive_entry_set_pathname(e, name.c_str());
    archive_entry_set_size(e, static_cast<la_int64_t>(data.size()));
    archive_entry_set_filetype(e, AE_IFREG);
    archive_entry_set_perm(e, 0644);
    archive_write_header(a, e);
    archive_write_data(a, data.data(), data.size());
    archive_entry_free(e);
  }
  archive_write_close(a);
  archive_write_free(a);
  return true;
}

// A file:/// address for `p` ("file:///E:/..." on Windows, as the kit takes one in a test run), percent-encoded, so a
// checkout's path may hold a space.
std::string file_url(const fs::path& p) {
  static const char* hex = "0123456789ABCDEF";
  const std::string path = p.generic_string();
  std::string url = path.rfind('/', 0) == 0 ? "file://" : "file:///";
  for (const char ch : path) {
    const auto c = static_cast<unsigned char>(ch);
    if (std::isalnum(c) || std::strchr("/-._~:", c)) {
      url += ch;
    } else {
      url += '%';
      url += hex[c >> 4];
      url += hex[c & 15];
    }
  }
  return url;
}

bool has(const std::vector<std::string>& log, const std::string& line) {
  for (const std::string& l : log)
    if (l == line) return true;
  return false;
}

bool starts(const std::vector<std::string>& log, const std::string& head) {
  for (const std::string& l : log)
    if (l.rfind(head, 0) == 0) return true;
  return false;
}

#endif

// The recipe's catalog: the launcher's two addresses, its key, the Rebuild's entry, this build's tag.
void recipe(const fs::path& work) {
  const rekit::sdl::Game g = MwSetupKitGame((work / "user").string());
  check(g.original_catalogs.size() == 2 && g.original_catalogs[0] == "https://reverse-engineered.com/launcher/catalog.json" &&
            g.original_catalogs[1] == "https://raw.githubusercontent.com/telltaleatheist/mw3-re/main/catalog/games.json",
        "the recipe asks the launcher's catalog, then its copy on GitHub");
  check(g.catalog_keys.size() == 1 && g.catalog_keys[0] == setup::kCatalogKey && g.catalog_keys[0].size() == 64,
        "the recipe checks the catalog's signature against the launcher's key");
  check(g.original_key == "mw1r", "the recipe's entry is the Rebuild's (mw1r), not the Remake's (mw1)");
  check(g.version == setup::build_tag(), "the recipe's version is the build's release tag (\"" + g.version + "\")");
}

#if REKIT_HAVE_LIBARCHIVE
// A fresh setup that finds no copy: the download started at once (the small window), fetched, and installed when it
// is the game.
void download(const fs::path& work, const char* game, const char* ref) {
  const bool real = game && ref;
  std::string main_exe;
  if (real) main_exe = rekit::any_case_path(game, "MW_MAIN.EXE");
  std::vector<std::pair<std::string, std::string>> files;
  if (real && !main_exe.empty()) {
    files = {{"mechwarrior/MECHWAR/BTECH.EXE", read(ref)}, {"mechwarrior/MECHWAR/MW_MAIN.EXE", read(main_exe)}};
  } else {
    files = {{"mechwarrior/MECHWAR/BTECH.EXE", "a stand-in"}};
  }
  files.push_back({"mechwarrior/MECHWAR/NEV_UNIV.EXE", "a stand-in"});
  const fs::path served = work / "served", zip = served / "MechWarrior_1989.zip";
  fs::create_directories(served);
  check(make_zip(zip, files), "a zip built in the work folder");
  const std::string bytes = read(zip);
  const std::string sum = rekit::sha256_hex(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size());
  const std::string source = "{\"url\": \"" + file_url(zip) + "\", \"sha256\": \"" + sum + "\", \"size\": " +
                             std::to_string(bytes.size()) + ", \"name\": \"MechWarrior_1989.zip\", \"note\": \"a test\"}";
  const std::string text =
      "{\"revision\": 9, \"games\": [{\"key\": \"mw1r\", \"name\": \"MechWarrior 1\", \"original\": [" + source + "]}]}";
  const fs::path cat = work / "catalog.json";
  write(cat, text);
  write(cat.string() + ".sig", test_keys::sign(text, 1));

  const fs::path user = work / "user";
  rekit::sdl::Game g = MwSetupKitGame(user.string());
  g.original_catalogs = {file_url(cat)};
  g.catalog_keys = {test_keys::public_key(1)};
  g.search = [] { return std::string(); };  // nothing on this computer: the test never looks at the real one

  std::vector<std::string> log;
  rekit::sdl::SetupOptions o;
  o.user_folder = user.string(), o.default_out = setup::default_install(user.string()), o.hidden = true;
  o.temp_dir = (work / "temp").string();
  fs::create_directories(o.temp_dir);
  o.log = [&log](const std::string& l) { log.push_back(l); };
  SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
  SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "REKIT_SETUP_ALLOW_FILE", "1", true);
  SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "REKIT_SETUP_DOWNLOAD", "1", true);
  SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "REKIT_SETUP_FRAMES", "60", true);
  rekit::sdl::unattended() = [] { return true; };
  int pickers = 0;
  rekit::sdl::picker_opener() = [&pickers](const rekit::sdl::PickerAsk&) { ++pickers; };
  const rekit::sdl::SetupResult r = rekit::sdl::run_setup(g, rekit::sdl::Theme{}, o);

  check(has(log, "start: no copy found on this computer"), "a fresh setup searched this computer first");
  check(has(log, "window small") && has(log, "screen asking: USE MY OWN COPY INSTEAD, CANCEL"),
        "no copy found: the download asked for at once, in the small window, the player's own copy offered instead");
  check(has(log, "work: GETTING THE ORIGINAL GAME FILES") && has(log, "original: downloaded MechWarrior_1989.zip"),
        "the download fetched from the catalog's source");
  check(pickers == 0, "no picker opened");
  if (real && !main_exe.empty()) {
    const fs::path out = o.default_out;
    check(r.exit == rekit::kSetupReady && fs::exists(out / "BTECH.EXE"), "the download installed, ready");
    check(!fs::exists(out / "NEV_UNIV.EXE"), "the download's NEV_UNIV.EXE left out of the install");
  } else {
    check(r.exit != rekit::kSetupReady && !starts(log, "work: INSTALLING"), "break: a stand-in download is not installed");
    std::printf("skip the install: MW1O_GAME and MW1O_BTECH_REFERENCE not both set\n");
  }
  if (g_failures)
    for (const std::string& l : log) std::printf("    | %s\n", l.c_str());
}
#endif

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) return std::fprintf(stderr, "usage: setup_download_test <work folder>\n"), 2;
  const fs::path work = fs::absolute(argv[1]).lexically_normal();  // a catalog's addresses hold no ".."
  std::error_code ec;
  fs::remove_all(work, ec);  // the test's own folder (the build dir's setup_download_work)
  fs::create_directories(work);
  recipe(work);
#if REKIT_HAVE_LIBARCHIVE
  download(work, std::getenv("MW1O_GAME"), std::getenv("MW1O_BTECH_REFERENCE"));
#else
  std::printf("skip the download: this build has no libarchive\n");
#endif
  fs::remove_all(work, ec);  // nothing of the game's kept
  std::printf("%s (%d failed)\n", g_failures ? "FAILED" : "passed", g_failures);
  return g_failures ? 1 : 0;
}
