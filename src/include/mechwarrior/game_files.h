#ifndef MECHWARRIOR_GAME_FILES_H
#define MECHWARRIOR_GAME_FILES_H
// The game's files at play: the player's installation (read only) and the save folder. It replaces the INT 21h
// handle calls; nothing here is game state. The rules play can see are named below and tested (tests/game_files_test.cpp).
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mw1o::files {

enum class FileError { kNone, kNotFound, kDenied, kNoRoom };

// An open file; closed when it goes out of scope.
class GameFile {
 public:
  GameFile() = default;
  explicit GameFile(std::FILE *file) : file_(file) {}
  explicit GameFile(FileError error) : error_(error) {}
  explicit operator bool() const { return file_ != nullptr; }
  FileError error() const { return error_; }

  // The bytes read, or nothing if the read failed. A short read is a success. The destination is checked
  // (mwHostValidateFileDestination) after the real read and before the copy, for the bytes really read.
  std::optional<std::uint16_t> read(std::uint8_t *buffer, std::uint16_t byte_count);
  bool seek(std::uint32_t offset);
  // True if every byte reached the file system: DOS wrote straight through, so the write is flushed here
  // and a full disk is seen by the caller, not at a close whose result play ignores.
  bool write(const std::uint8_t *buffer, std::uint16_t byte_count);
  bool close();

 private:
  struct Closer {
    void operator()(std::FILE *file) const { std::fclose(file); }
  };
  std::unique_ptr<std::FILE, Closer> file_;
  FileError error_ = FileError::kNone;
};

void set_game_directory(const std::string &directory);
void set_save_directory(const std::string &directory);

// DIRECTORY/NAME for a flat DOS file name: the exact name if it exists, else the one entry whose name matches
// ignoring case (an install copied with lower-case names on a case-sensitive file system), else the exact name.
// The guard (docs/phase2/PLAN.md 8.4, decision 5): a program's name (.EXE or .COM, any case) stops the program,
// naming the file and `caller`. Only the import reads the game's programs, and it never comes here.
std::filesystem::path find_game_file(const std::filesystem::path &directory, const std::string &name, const char *caller);

// Opens read only: a .GAM name from the save folder (no separators allowed), any other from the installation.
// A directory is refused. The original opened read-write (INT 21h/3D, AL=02), so a read-only installation failed.
GameFile open_game_file(const std::string &name);
// Creates a save, truncating an existing one (INT 21h/3C). No separators in the name.
GameFile create_save(const std::string &name);
// The stem of every save in the save folder, upper case, in the file system's own order (never sorted, as the
// original listed them). Only 8.3 names ending in .GAM; on Windows, no hidden or system file.
std::vector<std::string> list_saves();

}  // namespace mw1o::files

// Host allocation check after a real read, before the copy (SDLBackend_Main.cpp sets it).
extern void (*mwHostValidateFileDestination)(const std::uint8_t *buffer, std::uint16_t bytesRead);

#endif
