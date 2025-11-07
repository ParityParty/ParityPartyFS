#pragma once
#include <vector>
#include <cstddef>
#include "gf256.h"

namespace gf256_utils {

std::vector<GF256> bytes_to_gf(const std::vector<std::byte>& bytes);
std::vector<std::byte> gf_to_bytes(const std::vector<GF256>& values);

}
