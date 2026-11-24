#include "Setup.h"

#include "mechwarrior/combat_installation.h"

#include <rekit/archive.h>
#include <rekit/files.h>
#include <rekit/installshield.h>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>

namespace fs = std::filesystem;

namespace mw1o::setup {
namespace {

std::string lower(std::string s) {
  for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

// `dir` ending in a separator (this system's, so a path built on it reads as one, as SDL_GetPrefPath names it).
std::string with_slash(std::string dir) {
  if (!dir.empty() && dir.back() != '/' && dir.back() != '\\') dir += static_cast<char>(fs::path::preferred_separator);
  return dir;
}

std::string env(const char* name) {
  const char* v = std::getenv(name);
  return v ? v : "";
}

std::vector<uint8_t> read_file(const fs::path& p, size_t limit) {
  std::ifstream in(p, std::ios::binary);
  std::vector<uint8_t> out;
  if (!in) return out;
  out.resize(limit);
  in.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(limit));
  out.resize(static_cast<size_t>(in.gcount()));
  return out;
}

// A file of `dir` by name in any case; "" when absent.
std::string any_case(const std::string& dir, const std::string& name) { return rekit::any_case_path(dir, name); }

bool is_dir(const std::string& p) {
  std::error_code ec;
  return !p.empty() && fs::is_directory(p, ec);
}

// The folder's regular files' bytes, stopping once past `cap`.
bool larger_than(const std::string& dir, uint64_t cap) {
  std::error_code ec;
  uint64_t total = 0;
  for (auto it = fs::recursive_directory_iterator(dir, fs::directory_options::skip_permission_denied, ec);
       !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
    std::error_code e2;
    if (it->is_regular_file(e2)) total += it->file_size(e2);
    if (total > cap) return true;
  }
  return false;
}

// `entry` ('/'-separated, as an archive stores it) under the game folder `root` ("" or ending '/'): its parts
// below root, made safe; false when outside root or unsafe.
bool under_root(const std::string& entry, const std::string& root, std::string* rel) {
  std::string e = entry;
  std::replace(e.begin(), e.end(), '\\', '/');
  if (lower(e.substr(0, root.size())) != lower(root)) return false;
  *rel = rekit::safe_relative(e.substr(root.size()));
  return !rel->empty();
}

}  // namespace

std::string pref_folder(const char* organisation, const char* application) {
  std::string base;
#if defined(_WIN32)
  base = env("APPDATA");
#elif defined(__APPLE__)
  if (!env("HOME").empty()) base = env("HOME") + "/Library/Application Support";
#else
  base = env("XDG_DATA_HOME");
  if (base.empty() && !env("HOME").empty()) base = env("HOME") + "/.local/share";
#endif
  if (base.empty()) return "";
#if defined(_WIN32)
  return with_slash(base) + organisation + "\\" + application + "\\";
#else
  return with_slash(base) + organisation + "/" + application + "/";
#endif
}

std::string user_folder() {
  const std::string configured = env("MECHWARRIOR_SAVE_DIRECTORY");
  if (!configured.empty()) return with_slash(configured);
  const std::string folder = pref_folder(kOrganisation, kApplication);
  const std::string unmoved = unmoved_folder(folder);
  return unmoved.empty() ? folder : unmoved;
}

bool has_content(const std::string& dir) {
  std::error_code ec;
  if (!is_dir(dir)) return false;
  for (fs::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec))
    if (it->path().filename() != ".DS_Store") return true;
  return false;
}

std::string unmoved_folder(const std::string& folder) {
  const std::string old = folder.empty() ? "" : rekit::old_org_folder(folder);
  return !old.empty() && !has_content(folder) && has_content(old) ? with_slash(old) : "";
}

Settled settle_user_folder(const std::string& folder, const std::string& preview) {
  Settled s;
  s.folder = folder;
  if (has_content(folder)) return s.kind = Settled::kInUse, s;
  const std::string old = unmoved_folder(folder);
  if (!old.empty()) {
    s.from = old;
    std::string to = folder, from = old;  // without the separator: the rename's own names
    while (to.size() > 1 && (to.back() == '/' || to.back() == '\\')) to.pop_back();
    while (from.size() > 1 && (from.back() == '/' || from.back() == '\\')) from.pop_back();
    const rekit::FolderMove m = rekit::move_once(from, to);
    if (m.kind == rekit::FolderMove::kMoved) {
      rekit::remove_if_empty(fs::path(from).parent_path().string(), 1);  // MicroProse, if nothing else is in it
      return s.kind = Settled::kMoved, s;
    }
    s.kind = Settled::kMoveFailed, s.folder = old;
    s.error = m.error.empty() ? "nothing moved from " + from + " to " + to : m.error;
    return s;
  }
  const Carried c = carry_over(preview, folder);
  s.files = c.files, s.error = c.error;
  if (c.done) s.kind = Settled::kCarried, s.from = preview;
  return s;
}

std::string default_install(const std::string& user) {
  return user.empty() ? "" : (fs::path(user).lexically_normal() / "install").lexically_normal().string();
}

Carried carry_over(const std::string& from, const std::string& to) {
  Carried c;
  std::error_code ec;
  if (from.empty() || to.empty() || !is_dir(from)) return c;
  const fs::path dest(to);
  if (fs::exists(dest / kCarriedMarker, ec)) return c;
  if (fs::equivalent(from, to, ec)) return c;
  fs::create_directories(dest, ec);
  if (ec) return c.error = "Could not make " + to + ": " + ec.message(), c;
  for (auto it = fs::recursive_directory_iterator(from, fs::directory_options::skip_permission_denied, ec);
       !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
    std::error_code e2;
    if (it->is_symlink(e2) || !it->is_regular_file(e2)) continue;
    const fs::path rel = fs::relative(it->path(), from, e2);
    if (e2 || rel.empty()) continue;
    if (lower(rel.filename().string()) == "mechwarrior.log") continue;  // the old run's log, not the player's
    const fs::path out = dest / rel;
    if (fs::exists(out, e2)) continue;  // never overwritten
    fs::create_directories(out.parent_path(), e2);
    if (!fs::copy_file(it->path(), out, fs::copy_options::none, e2)) {
      if (c.error.empty()) c.error = "Could not copy " + it->path().string() + ": " + e2.message();
      continue;
    }
    ++c.files;
  }
  if (ec && c.error.empty()) c.error = "Could not read " + from + ": " + ec.message();
  // Written even after a partial copy: what is there stays, and the next start does not copy again over the
  // player's new files. The error is logged.
  std::ofstream(dest / kCarriedMarker) << "Saves and settings were copied once from " << from
                                       << " (left as it was) on the first start of this version.\n";
  c.done = true;
  return c;
}

std::string btech_revision(const std::vector<uint8_t>& bytes) {
  if (!BtechInstallationPackedBytes(bytes.size())) return "";
  uint8_t* image = nullptr;
  BtechRevision revision{};
  if (!BtechDecodeImage(bytes.data(), bytes.size(), &image, &revision)) return "";
  std::free(image);
  const char* name = BtechRevisionName(revision);
  return name ? name : "";
}

rekit::SourceRules rules() {
  rekit::SourceRules r;
  rekit::Clue btech;
  btech.kind = rekit::Clue::kInstall;
  btech.path = "btech.exe";
  btech.limit = 1u << 20;
  btech.judge = [](const std::vector<uint8_t>& bytes) {
    rekit::Identity id;
    const std::string v = btech_revision(bytes);
    if (v.empty()) {
      id.why = "BTECH.EXE (" + std::to_string(bytes.size()) + " bytes) is not a revision this program knows";
      return id;
    }
    id.game = kId, id.version = v, id.rank = 1;
    return id;
  };
  r.clues.push_back(std::move(btech));
  r.programs = {"btech.exe", "mw_main.exe"};
  return r;
}

std::string recognise(const std::string& path) try {
  // The other games this program is asked for by mistake, each told by a file of its own; found by the kit's walk.
  const auto other = [](const char* game, int rank) {
    rekit::Identity id;
    id.game = game, id.rank = rank;
    return id;
  };
  rekit::SourceRules r;
  const auto add = [&r](rekit::Clue::Kind kind, const char* file, std::function<rekit::Identity(const std::vector<uint8_t>&)> judge,
                        size_t limit = 1u << 20) { r.clues.push_back({kind, file, limit, std::move(judge)}); };
  // A BTECH.EXE, not a known revision (a known one is the game, and was found).
  add(rekit::Clue::kInstall, "btech.exe", [other](const std::vector<uint8_t>& b) {
    return btech_revision(b).empty() ? other("btech", 30) : rekit::Identity{};
  });
  // MechWarrior 3's and Pirate's Moon's installed data: the archive's last words name the version. A disc's
  // InstallShield header lists that file, Pirate's Moon's its own missions.
  const auto mw3 = [other](const std::vector<uint8_t>& b) {
    const auto word = [&b](size_t from_end) {
      const uint8_t* p = b.data() + b.size() - from_end;
      return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
    };
    if (b.size() < 12) return rekit::Identity{};
    if (word(8) == 1) return other("mw3", 20);
    if (word(12) == 2) return other("pm", 20);
    return rekit::Identity{};
  };
  add(rekit::Clue::kInstall, "zbd/reader.zbd", mw3, 64u << 20);
  add(rekit::Clue::kDisc, "data1.hdr", [other](const std::vector<uint8_t>& b) {
    std::vector<std::pair<std::string, uint64_t>> files;
    std::string error;
    if (!rekit::list_installshield(b, &files, &error)) return rekit::Identity{};
    bool reader = false, moon = false;
    for (const auto& [name, size] : files) {
      const std::string l = lower(name);
      reader |= l == "zbd/reader.zbd";
      moon |= l.find("readersa1.zbd") != std::string::npos;
    }
    return moon ? other("pm", 20) : reader ? other("mw3", 20) : rekit::Identity{};
  }, 8u << 20);
  // MechWarrior 2's project file (it starts with PROJ) and its database, in the game's folder, its disc's MECH2
  // folder or the disc image's top.
  const auto project = [other](const std::vector<uint8_t>& b) {
    return b.size() >= 4 && std::equal(b.begin(), b.begin() + 4, "PROJ") ? other("mw2", 10) : rekit::Identity{};
  };
  const auto database = [other](const std::vector<uint8_t>& b) { return b.empty() ? rekit::Identity{} : other("mw2", 10); };
  for (const char* folder : {"", "mech2/"}) {
    add(rekit::Clue::kInstall, folder == std::string() ? "mw2.prj" : "mech2/mw2.prj", project);
    add(rekit::Clue::kInstall, folder == std::string() ? "database.mw2" : "mech2/database.mw2", database);
  }
  r.programs = {"btech.exe", "mw_main.exe", "mech2.exe", "mw2.exe", "mech3.exe"};

  const auto stop = std::chrono::steady_clock::now() + std::chrono::seconds(kRecogniseSeconds);
  const rekit::Found f = rekit::find_source(path, r, [stop](double, const std::string&) { return std::chrono::steady_clock::now() < stop; });
  if (f.kind == rekit::Found::kNothing) return "";
  const std::string plays = std::string(" This program plays ") + kTitle + ", the 1989 game whose program is BTECH.EXE.";
  if (f.game == "btech")
    return "BTECH.EXE was found there, but it is a revision this program doesn't know. It knows two: the 1989 release and the one of 1990-01-22.";
  if (f.game == "mw2") return "That is MechWarrior 2, another game." + plays;
  if (f.game == "mw3") return "That is MechWarrior 3, another game." + plays;
  if (f.game == "pm") return "That is MechWarrior 3: Pirate's Moon, another game." + plays;
  return "";
} catch (...) {
  return "";
}

rekit::SearchHints hints() {
  rekit::SearchHints h;
  // The folder names a copy of the DOS game is kept under, alone and inside the usual Games, DOSBox and DOS
  // folders (as mech3-re's MW1 search names them). Each found folder is looked into two levels down (find_install).
  const char* const names[] = {"MechWarrior", "MechWarrior1989", "MechWarrior 1", "MW1", "MECHWAR", "MECH", "MW"};
  const char* const parents[] = {"", "Games/", "DOSBox/", "DOS/", "GOG Games/"};
  for (const char* p : parents)
    for (const char* n : names) h.other_folders.push_back(std::string(p) + n);
  h.drop_in = true;
  return h;
}

bool story_imported(const std::string& user) {
  std::error_code ec;
  return !user.empty() && fs::is_regular_file(fs::path(with_slash(user)) / "story" / "import.json", ec);
}

Check check_install(const std::string& dir, bool imported) {
  Check c;
  if (!is_dir(dir)) return c.why = "no such folder", c;
  const std::string btech = any_case(dir, "BTECH.EXE");
  if (btech.empty()) return c.why = "no BTECH.EXE", c;
  c.version = btech_revision(read_file(btech, 1u << 20));
  if (c.version.empty()) return c.why = "BTECH.EXE is not a revision this program knows", c;
  c.state = rekit::Status::kReady;
  if (!imported && any_case(dir, "MW_MAIN.EXE").empty()) {
    c.state = rekit::Status::kIncomplete;
    c.missing.push_back("MW_MAIN.EXE (the game's main program, read once for the story)");
  }
  return c;
}

std::string game_in(const std::string& dir, int depth) {
  if (!is_dir(dir)) return "";
  std::vector<std::string> level{dir};
  for (int d = 0; d <= depth && !level.empty(); ++d) {
    std::vector<std::string> next;
    for (const std::string& f : level) {
      if (!any_case(f, "BTECH.EXE").empty()) return f;
      if (d == depth) continue;
      std::error_code ec;
      std::vector<std::string> subs;
      for (fs::directory_iterator it(f, fs::directory_options::skip_permission_denied, ec), end; !ec && it != end;
           it.increment(ec)) {
        std::error_code e2;
        if (!it->is_directory(e2) || it->is_symlink(e2)) continue;
        try {
          subs.push_back(it->path().string());
        } catch (const std::exception&) {  // a name that isn't text (Windows): no game is looked for under it
        }
      }
      std::sort(subs.begin(), subs.end());
      next.insert(next.end(), subs.begin(), subs.end());
    }
    level.swap(next);
  }
  return "";
}

std::string find_install(const std::string& remembered, const std::string& user, const std::string& program_dir,
                         bool* searched) {
  if (searched) *searched = false;
  const bool imported = story_imported(user);
  auto usable = [imported](const std::string& d) {
    return !d.empty() && check_install(d, imported).state != rekit::Status::kMissing;
  };
  if (usable(remembered)) return remembered;
  const std::string own = default_install(user);
  if (usable(own)) return own;
  for (const std::string& place : rekit::install_candidates(hints(), program_dir)) {
    const std::string g = game_in(place, 2);
    if (usable(g)) {
      if (searched) *searched = true;
      return g;
    }
  }
  return "";
}

std::string find_other_install(const std::vector<std::string>& not_these, const std::string& user, const std::string& program_dir) {
  const bool imported = story_imported(user);
  for (const std::string& place : rekit::install_candidates(hints(), program_dir)) {
    const std::string g = game_in(place, 2);
    if (g.empty() || check_install(g, imported).state == rekit::Status::kMissing) continue;
    const bool known = std::any_of(not_these.begin(), not_these.end(), [&g](const std::string& d) {
      std::error_code ec;
      return !d.empty() && fs::equivalent(g, d, ec);
    });
    if (!known) return g;
  }
  return "";
}

std::string remembered_folder(const std::string& user) {
  if (user.empty()) return "";
  std::ifstream in(fs::path(with_slash(user)) / "MechWarrior.ini");
  std::string line, found;
  while (std::getline(in, line)) {
    const size_t start = line.find_first_not_of(" \t");
    if (start == std::string::npos || line.compare(start, 11, "game_folder") != 0) continue;
    size_t eq = line.find_first_not_of(" \t", start + 11);
    if (eq == std::string::npos || line[eq] != '=') continue;
    std::string v = line.substr(eq + 1);
    v.erase(0, v.find_first_not_of(" \t"));
    while (!v.empty() && std::isspace(static_cast<unsigned char>(v.back()))) v.pop_back();
    found = v;
  }
  return found;
}

rekit::Status status(const std::string& remembered, const std::string& user, const std::string& program_dir,
                     const std::string& version) {
  rekit::Status st;
  st.game = kId, st.version = version;
  if (user.empty()) st.note = "the player's folder cannot be found";
  bool searched = false;
  const std::string found = find_install(remembered, user, program_dir, &searched);
  const bool imported = story_imported(user);
  if (!found.empty()) {
    const Check c = check_install(found, imported);
    st.where = found, st.data = c.version, st.state = c.state, st.missing = c.missing;
    if (searched) st.note = "found by the search, not set up yet";
  }
  if (!imported) st.pending.push_back("story import");
  return st;
}

Installed install(const rekit::Found& found, const std::string& out_dir, const Progress& progress) {
  Installed r;
  auto fail = [&r](const std::string& why) {
    r.error = why;
    return r;
  };
  if (found.kind != rekit::Found::kInstall || found.game != kId)
    return fail("That is not " + std::string(kTitle) + ": nothing was copied.");
  if (out_dir.empty()) return fail("There is no folder to copy the game into.");
  std::error_code ec;
  const fs::path dest(out_dir), tmp(out_dir + ".new");
  // tmp is ours only when it carries the marker (an earlier run's, stopped); else it is the player's, and never
  // deleted. It is made and marked before anything goes in it, so a run killed at any point leaves one that is ours.
  if (fs::exists(tmp, ec) && !fs::exists(tmp / kInstallMarker, ec) && !(fs::is_directory(tmp, ec) && fs::is_empty(tmp, ec)))
    return fail("The folder " + tmp.string() + " is in the way, and nothing in it was touched. Move that folder somewhere "
                "else, or delete it if you don't need it, then set the game up again.");
  const auto stage = [&](const std::string& from) -> std::string {  // "" or why not
    std::error_code e;
    if (fs::exists(tmp / kInstallMarker, e)) fs::remove_all(tmp, e);
    fs::create_directories(tmp, e);
    if (e) return "Could not make " + tmp.string() + ": " + e.message() + ".";
    std::ofstream mark(tmp / kInstallMarker);
    mark << from << std::flush;
    return mark ? "" : "Could not write in " + tmp.string() + ".";
  };
  fs::path ready = tmp;  // what is swapped in
  fs::create_directories(dest.parent_path(), ec);

  if (found.on_disk && found.steps.empty()) {
    // An installed copy in a folder: copied, never moved.
    const std::string from = (fs::path(found.path) / found.root).lexically_normal().string();
    const std::string src = is_dir(from) ? from : fs::path(from).parent_path().string();
    if (fs::exists(dest, ec) && fs::equivalent(src, dest, ec)) return r.ok = true, r.dir = dest.string(), r;
    const fs::path real = fs::weakly_canonical(src, ec), staged = fs::weakly_canonical(tmp, ec);
    if (std::mismatch(staged.begin(), staged.end(), real.begin(), real.end()).first == staged.end())
      return fail("The folder " + src + " is a copy this setup didn't finish: pick your own copy.");
    if (larger_than(src, kMaxGameBytes))
      return fail("The folder " + src + " holds far more than " + std::string(kTitle) +
                  ", which is about 1 MB: nothing was copied. Put the game's files in a folder of their own and pick "
                  "that.");
    // Copied into a folder inside the marked one (the kit copies only into a new one).
    const std::string from_words = "Copied from " + src + " by the setup (the original was left as it was).\n";
    if (const std::string why = stage(from_words); !why.empty()) return fail(why);
    ready = tmp / "copy";
    const rekit::CopyResult c = rekit::copy_folder(src, ready.string(), [&](uint64_t done, uint64_t total, const std::string& f) {
      return !progress || progress(total ? double(done) / double(total) : 0.0, f);
    });
    if (!c.ok) {
      fs::remove_all(tmp, ec);  // ours: marked above
      r.cancelled = c.cancelled;
      return fail(c.cancelled ? "Stopped. Nothing was copied." : "Could not copy the game from " + src + ": " + c.error);
    }
    r.files = c.files, r.bytes = c.bytes;
    std::ofstream(ready / kInstallMarker) << from_words;
  } else {
    // In an archive (at any depth): the game's folder read out in one forward pass.
    if (!rekit::archives_supported()) return fail("This build cannot open archives: unpack it and pick its BTECH.EXE.");
    std::vector<std::unique_ptr<rekit::ArchiveReader>> chain;
    chain.push_back(std::make_unique<rekit::ArchiveReader>());
    if (!chain.back()->open_file(found.path)) return fail("Could not open " + found.path + " (" + chain.back()->error() + ").");
    rekit::ArchiveReader::Entry e;
    for (const rekit::Step& s : found.steps) {
      if (s.kind != rekit::Step::kArchive) return fail("The game is inside a disc image: that is not how MechWarrior 1 came.");
      bool at = false;
      while (chain.back()->next(&e))
        if (e.path == s.entry) {
          at = true;
          break;
        }
      if (!at) return fail("Could not find " + s.entry + " again in " + found.path + ".");
      auto inner = std::make_unique<rekit::ArchiveReader>();
      if (!inner->open_stream(chain.back()->entry_stream())) return fail("Could not open " + s.entry + " (" + inner->error() + ").");
      chain.push_back(std::move(inner));
    }
    rekit::ArchiveReader& a = *chain.back();
    if (const std::string why = stage("Unpacked from " + found.path + " by the setup (the archive was left as it was).\n"); !why.empty())
      return fail(why);
    std::vector<uint8_t> buf(1 << 16);
    std::string rel, why;
    const int64_t whole = [&] {
      std::error_code e3;
      const auto n = fs::file_size(found.path, e3);
      return e3 ? int64_t(0) : static_cast<int64_t>(n);
    }();
    while (why.empty() && a.next(&e)) {
      if (e.other || !under_root(e.path, found.root, &rel)) continue;
      if (lower(fs::path(rel).filename().string()) == kInstallMarker) continue;
      const fs::path out = tmp / fs::path(rel);
      if (e.dir) {
        fs::create_directories(out, ec);
        continue;
      }
      fs::create_directories(out.parent_path(), ec);
      std::ofstream f(out, std::ios::binary);
      if (!f) why = "Could not write " + out.string() + ".";
      for (int64_t n; why.empty() && (n = a.read(buf.data(), buf.size())) != 0;) {
        if (n < 0) why = "That archive is damaged (" + a.error() + ").";
        else if ((r.bytes += uint64_t(n)) > kMaxGameBytes) why = "That archive holds far more than the game: nothing was copied.";
        else f.write(reinterpret_cast<const char*>(buf.data()), n);
      }
      if (why.empty() && !f) why = "Could not write " + out.string() + ".";
      ++r.files;
      if (why.empty() && progress && !progress(whole ? double(chain.front()->position()) / double(whole) : 0.0, rel))
        r.cancelled = true, why = "Stopped. Nothing was copied.";
    }
    if (why.empty() && !a.error().empty()) why = "That archive is damaged (" + a.error() + ").";
    if (!why.empty()) {
      fs::remove_all(tmp, ec);  // ours: made above with the marker
      return fail(why);
    }
  }
  if (check_install(ready.string(), true).state == rekit::Status::kMissing) {
    fs::remove_all(tmp, ec);
    return fail("The copy has no BTECH.EXE this program knows: nothing was changed.");
  }
  if (!rekit::swap_in_folder(ready.string(), dest.string(), kInstallMarker, ec)) {
    const std::string why = ec.message();
    fs::remove_all(tmp, ec);
    return fail("Could not put the game in " + dest.string() + " (" + why + "): nothing was changed.");
  }
  fs::remove_all(tmp, ec);  // the marked folder the copy was made in
  r.ok = true, r.dir = dest.string();
  return r;
}

}  // namespace mw1o::setup
