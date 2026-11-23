// MechWarrior 1's setup recipe (src/Setup/Setup.h) over re-setup-kit. The game-free parts always run: the
// player's folder's name, the family folder's order (in use, moved from MicroProse, a failed move, the
// previews' copied), the carry-over (never overwriting, once only), MechWarrior.ini's game_folder, the
// verdicts on folders that are not the game, an install refusing a folder that is not ours, --status's answer
// writing nothing. With the player's files (MW1O_GAME, MW1O_BTECH_REFERENCE, MW1O_BTECH_1990) the game parts run
// too: each revision judged, an install from a folder and from a zip (built here, in the work folder, never kept),
// an entry climbing out of the archive left out. Prints names and counts only, never the game's bytes.
//
//     setup_test <work folder>
#include "Setup/Setup.h"

#include <rekit/archive.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#if REKIT_HAVE_LIBARCHIVE
#include <archive.h>
#include <archive_entry.h>
#endif

namespace fs = std::filesystem;
namespace setup = mw1o::setup;

namespace {

int g_failures = 0;
void check(bool ok, const std::string& what) {
  std::printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
  if (!ok) ++g_failures;
}

void write(const fs::path& p, const std::string& text) {
  fs::create_directories(p.parent_path());
  std::ofstream(p, std::ios::binary) << text;
}
std::string read(const fs::path& p) {
  std::ifstream in(p, std::ios::binary);
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
size_t count_files(const fs::path& d) {
  std::error_code ec;
  size_t n = 0;
  for (auto it = fs::recursive_directory_iterator(d, ec); !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) ++n;
  return n;
}

#if REKIT_HAVE_LIBARCHIVE
// A zip of `files` (name in the archive, path on disk; an empty path writes a short text entry).
bool make_zip(const fs::path& out, const std::vector<std::pair<std::string, fs::path>>& files) {
  archive* a = archive_write_new();
  archive_write_set_format_zip(a);
  if (archive_write_open_filename(a, out.string().c_str()) != ARCHIVE_OK) return archive_write_free(a), false;
  for (const auto& [name, path] : files) {
    const std::string data = path.empty() ? std::string("not the game\n") : read(path);
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
#endif

// The family folder's order on each start (setup::settle_user_folder), each branch on folders of its own.
void family_folder(const fs::path& base) {
  std::error_code ec;
  const fs::path bt = base / "BattleTech" / setup::kApplication, mp = base / "MicroProse" / setup::kApplication;
  const fs::path preview = base / setup::kOldOrganisation / setup::kOldApplication;
  const std::string folder = (bt / "").string(), old = (mp / "").string();  // ending in a separator, as SDL names them
  const auto fresh = [&] { fs::remove_all(base, ec); fs::create_directories(base); };

  // 1. The BattleTech folder in use: used as it is, a MicroProse one beside it left alone.
  fresh();
  write(bt / "saves" / "A.SAV", "battletech");
  write(mp / "saves" / "A.SAV", "microprose");
  setup::Settled s = setup::settle_user_folder(folder, preview.string());
  check(s.kind == setup::Settled::kInUse && s.folder == folder && read(bt / "saves" / "A.SAV") == "battletech" &&
            read(mp / "saves" / "A.SAV") == "microprose" && setup::unmoved_folder(folder).empty(),
        "family: a BattleTech folder in use is used, never merged with MicroProse's");

  // 2. Only MicroProse's: renamed across once (an empty BattleTech one, as SDL makes it, replaced), MicroProse gone.
  fresh();
  write(mp / "saves" / "A.SAV", "microprose");
  write(mp / "MechWarrior.ini", "game_folder=/x\n");
  write(preview / "saves" / "B.SAV", "preview");
  fs::create_directories(bt);
  check(setup::unmoved_folder(folder) == old, "family: before the move, the MicroProse folder is the one in use");
  s = setup::settle_user_folder(folder, preview.string());
  check(s.kind == setup::Settled::kMoved && s.folder == folder && s.from == old, "family: MicroProse's folder moved");
  check(read(bt / "saves" / "A.SAV") == "microprose" && fs::exists(bt / "MechWarrior.ini") && count_files(bt) == 3,
        "family: everything moved, nothing of the previews' copied in");
  check(!fs::exists(mp) && !fs::exists(base / "MicroProse"), "family: the emptied MicroProse folder removed");
  check(setup::unmoved_folder(folder).empty() &&
            setup::settle_user_folder(folder, preview.string()).kind == setup::Settled::kInUse,
        "family: the next start uses the moved folder");

  // 2, with another game in MicroProse: moved, MicroProse kept.
  fresh();
  write(mp / "saves" / "A.SAV", "microprose");
  write(base / "MicroProse" / "Another Game" / "x.cfg", "another game's");
  s = setup::settle_user_folder(folder, preview.string());
  check(s.kind == setup::Settled::kMoved && read(base / "MicroProse" / "Another Game" / "x.cfg") == "another game's",
        "break: MicroProse kept while another game's folder is in it");

  // 2, the move failing (the BattleTech name taken by a file): MicroProse's in use this start, tried again next.
  fresh();
  write(mp / "saves" / "A.SAV", "microprose");
  write(base / "BattleTech", "not a folder");
  s = setup::settle_user_folder(folder, preview.string());
  check(s.kind == setup::Settled::kMoveFailed && s.folder == old && !s.error.empty() &&
            read(mp / "saves" / "A.SAV") == "microprose" && setup::unmoved_folder(folder) == old,
        "break: a failed move keeps the MicroProse folder whole and in use");
  fs::remove(base / "BattleTech", ec);
  s = setup::settle_user_folder(folder, preview.string());
  check(s.kind == setup::Settled::kMoved && read(bt / "saves" / "A.SAV") == "microprose",
        "family: the next start tries the move again");

  // 3. Neither: the previews' folder copied in once.
  fresh();
  write(preview / "saves" / "B.SAV", "preview");
  s = setup::settle_user_folder(folder, preview.string());
  check(s.kind == setup::Settled::kCarried && s.files == 1 && read(bt / "saves" / "B.SAV") == "preview" &&
            fs::exists(bt / setup::kCarriedMarker) && read(preview / "saves" / "B.SAV") == "preview",
        "family: no BattleTech or MicroProse folder: the previews' copied in once, left as it was");

  // Nothing anywhere: nothing made.
  fresh();
  s = setup::settle_user_folder(folder, preview.string());
  check(s.kind == setup::Settled::kNew && !fs::exists(bt) && s.error.empty(), "family: nothing to settle: nothing made");
  fs::remove_all(base, ec);
}

void game_free(const fs::path& work) {
  // The player's folder's name, without making it.
  const std::string pf = setup::pref_folder(setup::kOrganisation, setup::kApplication);
  check(pf.find("BattleTech") != std::string::npos && pf.find("MicroProse") == std::string::npos &&
            pf.find("MechWarrior 1 Rebuild") != std::string::npos,
        "the player's folder is BattleTech/MechWarrior 1 Rebuild");

  // The carry-over: copies, never overwrites, once only, the old folder left as it was.
  const fs::path old = work / "old", now = work / "new";
  write(old / "MechWarrior.ini", "game_folder=/old/place\n");
  write(old / "story" / "import.json", "{}");
  write(old / "saves" / "A.SAV", "old save");
  write(old / "mechwarrior.log", "an old log");
  write(now / "saves" / "A.SAV", "the player's newer save");
  setup::Carried c = setup::carry_over(old.string(), now.string());
  check(c.done && c.files == 2 && c.error.empty(), "carry-over copies the two files the new folder lacks");
  check(read(now / "saves" / "A.SAV") == "the player's newer save", "break: carry-over never overwrites a file");
  check(!fs::exists(now / "mechwarrior.log"), "carry-over leaves the old log behind");
  check(read(old / "saves" / "A.SAV") == "old save" && fs::exists(old / "MechWarrior.ini"), "the old folder is left as it was");
  fs::remove(now / "MechWarrior.ini");
  c = setup::carry_over(old.string(), now.string());
  check(!c.done && !fs::exists(now / "MechWarrior.ini"), "break: a second carry-over copies nothing (the marker)");
  check(!setup::carry_over((work / "absent").string(), (work / "new2").string()).done && !fs::exists(work / "new2"),
        "no old folder: nothing made");

  family_folder(work / "family");

  // MechWarrior.ini's game_folder.
  write(work / "ini" / "MechWarrior.ini", "; a comment\ncombat_speed=2\n  game_folder = /some/where  \n");
  check(setup::remembered_folder((work / "ini").string()) == "/some/where", "game_folder read from MechWarrior.ini");
  check(setup::remembered_folder((work / "absent").string()).empty(), "no MechWarrior.ini: no game_folder");

  // Folders that are not the game.
  write(work / "notgame" / "README.TXT", "x");
  write(work / "fake" / "BTECH.EXE", std::string(1000, 'x'));
  check(setup::check_install((work / "notgame").string(), false).state == rekit::Status::kMissing, "a folder without BTECH.EXE: missing");
  check(setup::check_install((work / "fake").string(), false).state == rekit::Status::kMissing, "break: a BTECH.EXE of another size: missing");
  check(setup::btech_revision(std::vector<uint8_t>(174267, 0)).empty(), "break: zeros of BTECH's size are not a revision");

  // --status's answer with nothing set up: missing, the story pending, and nothing written.
  const fs::path user = work / "status-user";
  const rekit::Status st = setup::status("", user.string(), (work / "notgame").string(), "test");
  check(st.game == "mech1" && st.state == rekit::Status::kMissing, "status: mech1, missing");
  check(st.pending.size() == 1 && st.pending[0] == "story import", "status: the story import pending");
  check(!fs::exists(user), "break: status writes nothing (the player's folder not made)");
  check(rekit::status_json(st).find("\"game\": \"mech1\"") != std::string::npos || rekit::status_json(st).find("\"game\":\"mech1\"") != std::string::npos,
        "status JSON names mech1");
}

void with_game(const fs::path& work, const char* game_env, const char* ref_env, const char* r1990_env) {
  const fs::path game = game_env, ref = ref_env, r1990 = r1990_env;
  const fs::path main_exe = rekit::any_case_path(game.string(), "MW_MAIN.EXE");
  check(!main_exe.empty(), "MW1O_GAME holds MW_MAIN.EXE");
  auto bytes = [](const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
  };
  const std::string v_ref = setup::btech_revision(bytes(ref)), v_1990 = setup::btech_revision(bytes(r1990));
  check(!v_ref.empty() && !v_1990.empty() && v_ref != v_1990, "both revisions judged, each its own (" + v_ref + ", " + v_1990 + ")");

  // A copy in a folder, a level down inside the folder picked: found, installed (copied), the original kept.
  const fs::path picked = work / "picked", copy = picked / "MECHWAR";
  fs::create_directories(copy);
  fs::copy_file(r1990, copy / "BTECH.EXE");
  fs::copy_file(main_exe, copy / "MW_MAIN.EXE");
  write(copy / "SAVED.DAT", "x");
  check(setup::game_in(picked.string()) == copy.string(), "game_in finds the copy a level down");
  rekit::Found f = rekit::find_source(copy.string(), setup::rules());
  check(f.kind == rekit::Found::kInstall && f.game == "mech1" && f.version == v_1990, "find_source: the folder is mech1 " + v_1990);
  const rekit::Found by_program = rekit::find_source((copy / "MW_MAIN.EXE").string(), setup::rules());
  check(by_program.kind == rekit::Found::kInstall, "a pick of MW_MAIN.EXE stands for its folder");
  const fs::path out = work / "user" / "install";
  setup::Installed r = setup::install(f, out.string());
  check(r.ok && setup::check_install(out.string(), false).state == rekit::Status::kReady, "installed from a folder: ready");
  check(fs::exists(copy / "BTECH.EXE") && fs::exists(copy / "SAVED.DAT"), "the player's folder is left as it was");
  check(fs::exists(out / setup::kInstallMarker), "our install carries its marker");

  // Installing again replaces our own install.
  r = setup::install(f, out.string());
  check(r.ok, "installing again replaces our own install");

  // Break: a folder that is not ours where the install goes is refused and left alone.
  const fs::path theirs = work / "theirs";
  write(theirs / "LETTER.TXT", "the player's");
  r = setup::install(f, theirs.string());
  check(!r.ok && read(theirs / "LETTER.TXT") == "the player's" && !fs::exists(theirs.string() + ".new"),
        "break: a folder that is not ours is never replaced");
  write(work / "blocked.new" / "X", "the player's");
  r = setup::install(f, (work / "blocked").string());
  check(!r.ok && fs::exists(work / "blocked.new" / "X"), "break: a .new folder that is not ours is left alone");

  // Incomplete without MW_MAIN.EXE until the story is imported.
  fs::remove(rekit::any_case_path(out.string(), "MW_MAIN.EXE"));
  check(setup::check_install(out.string(), false).state == rekit::Status::kIncomplete, "no MW_MAIN.EXE before the import: incomplete");
  check(setup::check_install(out.string(), true).state == rekit::Status::kReady, "no MW_MAIN.EXE after the import: ready");

  // status: the player's own install found, ready, nothing written beyond what is there.
  const size_t before = count_files(work / "user");
  const rekit::Status st = setup::status("", (work / "user").string(), (work / "notgame").string(), "test");
  check(st.state == rekit::Status::kIncomplete && fs::path(st.where).lexically_normal() == out.lexically_normal(), "status: the player's install found (incomplete without MW_MAIN.EXE)");
  check(count_files(work / "user") == before, "break: status wrote nothing in the player's folder");

#if REKIT_HAVE_LIBARCHIVE
  // A download: the game a folder down in a zip, with an entry trying to climb out.
  const fs::path zip = work / "MechWarrior_1989.zip";
  check(make_zip(zip, {{"mechwarrior/MECHWAR/BTECH.EXE", ref},
                       {"mechwarrior/MECHWAR/MW_MAIN.EXE", main_exe},
                       {"mechwarrior/MECHWAR/../../ESCAPE.TXT", {}},
                       {"mechwarrior/OTHER.TXT", {}}}),
        "a zip built in the work folder");
  f = rekit::find_source(zip.string(), setup::rules());
  check(f.kind == rekit::Found::kInstall && f.version == v_ref && f.root == "mechwarrior/MECHWAR/", "find_source: the zip holds mech1 " + v_ref);
  const fs::path zout = work / "zuser" / "install";
  r = setup::install(f, zout.string());
  check(r.ok && setup::check_install(zout.string(), false).state == rekit::Status::kReady, "installed from a zip: ready");
  check(!fs::exists(work / "ESCAPE.TXT") && !fs::exists(work / "zuser" / "ESCAPE.TXT") && !fs::exists(zout / "OTHER.TXT"),
        "break: nothing outside the game's folder in the zip is written");
  check(fs::exists(zip), "the zip is left as it was");
  // Break: a zip with a BTECH.EXE of no known revision.
  const fs::path bad = work / "bad.zip";
  make_zip(bad, {{"MECHWAR/BTECH.EXE", {}}});
  check(rekit::find_source(bad.string(), setup::rules()).kind == rekit::Found::kNothing, "break: a zip whose BTECH.EXE is not the game");
#else
  std::printf("skip archives: this build has no libarchive\n");
#endif
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) return std::fprintf(stderr, "usage: setup_test <work folder>\n"), 2;
  const fs::path work = argv[1];
  std::error_code ec;
  fs::remove_all(work, ec);  // the test's own folder (the build dir's setup_work)
  fs::create_directories(work);
  game_free(work / "free");
  const char* game = std::getenv("MW1O_GAME");
  const char* ref = std::getenv("MW1O_BTECH_REFERENCE");
  const char* r1990 = std::getenv("MW1O_BTECH_1990");
  if (game && ref && r1990) with_game(work / "game", game, ref, r1990);
  else std::printf("skip the game parts: MW1O_GAME, MW1O_BTECH_REFERENCE and MW1O_BTECH_1990 not all set\n");
  fs::remove_all(work, ec);  // nothing of the game's kept
  std::printf("%s (%d failed)\n", g_failures ? "FAILED" : "passed", g_failures);
  return g_failures ? 1 : 0;
}
