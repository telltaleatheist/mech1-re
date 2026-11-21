// SHA-256, to check that a file is the version we know (FIPS 180-4). From the
// Crescent Hawk's Inception rebuild, src/formats/sha256.*, 258975d; kept in step
// with it.
#pragma once

#include <cstdint>
#include <span>
#include <string>

namespace mw1o::support {

// Lower-case hex digest of `data`.
std::string sha256_hex(std::span<const std::uint8_t> data);

}  // namespace mw1o::support
