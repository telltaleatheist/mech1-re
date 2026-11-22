// HMAC-SHA-256 (RFC 2104) over our SHA-256 (Sha256.h): the keyed hash of the
// word pieces (docs/phase2/PLAN.md 7.4), so a short word's hash in the import
// table can't be found by trying a dictionary. The key is the SHA-256 of a
// region of the player's own exe that the repo never holds (ImportTable.h).
#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>

namespace mw1o::support {

using Digest = std::array<std::uint8_t, 32>;

// The SHA-256 of `data`, as bytes.
Digest sha256_digest(std::span<const std::uint8_t> data);

// Lower-case hex HMAC-SHA-256 of `message` under `key`.
std::string hmac_sha256_hex(std::span<const std::uint8_t> key, std::span<const std::uint8_t> message);

}  // namespace mw1o::support
