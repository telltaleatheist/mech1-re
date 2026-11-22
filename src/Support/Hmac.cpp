#include "Hmac.h"

#include <vector>

#include "Sha256.h"

namespace mw1o::support {

Digest sha256_digest(std::span<const std::uint8_t> data) {
    const std::string hex = sha256_hex(data);
    Digest out{};
    const auto nibble = [](char c) { return static_cast<std::uint8_t>(c <= '9' ? c - '0' : c - 'a' + 10); };
    for (std::size_t i = 0; i < out.size(); ++i)
        out[i] = static_cast<std::uint8_t>(nibble(hex[i * 2]) << 4 | nibble(hex[i * 2 + 1]));
    return out;
}

std::string hmac_sha256_hex(std::span<const std::uint8_t> key, std::span<const std::uint8_t> message) {
    constexpr std::size_t kBlock = 64;
    std::array<std::uint8_t, kBlock> padded{};
    if (key.size() > kBlock) {
        const Digest hashed = sha256_digest(key);
        std::copy(hashed.begin(), hashed.end(), padded.begin());
    } else {
        std::copy(key.begin(), key.end(), padded.begin());
    }
    std::vector<std::uint8_t> inner(kBlock + message.size());
    for (std::size_t i = 0; i < kBlock; ++i) inner[i] = padded[i] ^ 0x36;
    std::copy(message.begin(), message.end(), inner.begin() + kBlock);
    const Digest inner_hash = sha256_digest(inner);
    std::vector<std::uint8_t> outer(kBlock + inner_hash.size());
    for (std::size_t i = 0; i < kBlock; ++i) outer[i] = padded[i] ^ 0x5c;
    std::copy(inner_hash.begin(), inner_hash.end(), outer.begin() + kBlock);
    return sha256_hex(outer);
}

}  // namespace mw1o::support
