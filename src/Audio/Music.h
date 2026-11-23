#ifndef MW1O_AUDIO_MUSIC_H
#define MW1O_AUDIO_MUSIC_H
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

struct tsf;

namespace mw1o::audio
{
/* The instrument table (data/music/instruments.json; docs/phase5/NO_EMULATION_PLAN.md,
 * "Instruments"): the songs carry no Program Change, so per song and MIDI
 * channel, which General MIDI program plays it, at what level and pan. A song
 * is named by the SHA-256 prefix of its track's first 32 bytes, as the event
 * log's "song" line names it. Numbers only; no text from the game. */
struct InstrumentTable
{
    struct Part
    {
        bool used = false, drums = false;
        int program = 0;          // 0..127; the kit (bank 128) when drums
        int level = 100, pan = 64; // 0..127, as MIDI's volume and pan
    };
    struct Song
    {
        std::string name, head;   // head: 16 hex digits
        std::array<Part, 16> parts; // by MIDI channel - 1
    };
    float gainDb = 0;
    std::vector<Song> songs;
    std::array<uint8_t, 128> drumKeys; // the rhythm part's key -> the kit's key
};

/* Parses and checks the table; throws std::runtime_error naming the problem. */
InstrumentTable ParseInstrumentTable(std::string_view json);

/* The campaign's music, rendered from a General MIDI soundfont through
 * TinySoundFont (MIT, extern/TinySoundFont). It takes what the game's own
 * sequencer sends (mechwarrior/music_out.h): song starts, which pick the
 * instrument table's programs, and MIDI bytes, of which it plays notes,
 * note-offs, the all-notes-off and all-sound-off controllers, sustain and pitch
 * bend. SysEx (the MT-32's timbre set-up) and program changes are skipped:
 * the table chooses the instruments. One thread; not an emulation of any card. */
class Music
{
public:
    Music() = default;
    Music(const Music &) = delete;
    Music &operator=(const Music &) = delete;
    ~Music();

    /* The soundfont's bytes and the table. False, with `error`, if the bank
     * doesn't load or lacks a program the table names. */
    bool load(const void *soundfont, std::size_t size, InstrumentTable table, uint32_t sampleRate, std::string &error);
    bool loaded() const { return bank_ != nullptr; }
    /* A song starts: its parts take the table's programs, levels and pans.
     * A song not in the table keeps the parts as they were. Returns its name,
     * or "" if the table doesn't know it. */
    std::string startSong(std::string_view head);
    void midiByte(uint8_t value);
    /* Interleaved stereo, added to nothing: `stereo` is overwritten. */
    void render(float *stereo, uint32_t frames);
    /* Voices sounding now (tests). */
    int activeVoices() const;

private:
    void channelMessage();
    tsf *bank_ = nullptr;
    InstrumentTable table_;
    uint8_t status_ = 0, data_[2] = {}, have_ = 0;
    bool sysex_ = false;
};
}
#endif
