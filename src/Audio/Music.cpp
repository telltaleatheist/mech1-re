#include "Music.h"
#include "../Support/Json.h"

#include <cmath>
#include <set>
#include <stdexcept>

#define TSF_IMPLEMENTATION
#define TSF_NO_STDIO /* The host reads the bank (SDL, UTF-8 paths); only tsf_load_memory is used. */
#include "tsf.h"

namespace mw1o::audio
{
namespace
{
using support::Json;

[[noreturn]] void Fail(const std::string &where, const std::string &why)
{
    throw std::runtime_error("instrument table: " + where + ": " + why);
}

void OnlyKeys(const Json &object, const std::string &where, std::initializer_list<std::string_view> keys)
{
    for (const auto &[key, value] : object.object())
    {
        (void)value;
        bool known = false;
        for (std::string_view k : keys) known = known || key == k;
        if (!known) Fail(where, "unknown key \"" + key + "\"");
    }
}

int Whole(const Json &object, std::string_view key, const std::string &where, int low, int high, int fallback, bool required)
{
    const Json *value = object.find(key);
    if (!value)
    {
        if (required) Fail(where, "needs \"" + std::string(key) + "\"");
        return fallback;
    }
    if (!value->is_number()) Fail(where, "\"" + std::string(key) + "\" must be a number");
    const double number = value->number();
    if (number != std::floor(number) || number < low || number > high)
        Fail(where, "\"" + std::string(key) + "\" must be a whole number from " + std::to_string(low) + " to " + std::to_string(high));
    return (int)number;
}
}

InstrumentTable ParseInstrumentTable(std::string_view text)
{
    const Json root = support::parse_json(text);
    if (!root.is_object()) Fail("the file", "must be an object");
    OnlyKeys(root, "the file", {"gain_db", "songs", "drum_keys"});
    InstrumentTable table;
    for (int key = 0; key < 128; ++key) table.drumKeys[key] = (uint8_t)key;
    if (const Json *gain = root.find("gain_db"))
    {
        if (!gain->is_number() || gain->number() < -40 || gain->number() > 20) Fail("gain_db", "must be a number from -40 to 20");
        table.gainDb = (float)gain->number();
    }
    const Json *songs = root.find("songs");
    if (!songs || !songs->is_array() || songs->array().empty()) Fail("songs", "must be a non-empty array");
    std::set<std::string> names, heads;
    for (const Json &entry : songs->array())
    {
        if (!entry.is_object()) Fail("songs", "each song must be an object");
        const Json *name = entry.find("name"), *head = entry.find("head"), *parts = entry.find("parts");
        if (!name || !name->is_string() || name->string().empty()) Fail("songs", "each song needs a \"name\"");
        const std::string where = "song " + name->string();
        OnlyKeys(entry, where, {"name", "head", "parts"});
        if (!head || !head->is_string() || head->string().size() != 16
            || head->string().find_first_not_of("0123456789abcdef") != std::string::npos)
            Fail(where, "\"head\" must be 16 lowercase hex digits");
        if (!names.insert(name->string()).second) Fail(where, "named twice");
        if (!heads.insert(head->string()).second) Fail(where, "its head is another song's");
        if (!parts || !parts->is_array() || parts->array().empty()) Fail(where, "\"parts\" must be a non-empty array");
        InstrumentTable::Song song;
        song.name = name->string();
        song.head = head->string();
        for (const Json &p : parts->array())
        {
            if (!p.is_object()) Fail(where, "each part must be an object");
            OnlyKeys(p, where, {"channel", "mt32", "drums", "program", "level", "pan"});
            const int channel = Whole(p, "channel", where, 1, 16, 0, true);
            const std::string at = where + " channel " + std::to_string(channel);
            InstrumentTable::Part &part = song.parts[channel - 1];
            if (part.used) Fail(at, "listed twice");
            part.used = true;
            if (const Json *drums = p.find("drums"))
            {
                if (!std::holds_alternative<bool>(drums->value)) Fail(at, "\"drums\" must be true or false");
                part.drums = std::get<bool>(drums->value);
            }
            if (const Json *mt32 = p.find("mt32"); mt32 && !mt32->is_string()) Fail(at, "\"mt32\" must be a string");
            part.program = Whole(p, "program", at, 0, 127, 0, !part.drums);
            part.level = Whole(p, "level", at, 0, 127, 100, false);
            part.pan = Whole(p, "pan", at, 0, 127, 64, false);
        }
        table.songs.push_back(song);
    }
    if (const Json *keys = root.find("drum_keys"))
    {
        if (!keys->is_array()) Fail("drum_keys", "must be an array of [from, to] pairs");
        std::set<double> seen;
        for (const Json &pair : keys->array())
        {
            if (!pair.is_array() || pair.array().size() != 2 || !pair.array()[0].is_number() || !pair.array()[1].is_number())
                Fail("drum_keys", "each entry must be [from, to]");
            const double from = pair.array()[0].number(), to = pair.array()[1].number();
            for (double key : {from, to})
                if (key != std::floor(key) || key < 0 || key > 127) Fail("drum_keys", "keys must be whole numbers from 0 to 127");
            if (!seen.insert(from).second) Fail("drum_keys", "a key is mapped twice");
            table.drumKeys[(int)from] = (uint8_t)to;
        }
    }
    return table;
}

Music::~Music()
{
    if (bank_) tsf_close(bank_);
}

bool Music::load(const void *soundfont, std::size_t size, InstrumentTable table, uint32_t sampleRate, std::string &error)
{
    if (bank_) tsf_close(bank_);
    bank_ = tsf_load_memory(soundfont, (int)size);
    if (!bank_)
    {
        error = "the soundfont did not load";
        return false;
    }
    for (const InstrumentTable::Song &song : table.songs)
        for (int channel = 0; channel < 16; ++channel)
        {
            const InstrumentTable::Part &part = song.parts[channel];
            if (part.used && tsf_get_presetindex(bank_, part.drums ? 128 : 0, part.program) < 0)
            {
                error = "the soundfont lacks " + std::string(part.drums ? "kit " : "program ") + std::to_string(part.program)
                    + " (song " + song.name + ", channel " + std::to_string(channel + 1) + ")";
                tsf_close(bank_);
                bank_ = nullptr;
                return false;
            }
        }
    table_ = std::move(table);
    tsf_set_output(bank_, TSF_STEREO_INTERLEAVED, (int)sampleRate, table_.gainDb);
    tsf_set_max_voices(bank_, 96);
    status_ = have_ = 0;
    sysex_ = false;
    return true;
}

std::string Music::startSong(std::string_view head)
{
    for (const InstrumentTable::Song &song : table_.songs)
    {
        if (song.head != head) continue;
        for (int channel = 0; channel < 16; ++channel)
        {
            const InstrumentTable::Part &part = song.parts[channel];
            if (!part.used) continue;
            tsf_channel_set_presetnumber(bank_, channel, part.program, part.drums ? 1 : 0);
            // The level as MIDI's channel volume would set it (TinySoundFont's own curve for CC 7).
            tsf_channel_set_volume(bank_, channel, std::pow(part.level / 127.0f, 3.0f));
            tsf_channel_set_pan(bank_, channel, part.pan / 127.0f);
        }
        return song.name;
    }
    return "";
}

void Music::midiByte(uint8_t value)
{
    if (!bank_) return;
    if (value >= 0xF8) return; // real-time messages interleave anywhere and change nothing here
    if (value == 0xF0) {sysex_ = true; status_ = 0; return;}
    if (value == 0xF7) {sysex_ = false; return;}
    if (value >= 0x80)
    {
        sysex_ = false;
        status_ = value < 0xF0 ? value : 0; // system common messages cancel running status
        have_ = 0;
        return;
    }
    if (sysex_ || !status_) return;
    data_[have_++] = value;
    const uint8_t kind = status_ & 0xF0;
    if (have_ < (kind == 0xC0 || kind == 0xD0 ? 1 : 2)) return;
    channelMessage();
    have_ = 0; // running status: the next data byte starts another message
}

void Music::channelMessage()
{
    const int channel = status_ & 15;
    const uint8_t a = data_[0], b = data_[1];
    const bool drums = tsf_channel_get_preset_bank(bank_, channel) == 128;
    const int key = drums ? table_.drumKeys[a] : a;
    switch (status_ & 0xF0)
    {
    case 0x90:
        if (b) {tsf_channel_note_on(bank_, channel, key, b / 127.0f); break;}
        [[fallthrough]];
    case 0x80: tsf_channel_note_off(bank_, channel, key); break;
    case 0xB0:
        // The table owns volume, pan and the program; the rest is TinySoundFont's.
        if (a != 7 && a != 10 && a != 0 && a != 32) tsf_channel_midi_control(bank_, channel, a, b);
        break;
    case 0xE0: tsf_channel_set_pitchwheel(bank_, channel, a | (b << 7)); break;
    default: break; // program change, channel and key pressure
    }
}

void Music::render(float *stereo, uint32_t frames)
{
    if (!bank_)
    {
        for (uint32_t i = 0; i < frames * 2; ++i) stereo[i] = 0;
        return;
    }
    tsf_render_float(bank_, stereo, (int)frames, 0);
}

int Music::activeVoices() const
{
    return bank_ ? tsf_active_voice_count(bank_) : 0;
}
}
