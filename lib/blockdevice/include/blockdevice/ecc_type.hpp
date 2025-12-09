#pragma once
#include <string_view>

enum class ECCType : std::uint8_t {
    None,
    Crc,
    Hamming,
    Parity,
    ReedSolomon,
};