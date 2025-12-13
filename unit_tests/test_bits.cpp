#include "common/bit_helpers.hpp"
#include <array>
#include <gtest/gtest.h>

TEST(Bits, BlockToBits)
{
    std::array<std::uint8_t, 2> block_arr;
    block_arr[0] = std::uint8_t(0xff);
    block_arr[1] = std::uint8_t(0x00);
    static_vector<std::uint8_t> block(block_arr.data(), 2, 2);

    std::array<bool, 16> bits_buffer;
    static_vector<bool> bits(bits_buffer.data(), 16);
    BitHelpers::blockToBits(block, bits);

    for (int i = 0; i < 8; i++) {
        EXPECT_TRUE(bits[i]);
        EXPECT_FALSE(bits[i + 8]);
    }
}

TEST(Bits, UlongToBits)
{
    unsigned long a = 0xffffffff;

    std::array<bool, 64> bits_buffer;
    static_vector<bool> bits(bits_buffer.data(), 64);
    BitHelpers::ulongToBits(a, bits);

    for (int i = 0; i < 32; i++) {
        EXPECT_FALSE(bits[i]);
        EXPECT_TRUE(bits[i + 32]);
    }
}