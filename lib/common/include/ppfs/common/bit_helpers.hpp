#pragma once

#include "ppfs/common/static_vector.hpp"
#include <cstdint>

namespace BitHelpers {

inline bool getBit(const static_vector<uint8_t>& data, unsigned int index)
{
    unsigned int byteIndex = index / 8;
    unsigned int bitIndex = index % 8;

    std::uint8_t byteValue = data[byteIndex];
    return (byteValue >> (7 - bitIndex)) & 0x1;
}

inline bool getBit(const std::uint8_t* data, size_t index)
{
    unsigned int byteIndex = index / 8;
    unsigned int bitIndex = index % 8;

    std::uint8_t byteValue = data[byteIndex];
    return (byteValue >> (7 - bitIndex)) & 0x1;
}

inline void setBit(static_vector<uint8_t>& data, unsigned int index, bool value)
{
    unsigned int byteIndex = index / 8;
    unsigned int bitIndex = index % 8;

    std::uint8_t byteValue = data[byteIndex];

    if (value)
        byteValue |= (1 << (7 - bitIndex)); // set 1
    else
        byteValue &= ~(1 << (7 - bitIndex)); // set 0
    data[byteIndex] = std::uint8_t(byteValue);
}

inline void setBit(std::uint8_t* data, size_t index, bool value)
{
    unsigned int byteIndex = index / 8;
    unsigned int bitIndex = index % 8;

    std::uint8_t byteValue = data[byteIndex];

    if (value)
        byteValue |= (1 << (7 - bitIndex)); // set 1
    else
        byteValue &= ~(1 << (7 - bitIndex)); // set 0
    data[byteIndex] = std::uint8_t(byteValue);
}

inline void blockToBits(const static_vector<uint8_t>& block, static_vector<bool>& bits)
{
    size_t num_bits = block.size() * 8;
    bits.resize(num_bits);
    for (size_t i = 0; i < num_bits; ++i) {
        bits[i] = getBit(block, i);
    }
}

inline void ulongToBits(unsigned long int value, static_vector<bool>& bits)
{
    auto size = sizeof(unsigned long int) * 8;
    bits.resize(size);
    for (size_t i = 0; i < size; ++i) {
        bits[i] = ((value >> (size - 1 - i)) & 1) > 0;
    }
}

}