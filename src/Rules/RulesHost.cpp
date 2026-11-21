// The game's door to the rules: read once at start, then held as the exe's
// bytes for the importers in Original/Memory, which run again on every EXEC.
#include "mechwarrior/rules.h"

#include <cstdio>
#include <cstring>

#include "RuleTable.h"
#include "Rules.h"

namespace {
std::map<std::string, std::string, std::less<>> g_native;
}

bool MwHostPrepareRules(const char *dataDirectory)
{
    try {
        g_native = mw1o::rules::native_pieces(mw1o::rules::load_rules(dataDirectory));
        return true;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "MechWarrior: %s\n", e.what());
        return false;
    }
}

bool MwRulesCopyNative(const char *id, uint8_t *bytes, size_t size)
{
    const mw1o::rules::RulePiece *piece = mw1o::rules::find_rule_piece(id);
    const auto it = g_native.find(id);
    if (!piece || it == g_native.end()) {
        std::fprintf(stderr, "MechWarrior: the rules have no table \"%s\"\n", id);
        return false;
    }
    if (it->second.size() != piece->length || size != piece->length) {
        std::fprintf(stderr, "MechWarrior: the rule table \"%s\" is %zu bytes, the game's %zu\n", id,
                     it->second.size(), size);
        return false;
    }
    std::memcpy(bytes, it->second.data(), size);
    return true;
}

bool MwRulesCopyWords(const char *id, uint16_t *words, size_t count, size_t at)
{
    const auto it = g_native.find(id);
    if (it == g_native.end() || !mw1o::rules::find_rule_piece(id)) {
        std::fprintf(stderr, "MechWarrior: the rules have no table \"%s\"\n", id);
        return false;
    }
    const std::string &bytes = it->second;
    if (at > bytes.size() || count > (bytes.size() - at) / 2) {
        std::fprintf(stderr, "MechWarrior: the rule table \"%s\" ends before word %zu\n", id, at / 2 + count);
        return false;
    }
    for (size_t i = 0; i < count; ++i)
        words[i] = static_cast<uint16_t>(static_cast<uint8_t>(bytes[at + i * 2]) |
                                         static_cast<uint8_t>(bytes[at + i * 2 + 1]) << 8);
    return true;
}

size_t MwRulesPieceSize(const char *id)
{
    const mw1o::rules::RulePiece *piece = mw1o::rules::find_rule_piece(id);
    return piece ? piece->length : 0;
}
