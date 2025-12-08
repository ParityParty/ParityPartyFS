#pragma once
#include <string_view>

enum class ECCType {
    None,
    Crc,
    Hamming,
    Parity,
    ReedSolomon,
};