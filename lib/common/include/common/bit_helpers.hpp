#pragma once

#include <cstdint>
#include <vector>

namespace BitHelpers {

inline bool getBit(const std::vector<std::uint8_t>& data, unsigned int index)
{
    unsigned int byteIndex = index / 8;
    unsigned int bitIndex = index % 8;

    std::uint8_t byteValue = data[byteIndex];
    return (byteValue >> (7 - bitIndex)) & 0x1;
}

inline void setBit(std::vector<std::uint8_t>& data, unsigned int index, bool value)
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

inline std::vector<bool> blockToBits(const std::vector<std::uint8_t>& block)
{
    std::vector<bool> bits(block.size() * 8);

    for (int i = 0; i < bits.size(); ++i) {
        bits[i] = getBit(block, i);
    }

    return bits;
}

inline std::vector<bool> ulongToBits(unsigned long int value)
{
    auto size = sizeof(unsigned long int) * 8;
    std::vector<bool> bits(size);
    for (int i = 0; i < size; ++i) {
        bits[i] = ((value >> (size - 1 - i)) & 1) > 0;
    }
    return bits;
}

}