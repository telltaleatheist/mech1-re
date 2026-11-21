// The game's door to the rules: read once at start, then held as the exe's
// bytes for the importers in Original/Memory, which run again on every EXEC.
#include "mechwarrior/rules.h"

#include <cstdint>
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

namespace {
// The bytes [at, at + count) of rule piece `id`, or null (with the reason on stderr).
const char *piece_range(const char *id, size_t count, size_t at)
{
    const auto it = g_native.find(id);
    if (it == g_native.end() || !mw1o::rules::find_rule_piece(id)) {
        std::fprintf(stderr, "MechWarrior: the rules have no table \"%s\"\n", id);
        return nullptr;
    }
    const std::string &bytes = it->second;
    if (at > bytes.size() || count > bytes.size() - at) {
        std::fprintf(stderr, "MechWarrior: the rule table \"%s\" ends before byte %zu\n", id, at + count);
        return nullptr;
    }
    return bytes.data() + at;
}
}  // namespace

bool MwRulesCopyBytes(const char *id, uint8_t *bytes, size_t count, size_t at)
{
    const char *from = piece_range(id, count, at);
    if (from) std::memcpy(bytes, from, count);
    return from != nullptr;
}

bool MwRulesCopyWords(const char *id, uint16_t *words, size_t count, size_t at)
{
    if (count > SIZE_MAX / 2) return false;
    const char *from = piece_range(id, count * 2, at);
    for (size_t i = 0; from && i < count; ++i)
        words[i] = static_cast<uint16_t>(static_cast<uint8_t>(from[i * 2]) | static_cast<uint8_t>(from[i * 2 + 1]) << 8);
    return from != nullptr;
}

size_t MwRulesPieceSize(const char *id)
{
    const mw1o::rules::RulePiece *piece = mw1o::rules::find_rule_piece(id);
    return piece ? piece->length : 0;
}
