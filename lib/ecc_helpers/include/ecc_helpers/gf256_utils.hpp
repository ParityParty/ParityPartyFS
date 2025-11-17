#pragma once
#include "gf256.hpp"
#include <cstddef>
#include <vector>

namespace gf256_utils {

std::vector<GF256> bytes_to_gf(const std::vector<std::uint8_t>& bytes);
std::vector<std::uint8_t> gf_to_bytes(const std::vector<GF256>& values);

}
