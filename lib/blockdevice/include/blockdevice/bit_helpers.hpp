#pragma once

#include <cstdint>
#include <vector>

namespace BitHelpers {

inline bool getBit(const std::vector<std::byte>& data, unsigned int index)
{
    unsigned int byteIndex = index / 8;
    unsigned int bitIndex = index % 8;

    uint8_t byteValue = std::to_integer<uint8_t>(data[byteIndex]);
    return (byteValue >> bitIndex) & 0x1;
}

inline void setBit(std::vector<std::byte>& data, unsigned int index, bool value)
{
    unsigned int byteIndex = index / 8;
    unsigned int bitIndex = index % 8;

    uint8_t byteValue = std::to_integer<uint8_t>(data[byteIndex]);

    if (value)
        byteValue |= (1 << bitIndex); // set 1
    else
        byteValue &= ~(1 << bitIndex); // set 0
    data[byteIndex] = std::byte(byteValue);
}

inline std::vector<bool> blockToBits(const std::vector<std::byte> block)
{
    std::vector<bool> bits(block.size() * 8);

    for (int i = 0; i < bits.size(); ++i) {
        bits[i] = getBit(block, i);
    }

    return bits;
}

}