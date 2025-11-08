#include "ecc_helpers/gf256_utils.hpp"

namespace gf256_utils {

std::vector<GF256> bytes_to_gf(const std::vector<std::byte>& bytes) {
    std::vector<GF256> result;
    result.reserve(bytes.size());
    for (auto b : bytes)
        result.emplace_back(GF256(b));
    return result;
}

std::vector<std::byte> gf_to_bytes(const std::vector<GF256>& values) {
    std::vector<std::byte> result;
    result.reserve(values.size());
    for (auto v : values)
        result.push_back(static_cast<std::byte>(static_cast<uint8_t>(v)));
    return result;
}

}
