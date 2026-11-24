#include "ProgramRecord.h"

// MECHWARRIOR_RELEASE_TAG is a release's tag (CMake refuses any other), undefined in any other build, which then holds
// no stamp at all.
#if defined(MECHWARRIOR_RELEASE_TAG)
#define MW1R_STAMP "MW3-PROGRAM-VERSION:"
// Kept in the file: `used`, and read by build_tag.
#if defined(__GNUC__) || defined(__clang__)
__attribute__((used))
#endif
extern "C" const char mw1r_version_tag[] = MW1R_STAMP MECHWARRIOR_RELEASE_TAG;
const char* mw1o::setup::build_tag() { return mw1r_version_tag + sizeof(MW1R_STAMP) - 1; }
#else
const char* mw1o::setup::build_tag() { return ""; }
#endif
