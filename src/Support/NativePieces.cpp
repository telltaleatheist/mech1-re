#include "NativePieces.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

namespace mw1o::support {

bool NativeStore::copy_native(const char* id, std::uint8_t* bytes, std::size_t size) const {
    const NativePiece* piece = find_piece(table_, id);
    const auto it = bytes_.find(id);
    if (!piece || it == bytes_.end()) {
        std::fprintf(stderr, "MechWarrior: the %s have no table \"%s\"\n", what_, id);
        return false;
    }
    if (it->second.size() != piece->length || size != piece->length) {
        std::fprintf(stderr, "MechWarrior: the %s table \"%s\" is %zu bytes, the game's %zu\n", what_, id,
                     it->second.size(), size);
        return false;
    }
    std::memcpy(bytes, it->second.data(), size);
    return true;
}

// The bytes [at, at + count) of piece `id`, or null (with the reason on stderr).
const char* NativeStore::range(const char* id, std::size_t count, std::size_t at) const {
    const auto it = bytes_.find(id);
    if (it == bytes_.end() || !find_piece(table_, id)) {
        std::fprintf(stderr, "MechWarrior: the %s have no table \"%s\"\n", what_, id);
        return nullptr;
    }
    const std::string& bytes = it->second;
    if (at > bytes.size() || count > bytes.size() - at) {
        std::fprintf(stderr, "MechWarrior: the %s table \"%s\" ends before byte %zu\n", what_, id, at + count);
        return nullptr;
    }
    return bytes.data() + at;
}

bool NativeStore::copy_bytes(const char* id, std::uint8_t* bytes, std::size_t count, std::size_t at) const {
    const char* from = range(id, count, at);
    if (from) std::memcpy(bytes, from, count);
    return from != nullptr;
}

bool NativeStore::copy_words(const char* id, std::uint16_t* words, std::size_t count, std::size_t at) const {
    if (count > SIZE_MAX / 2) return false;
    const char* from = range(id, count * 2, at);
    for (std::size_t i = 0; from && i < count; ++i)
        words[i] = static_cast<std::uint16_t>(static_cast<std::uint8_t>(from[i * 2]) |
                                              static_cast<std::uint8_t>(from[i * 2 + 1]) << 8);
    return from != nullptr;
}

std::size_t NativeStore::size(const char* id) const {
    const NativePiece* piece = find_piece(table_, id);
    return piece ? piece->length : 0;
}

}  // namespace mw1o::support
