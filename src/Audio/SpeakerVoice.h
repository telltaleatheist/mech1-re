#ifndef MW1O_AUDIO_SPEAKER_VOICE_H
#define MW1O_AUDIO_SPEAKER_VOICE_H
#include <cstdint>

namespace mw1o::audio
{
/* The PC speaker: PIT channel 2 in mode 3 (square wave) behind the port 61h
 * gate. A bounded 32-harmonic band-limited square, integrated over PIT time,
 * so the native period 0014h (about 59 kHz) is suppressed rather than
 * aliased into 48 kHz output. A digital model, not verified analogue
 * acoustics. */
class SpeakerVoice
{
public:
    void reset();
    /* PIT 43h (channel 2 mode commands), 42h (divisor, low then high) and 61h.
     * False for any other write, which the caller treats as unsupported. */
    bool writePort(uint16_t port, uint8_t value);
    uint8_t control() const { return control_; }
    /* Adds the square's integral over `duration` PIT cycles to `integral`
     * (in the caller's order, harmonic by harmonic) and advances its phase. */
    void integrate(double duration, double &integral);

private:
    uint32_t divisor_ = 0x10000;
    uint8_t control_ = 0, periodLow_ = 0;
    bool highByteNext_ = false, counting_ = false;
    double phase_ = 0;
};
}
#endif
