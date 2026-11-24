// The story's runs of words and native spans (story_span.h): reads, bounds, gaps in the table, and every stop, by its
// message. No game file is needed.
#include <cstdio>
#include <cstring>
#include <string>

#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/story_span.h"

namespace {
struct Refused {
  std::string reason;
};
int failed = 0;

template <class Call>
std::string Reason(Call call) {
  try {
    call();
  } catch (const Refused &r) {
    return r.reason;
  }
  return "nothing";
}
template <class Call>
void Stops(Call call, const std::string &want) {
  const std::string got = Reason(call), whole = "Story data: " + want + ". This is a fault of the program";
  std::printf("  %s\n", got.c_str());
  if (got == whole) return;
  std::printf("FAIL: wanted \"%s\"\n", whole.c_str());
  ++failed;
}
void Check(bool ok, const char *what) {
  if (ok) return;
  std::printf("FAIL: %s\n", what);
  ++failed;
}
}  // namespace

[[noreturn]] void MwAbortWithReason(const char *, const char *reason, const char *, int, const char *) {
  throw Refused{reason};
}

int main() {
  const MwStorySpan span("test.run", 0x1000, {'a', 'b', 0, 'c', 0, 'd'});
  Check(span.size() == 6 && span.byte(5) == 'd' && span.word(0) == 'a' + 'b' * 256 && span.word(4) == 'd' * 256, "the reads");
  Check(!std::strcmp(span.text(0), "ab") && !std::strcmp(span.text(1), "b") && !std::strcmp(span.text(3), "c"), "the texts");
  Check(span.holds(0x1000) && span.holds(0x1005) && !span.holds(0x1006) && !span.holds(0x0FFF), "what it holds");
  Check(span.holdsText(0x1004) && !span.holdsText(0x1005) && !span.holdsText(0x1006), "the texts it holds");
  Check(span.index(0x1005) == 5, "the index");
  char out[6];
  span.copyAll(out, 6);
  Check(!std::memcmp(out, "ab\0c\0d", 6), "the copy");
  Stops([&] { span.byte(6); }, "\"test.run\" is 6 bytes, read at 6 (1 bytes)");
  Stops([&] { span.word(5); }, "\"test.run\" is 6 bytes, read at 5 (2 bytes)");
  Stops([&] { span.bytes(size_t(-1), 2); }, "\"test.run\" is 6 bytes, read at 18446744073709551615 (2 bytes)");
  Stops([&] { span.copyAll(out, 5); }, "\"test.run\" is 6 bytes, not 5");
  Stops([&] { span.text(5); }, "\"test.run\" holds a text without its end");
  Stops([&] { span.index(0x1006); }, "\"test.run\" (1000) does not hold 1006");

  // The table's runs. No words are loaded here, so a run without a gap stops at its first word.
  Stops([] { MwStorySpan::words({"test.word.none"}); }, "\"test.word.none\" names no word");
  Stops([] { MwStorySpan::words({"complex.word.ammo_family."}); },
        "\"complex.word.ammo_family.\" has a gap or an overlap before \"complex.word.ammo_family.2\"");
  Stops([] { MwStorySpan::words({"hub.word.month.", "hub.word.month.1"}); },
        "\"hub.word.month., hub.word.month.1\" has a gap or an overlap before \"hub.word.month.1\"");
  Stops([] { MwStorySpan::words({"orders.word."}); }, "\"orders.word.\" has no word \"orders.word.order.1\"");
  Stops([] { MwStorySpan::words({"hub.word.month."}); }, "\"hub.word.month.\" has no word \"hub.word.month.1\"");
  Stops([] { MwStorySpan::words({"cockpit.word.energy_ammo", "cockpit.word.range_class.", "cockpit.word.range_text_end"}); },
        "\"cockpit.word.energy_ammo, cockpit.word.range_class., cockpit.word.range_text_end\" has no word "
        "\"cockpit.word.range_class.1\"");
  Stops([] { MwStorySpan::native("music.driver", 0x0A12); }, "\"music.driver\" has no span of 2578 bytes");
  char text[8];
  Check(!MwWordsCopyTextAt("hub.word.", 1, text, sizeof(text)), "no word starts at 0001");
  std::printf("%d failed\n", failed);
  return failed != 0;
}
