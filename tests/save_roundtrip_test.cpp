// The save's file half (B6), driven by save_roundtrip.py, which compares the blocks field by field. Writes
// <work>/<case>.before and .after, each the 0x718-byte exchange block. No game text: names and numbers only.
//
//     save_roundtrip_test <data dir> <work folder> [a folder of .GAM copies]
#include "mechwarrior/campaign_exchange.h"
#include "mechwarrior/campaign_options.h"
#include "mechwarrior/game_files.h"
#include "mechwarrior/rules.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

namespace {

using Block = std::array<std::uint8_t, CAMPAIGN_SAVE_BYTES>;
int failures = 0;
void Check(bool ok, const std::string &what) {
  if (!ok) {
    std::printf("FAIL %s\n", what.c_str());
    ++failures;
  }
}

void PutBytes(const fs::path &path, const std::uint8_t *bytes, std::size_t count) {
  std::ofstream(path, std::ios::binary).write(reinterpret_cast<const char *>(bytes), static_cast<std::streamsize>(count));
}
Block Live() {
  Block block;
  MwEncodeCampaignExchange(block.data());
  return block;
}
void Forget() {
  const Block zero{};
  MwDecodeCampaignExchange(zero.data());
}
bool Listed(const std::string &stem) {
  if (MwEnumerateCampaignSaves()) return false;
  for (std::uint16_t row = 0; row < campaignOptions.saveFileCount; ++row)
    if (stem == campaignOptions.saveListRows[row].caption.columns.filename) return true;
  return false;
}

// Saves the live campaign, forgets it, loads it back through the list's name.
void RoundTrip(const fs::path &work, const std::string &name, const Block &block) {
  MwDecodeCampaignExchange(block.data());
  Check(Live() == block, name + ": the live state is the block");
  Check(MwWriteCampaignSaveFile("TRIP.GAM") == CampaignSaveWrite::Written, name + ": saved");
  Check(Listed("TRIP"), name + ": listed");
  Forget();
  Check(MwReadCampaignSaveFile("TRIP.GAM"), name + ": loaded");
  const Block after = Live();
  PutBytes(work / (name + ".before"), block.data(), block.size());
  PutBytes(work / (name + ".after"), after.data(), after.size());
}

}  // namespace

int main(int argc, char **argv) {
  if (argc != 3 && argc != 4) {
    std::fprintf(stderr, "usage: save_roundtrip_test <data dir> <work folder> [a folder of .GAM copies]\n");
    return 2;
  }
  const fs::path work = argv[2], saves = work / "saves";
  std::error_code ec;
  fs::create_directories(saves);
  mw1o::files::set_save_directory(saves.string());

  // The new campaign's start (our rules), with a name of ours, then some of it played on.
  Block start{};
  Check(MwHostPrepareRules(argv[1]) && MwRulesCopyNative("new_campaign.before_name", start.data(), 0x003B) &&
            MwRulesCopyNative("new_campaign.after_name", start.data() + 0x0045, CAMPAIGN_SAVE_BYTES - 0x0045),
        "the new campaign's start state");
  std::memcpy(start.data() + 0x003B, "ROUNDTRIP", 9);
  RoundTrip(work, "new_campaign", start);
  MwDecodeCampaignExchange(start.data());
  campaignEconomy.campaignFundsLow ^= 0x1234;
  campaignEconomy.campaignFundsHigh ^= 0x0005;
  campaignCore.calendarMonthIndex = 7;
  campaignCore.calendarYear += 2;
  campaignMechanics.rosterStatus[1] ^= 3;
  campaignMission.salvageEntries[2].unitKind = 4;
  campaignMission.salvageEntries[2].conditionPercent = 61;
  campaignMission.commanderNameBytes[0] = 'X';
  RoundTrip(work, "played_on", Live());
  // Every byte of the block its own value, then the inverse: no byte is lost, moved or shared.
  Block pattern;
  for (std::size_t i = 0; i < pattern.size(); ++i) pattern[i] = static_cast<std::uint8_t>(i * 7 + (i >> 8) * 31 + 1);
  RoundTrip(work, "pattern", pattern);
  for (std::uint8_t &byte : pattern) byte = static_cast<std::uint8_t>(~byte);
  RoundTrip(work, "pattern_inverse", pattern);

  // The load reads one byte past a save, so an oversized file is refused; a short one is too. The company stays.
  const Block held = Live();
  std::array<std::uint8_t, CAMPAIGN_SAVE_BYTES + 1> raw{};
  PutBytes(saves / "SHORT.GAM", raw.data(), CAMPAIGN_SAVE_BYTES - 1);
  PutBytes(saves / "LONG.GAM", raw.data(), CAMPAIGN_SAVE_BYTES + 1);
  Check(!MwReadCampaignSaveFile("SHORT.GAM") && Live() == held, "a short save is refused, the company kept");
  Check(!MwReadCampaignSaveFile("LONG.GAM") && Live() == held, "a save one byte too long is refused, the company kept");
  Check(!MwReadCampaignSaveFile("MISSING.GAM") && Live() == held, "a missing save is refused, the company kept");
  PutBytes(saves / "WHOLE.GAM", raw.data(), CAMPAIGN_SAVE_BYTES);
  Check(MwReadCampaignSaveFile("WHOLE.GAM"), "the exact size loads");
  fs::remove(saves / "SHORT.GAM", ec);
  fs::remove(saves / "LONG.GAM", ec);
  fs::remove(saves / "WHOLE.GAM", ec);

  // The list holds twelve saves and no more.
  for (int i = 0; i < CAMPAIGN_SAVE_ROWS + 3; ++i) PutBytes(saves / ("N" + std::to_string(i) + ".GAM"), raw.data(), 1);
  Check(!MwEnumerateCampaignSaves() && campaignOptions.saveFileCount == CAMPAIGN_SAVE_ROWS, "the list stops at its twelve rows");
  for (int i = 0; i < CAMPAIGN_SAVE_ROWS + 3; ++i) fs::remove(saves / ("N" + std::to_string(i) + ".GAM"), ec);

  if (argc == 4) {
    for (const auto &entry : fs::directory_iterator(argv[3])) {
      const std::string stem = entry.path().stem().string(), name = stem + ".GAM";
      Block file;
      std::ifstream(entry.path(), std::ios::binary).read(reinterpret_cast<char *>(file.data()), file.size());
      fs::copy_file(entry.path(), saves / name, fs::copy_options::overwrite_existing);
      Forget();
      Check(Listed(stem) && MwReadCampaignSaveFile(name.c_str()), "a kept save lists and loads");
      const Block after = Live();
      PutBytes(work / ("kept_" + stem + ".before"), file.data(), file.size());
      PutBytes(work / ("kept_" + stem + ".after"), after.data(), after.size());
    }
  }
  if (failures) std::printf("%d failed\n", failures);
  return failures ? 1 : 0;
}
