#ifndef MW1O_AUDIO_MIXER_H
#define MW1O_AUDIO_MIXER_H
#include "SpeakerVoice.h"
#include <cstdint>

namespace mw1o::audio
{
/* Synthesis on PIT time, with no output device: the speaker plus the selected
 * card, rendered one 48 kHz frame at a time as PIT time advances. Each block
 * of BLOCK_FRAMES frames goes to the sink. The game thread drives it from the
 * hardware timeline (port writes and IRQs); it never runs game code. */
class Mixer
{
public:
    enum class Card { None, Mt32, Adlib };
    enum : uint32_t { BLOCK_FRAMES = 256 };
    using Sink = void (*)(const float *samples, uint32_t frames);

    /* silentMt32: the MT-32 card is selected but has no synthesizer (logging
     * without ROMs); its frames are silent. */
    void reset(Card card, bool silentMt32);
    /* One channel with no card (the speaker alone), two with a card. */
    uint32_t channels() const { return card_ == Card::None ? 1 : 2; }
    SpeakerVoice &speaker() { return speaker_; }
    /* Renders every frame due up to pitTime, sending full blocks to sink. */
    void advance(double pitTime, Sink sink);

private:
    Card card_ = Card::None;
    bool silentMt32_ = false;
    SpeakerVoice speaker_;
    double time_ = 0, sampleCycles_ = 0, sampleIntegral_ = 0;
    float block_[BLOCK_FRAMES * 2] = {};
    uint32_t blockFrames_ = 0;
};
}
#endif
