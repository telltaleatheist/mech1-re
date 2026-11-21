#include "Music.h"

#include "StrictReader.h"

namespace mw1o::import {

namespace fs = std::filesystem;
using detail::Reader;
using support::Json;

namespace {

// BTECH 304E:2BA0, combatAdlibDefaultInstruments: eight rows of 14 bytes, the
// 13 parameters and a byte BTECH never reads (21CA:0860 takes 13).
constexpr std::uint16_t kInstrumentsOffset = 0x2BA0;
constexpr std::size_t kInstruments = 8, kRowBytes = 14;

constexpr const char* kFields[kAdlibParameters] = {
    "key_scale_level", "frequency_multiplier", "feedback",        "attack",
    "sustain_level",   "sustain_enabled",      "decay",           "release",
    "attenuation",     "tremolo_enabled",      "vibrato_enabled", "key_scale_rate_enabled",
    "fm_connection_enabled"};

}  // namespace

std::string music_json(const PieceMap& pieces) {
    const auto piece = pieces.find("music.adlib_instruments");
    if (piece == pieces.end()) return "{\n  \"adlib_instruments\": []\n}\n";
    const std::string& bytes = piece->second.bytes;
    if (piece->second.offset != kInstrumentsOffset || bytes.size() != kInstruments * kRowBytes)
        throw ImportError("the import table's piece music.adlib_instruments is not BTECH 304E:2BA0..2C10");
    std::string out = "{\n  \"adlib_instruments\": [";
    for (std::size_t i = 0; i < kInstruments; ++i) {
        const std::size_t row = i * kRowBytes;
        // The unread byte isn't kept, so the rebuilt row is exact only if it is zero.
        if (bytes[row + kAdlibParameters] != 0)
            throw ImportError("music.adlib_instruments: instrument " + std::to_string(i + 1) +
                              "'s unread last byte is not zero");
        out += std::string(i ? "," : "") + "\n    {";
        for (std::size_t f = 0; f < kAdlibParameters; ++f)
            out += std::string(f ? ", " : "") + "\"" + kFields[f] +
                   "\": " + std::to_string(static_cast<std::uint8_t>(bytes[row + f]));
        out += "}";
    }
    return out + "\n  ]\n}\n";
}

std::vector<AdlibInstrument> load_music(const fs::path& write_dir) {
    Reader r(story_dir(write_dir) / "music.json");
    const Json file = r.parse();
    std::vector<AdlibInstrument> out;
    for (const Json& i : r.array(file, "adlib_instruments")) {
        AdlibInstrument instrument{};
        for (std::size_t f = 0; f < kAdlibParameters; ++f) instrument[f] = r.u8(i, kFields[f]);
        out.push_back(instrument);
    }
    return out;
}

}  // namespace mw1o::import
