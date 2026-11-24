// The game's files at play (B6, src/Support/GameFiles.cpp): each rule play can see, on files this test makes.
// No game files. Plain checks, no framework: exit 0 when all hold.
//
//     game_files_test <work folder>
//     game_files_test --open-program <name>    (run by the test itself: the guard stops the program)
#include "mechwarrior/game_files.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <string>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace fs = std::filesystem;
using namespace mw1o::files;

namespace {

int failures = 0;
void Check(bool ok, const std::string &what) {
  if (!ok) {
    std::printf("FAIL %s\n", what.c_str());
    ++failures;
  }
}

void Put(const fs::path &path, const std::string &text) { std::ofstream(path, std::ios::binary) << text; }
std::string Get(const fs::path &path) {
  std::ifstream in(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
std::string Contents(const std::string &name) {
  GameFile file = open_game_file(name);
  std::uint8_t bytes[64];
  const auto count = file.read(bytes, sizeof bytes);
  return count ? std::string(reinterpret_cast<char *>(bytes), *count) : std::string("(not read)");
}

int validated = 0;
void Validate(const std::uint8_t *, std::uint16_t count) { validated += count; }

void Names(const fs::path &game) {
  Put(game / "EXACT.DAT", "exact");
  Check(Contents("EXACT.DAT") == "exact", "the exact name");
  Put(game / "Mixed.Dat", "mixed");
  Check(Contents("MIXED.DAT") == "mixed", "else the entry matching but for case");
  GameFile missing = open_game_file("NONE.DAT");
  Check(!missing && missing.error() == FileError::kNotFound, "a missing file is not found");
#if !defined(_WIN32)
  // Only a case-sensitive file system can hold both names.
  Put(game / "BOTH.DAT", "upper");
  Put(game / "both.dat", "lower");
  if (Get(game / "BOTH.DAT") == "upper") Check(Contents("BOTH.DAT") == "upper", "the exact name before one differing in case");
#endif
  fs::create_directories(game / "FOLDER.DAT");
  GameFile folder = open_game_file("FOLDER.DAT");
  Check(!folder && folder.error() == FileError::kDenied, "a folder is refused");
}

void Reads(const fs::path &game) {
  Put(game / "TEN.DAT", "0123456789");
  std::uint8_t bytes[16] = {};
  GameFile file = open_game_file("TEN.DAT");
  mwHostValidateFileDestination = Validate;
  Check(file.read(bytes, 16) == 10, "a short read is a success with its count");
  Check(validated == 10, "the destination is checked for the bytes really read");
  Check(file.read(bytes, 16) == 0, "a read at the end is a success of 0 bytes");
  Check(validated == 10, "and checks nothing");
  mwHostValidateFileDestination = nullptr;
  Check(file.seek(4) && file.read(bytes, 2) == 2 && bytes[0] == '4', "a seek is from the start");
  Check(file.close(), "the close");
  Check(!file && !file.read(bytes, 1) && !file.seek(0) && !file.close(), "a closed file does nothing");
  for (int i = 0; i < 400; ++i) (void)open_game_file("TEN.DAT");
  Check(bool(open_game_file("TEN.DAT")), "a file is closed when it goes out of scope");
}

void ReadOnly(const fs::path &game) {
  const fs::path path = game / "LOCKED.DAT";
  Put(path, "locked");
  fs::permissions(path, fs::perms::owner_write | fs::perms::group_write | fs::perms::others_write, fs::perm_options::remove);
  if (std::FILE *writable = std::fopen(path.string().c_str(), "r+b")) {
    std::fclose(writable);
    std::printf("note: this user can write a read-only file; the read-only open is not proved here\n");
  }
  Check(Contents("LOCKED.DAT") == "locked", "a read-only file opens");
  fs::permissions(path, fs::perms::owner_write, fs::perm_options::add);
}

void Saves(const fs::path &game, const fs::path &saves) {
  Check(!create_save("ONE.GAM"), "no save folder: no save");
  Check(list_saves().empty(), "and no list");
  set_save_directory(saves.string());

  for (const char *name : {"a/b.GAM", "a\\b.GAM", "c:b.GAM"}) {
    GameFile made = create_save(name), opened = open_game_file(name);
    Check(!made && made.error() == FileError::kDenied && !opened && opened.error() == FileError::kDenied,
          std::string("a separator in a save's name is denied: ") + name);
  }
  Put(game / "GAMEONLY.GAM", "installation");
  Check(!open_game_file("GAMEONLY.GAM"), "a .GAM opens from the save folder alone");

  Put(saves / "OLD.GAM", "a longer old save");
  {
    GameFile file = create_save("OLD.GAM");
    Check(bool(file) && fs::file_size(saves / "OLD.GAM") == 0, "creating truncates the old save at once");
    const std::uint8_t bytes[3] = {'n', 'e', 'w'};
    Check(file.write(bytes, 3), "the write");
    Check(Get(saves / "OLD.GAM") == "new", "which has reached the file before the close");
  }

  for (const char *name : {"lower.gam", "EIGHTCHR.GAM", "NINECHARS.GAM", "TWO.DOT.GAM", "SP ACE.GAM", "NOEXT", "OTHER.SAV", ".GAM"})
    Put(saves / name, "x");
  fs::create_directories(saves / "DIR.GAM");
  const std::vector<std::string> listed = list_saves();
  Check(std::set<std::string>(listed.begin(), listed.end()) == std::set<std::string>({"OLD", "LOWER", "EIGHTCHR"}) && listed.size() == 3,
        "only 8.3 .GAM files are listed, by upper-case stem");
  std::vector<std::string> order;
  for (const auto &entry : fs::directory_iterator(saves)) {
    std::string stem = entry.path().stem().string();
    for (char &c : stem) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (std::find(listed.begin(), listed.end(), stem) != listed.end() && entry.is_regular_file() &&
        entry.path().extension() != ".SAV")
      order.push_back(stem);
  }
  Check(listed == order, "in the file system's own order");
#if defined(_WIN32)
  for (const DWORD attribute : {DWORD(FILE_ATTRIBUTE_HIDDEN), DWORD(FILE_ATTRIBUTE_SYSTEM)}) {
    Check(SetFileAttributesW((saves / "EIGHTCHR.GAM").c_str(), attribute) != 0, "the attribute is set");
    const std::vector<std::string> left = list_saves();
    Check(left.size() == 2 && std::find(left.begin(), left.end(), "EIGHTCHR") == left.end(), "a hidden or system save is not listed");
  }
  SetFileAttributesW((saves / "EIGHTCHR.GAM").c_str(), FILE_ATTRIBUTE_NORMAL);
#endif
  Check(Contents("LOWER.GAM") == "x", "a listed lower-case save opens by its listed name");
}

// A program's name stops the program, so each is tried in a child.
void Guard(const std::string &self) {
  for (const char *name : {"BTECH.EXE", "btech.exe", "Any.Com"}) {
#if defined(_WIN32)
    const std::string command = "\"\"" + self + "\" --open-program " + name + " 2>NUL\"";
#else
    const std::string command = "\"" + self + "\" --open-program " + name + " 2>/dev/null";
#endif
    Check(std::system(command.c_str()) != 0, std::string("play may not open a program: ") + name);
  }
  const std::string command =
#if defined(_WIN32)
      "\"\"" + self + "\" --open-program EXACT.DAT\"";
#else
      "\"" + self + "\" --open-program EXACT.DAT";
#endif
  Check(std::system(command.c_str()) == 0, "and the child that opens no program goes on");
}

}  // namespace

int main(int argc, char **argv) {
  if (argc == 3 && std::string(argv[1]) == "--open-program") {
    (void)find_game_file(".", argv[2], "game_files_test");
    return 0;
  }
  if (argc != 2) {
    std::fprintf(stderr, "usage: game_files_test <work folder>\n");
    return 2;
  }
  const fs::path work = argv[1], game = work / "game", saves = work / "saves";
  std::error_code ec;
  fs::remove_all(work, ec);
  fs::create_directories(game);
  fs::create_directories(saves);
  set_game_directory(game.string());
  Names(game);
  Reads(game);
  ReadOnly(game);
  Saves(game, saves);
  Guard(argv[0]);
  fs::remove_all(work, ec);
  if (failures) std::printf("%d failed\n", failures);
  else std::printf("game files: all hold\n");
  return failures ? 1 : 0;
}
