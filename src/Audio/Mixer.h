#ifndef MW1O_AUDIO_MIXER_H
#define MW1O_AUDIO_MIXER_H
#include "ToneVoice.h"
#include <cstdint>
#include <vector>

namespace mw1o::audio
{
class Music;
/* Synthesis on PIT time, with no output device: the tone voice plus the music,
 * if any, rendered one 48 kHz frame at a time as PIT time advances. Each block
 * of BLOCK_FRAMES frames goes to the sink. The game thread drives it from the
 * hardware timeline (port writes and IRQs); it never runs game code. */
class Mixer
{
public:
    enum : uint32_t { BLOCK_FRAMES = 256, MUSIC_RUN_FRAMES = 1024 };
    using Sink = void (*)(const float *samples, uint32_t frames);

    void reset(Music *music = nullptr);
    /* One channel for the tone voice alone, two with the music. */
    uint32_t channels() const { return music_ ? 2 : 1; }
    ToneVoice &tone() { return tone_; }
    /* Renders every frame due up to pitTime, sending full blocks to sink. */
    void advance(double pitTime, Sink sink);

private:
    Music *music_ = nullptr;
    /* The music's frames are rendered ahead in runs, up to the frames due by
     * the advance's pitTime (never past it, so an event lands within a frame). */
    std::vector<float> musicFrames_;
    uint32_t musicRead_ = 0, musicCount_ = 0;
    ToneVoice tone_;
    double time_ = 0, sampleCycles_ = 0, sampleIntegral_ = 0;
    float block_[BLOCK_FRAMES * 2] = {};
    uint32_t blockFrames_ = 0;
};
}
#endif
