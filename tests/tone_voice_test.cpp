// The tone voice (src/Audio/ToneVoice, S5): silence, a square's halves, the
// waveform's place kept across a repeated or changed pitch, a new tone from
// the start, and nothing at or above the output's Nyquist. Plain checks, no
// framework: exit 0 when all hold.
#include "AudioClock.h"
#include "ToneVoice.h"

#include <cmath>
#include <cstdio>

using mw1o::audio::PIT_CLOCK_HZ;
using mw1o::audio::ToneVoice;

namespace {

int failures = 0;
void Check(bool ok, const char *what) {
  if (!ok) {
    std::printf("FAIL %s\n", what);
    ++failures;
  }
}

// The voice's integral over `cycles` of a wave of `frequency` Hz.
double Over(ToneVoice &voice, double frequency, double cycles) {
  double integral = 0;
  voice.integrate(cycles * PIT_CLOCK_HZ / frequency, integral);
  return integral;
}

}  // namespace

int main() {
  ToneVoice voice;
  const double half = PIT_CLOCK_HZ / 440.0 / 2;  // half a cycle at 440 Hz, in PIT cycles

  double integral = 0;
  voice.integrate(1000, integral);
  Check(!voice.playing() && integral == 0, "a new voice is silent");

  voice.play(440);
  double first = Over(voice, 440, 0.5), second = Over(voice, 440, 0.5);
  Check(first > 0.85 * half && first < 1.1 * half, "the first half of a cycle is high");
  Check(second < -0.85 * half && second > -1.1 * half, "the second half is low");
  Check(std::fabs(Over(voice, 440, 1)) < 1e-6 * half, "a whole cycle sums to nothing");

  voice.play(440);
  Over(voice, 440, 0.5);
  voice.play(440);
  Check(Over(voice, 440, 0.5) < 0, "the same pitch again keeps the wave's place");
  voice.play(440);
  Over(voice, 440, 0.5);
  voice.play(880);
  Check(Over(voice, 880, 0.5) < 0, "a new pitch while playing keeps the wave's place");

  voice.play(440);
  Over(voice, 440, 0.5);
  voice.stop();
  integral = 0;
  voice.integrate(1000, integral);
  Check(!voice.playing() && integral == 0, "stopped: silent");
  voice.play(440);
  Check(Over(voice, 440, 0.5) > 0, "a new tone starts at the wave's start");

  voice.play(0);
  Check(!voice.playing(), "a frequency of 0 stops");
  voice.play(30000);
  Check(voice.playing() && Over(voice, 30000, 0.5) == 0, "a fundamental above Nyquist: nothing");
  voice.stop();
  voice.play(10000);
  double fundamentalOnly = Over(voice, 10000, 0.5);
  const double fundamental = 4 / 3.14159265358979323846 * (PIT_CLOCK_HZ / 10000.0) / 3.14159265358979323846;
  Check(std::fabs(fundamentalOnly - fundamental) < 1e-6 * fundamental, "10 kHz: the fundamental alone");

  std::printf("tone_voice: %s\n", failures ? "FAILED" : "ok");
  return failures ? 1 : 0;
}
