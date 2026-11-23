#ifndef MW1O_AUDIO_TONE_VOICE_H
#define MW1O_AUDIO_TONE_VOICE_H

namespace mw1o::audio
{
/* The tone voice (docs/phase5/NO_EMULATION_PLAN.md): our own small synth that
 * plays one pitch at a time, a square wave of its odd harmonics up to the 31st,
 * none at or above the output's Nyquist, so it never aliases. It takes tones, never ports: a
 * frequency to play, or silence. Combat's effects (Effects, S5) and, until S6,
 * the campaign's speaker music play on it. */
class ToneVoice
{
public:
    void reset();
    /* Plays `frequency` Hz from now. A new tone starts at the waveform's start;
     * a change of pitch while playing keeps the waveform's place. */
    void play(double frequency);
    void stop();
    bool playing() const { return frequency_ > 0; }
    double frequency() const { return frequency_; }
    /* Adds the wave's integral over `duration` PIT cycles (the mixer's clock)
     * to `integral`, and advances its place. */
    void integrate(double duration, double &integral);

private:
    double frequency_ = 0;
    double phase_ = 0; /* The place in the wave, in cycles of it: 0 to 1. */
};
}
#endif
