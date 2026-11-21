// story/music.json: the music the exes themselves hold (docs/phase2/PLAN.md),
// which is BTECH's eight AdLib default instruments. The rest of MW1's music
// (the songs, the combat sequence table filled from MUS:INF, MW_MAIN's
// instruments) is read from the game's data files, not the exes.
//
//   {"adlib_instruments": [{"key_scale_level", "frequency_multiplier",
//     "feedback", "attack", "sustain_level", "sustain_enabled", "decay",
//     "release", "attenuation", "tremolo_enabled", "vibrato_enabled",
//     "key_scale_rate_enabled", "fm_connection_enabled"}, ...]}
//
// The fields are one OPL operator's parameters in the order BTECH keeps them
// (BtechAdlibOperator), each a byte as stored. The instruments are in the
// original's order; BTECH 21CA:0440 gives them to the melodic modulators, the
// melodic carriers, then, in rhythm mode, operators 12, 15, 16, 14, 17 and 13
// (bass drum modulator and carrier, snare, tom-tom, cymbal, hi-hat).
#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "Records.h"

namespace mw1o::import {

constexpr std::size_t kAdlibParameters = 13;
using AdlibInstrument = std::array<std::uint8_t, kAdlibParameters>;

// From the piece music.adlib_instruments (BTECH 304E:2BA0, eight rows of the
// 13 parameters and a zero byte). A table without it writes an empty list.
// Throws ImportError.
std::string music_json(const PieceMap& pieces);

// Read strictly; throws ImportError naming the file.
std::vector<AdlibInstrument> load_music(const std::filesystem::path& write_dir);

}  // namespace mw1o::import
