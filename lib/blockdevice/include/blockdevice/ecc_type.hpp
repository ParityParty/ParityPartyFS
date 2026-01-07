#pragma once
#include <cstdint>
#include <string_view>

/**
 * Enumeration of supported error correction code types.
 */
enum class ECCType : std::uint8_t {
    None, ///< No error correction
    Crc, ///< Cyclic redundancy check (detection only)
    Hamming, ///< Hamming code (single-bit correction)
    Parity, ///< Simple parity check (detection only)
    ReedSolomon, ///< Reed-Solomon code (multi-byte correction)
};