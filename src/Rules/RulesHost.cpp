// The game's door to the rules: read once at start, then held as the exe's
// bytes for the importers in Original/Memory, which run again on every EXEC.
#include "mechwarrior/problem.h"
#include "mechwarrior/rules.h"

#include <cstdint>
#include <cstdio>
#include <exception>

#include "RuleTable.h"
#include "Rules.h"

namespace {
mw1o::support::NativeStore g_rules(mw1o::rules::rule_table(), "rules");
}

bool MwHostPrepareRules(const char *dataDirectory)
{
    try {
        g_rules.hold(mw1o::rules::native_pieces(mw1o::rules::load_rules(dataDirectory)));
        return true;
    } catch (const std::exception &e) {
        MwReportProblem(e.what());
        return false;
    }
}

bool MwRulesCopyNative(const char *id, uint8_t *bytes, size_t size) { return g_rules.copy_native(id, bytes, size); }
bool MwRulesCopyBytes(const char *id, uint8_t *bytes, size_t count, size_t at) { return g_rules.copy_bytes(id, bytes, count, at); }
bool MwRulesCopyWords(const char *id, uint16_t *words, size_t count, size_t at) { return g_rules.copy_words(id, words, count, at); }
size_t MwRulesPieceSize(const char *id) { return g_rules.size(id); }
