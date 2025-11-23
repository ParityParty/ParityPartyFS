#include "common/bit_helpers.hpp"
#include <gtest/gtest.h>

TEST(Bits, BlockToBits)
{
    std::vector<std::uint8_t> block(2);
    block[0] = std::uint8_t(0xff);
    block[1] = std::uint8_t(0x00);

    auto bits = BitHelpers::blockToBits(block);

    for (int i = 0; i < 8; i++) {
        EXPECT_TRUE(bits[i]);
        EXPECT_FALSE(bits[i + 8]);
    }
}

TEST(Bits, UlongToBits)
{
    unsigned long a = 0xffffffff;

    auto bits = BitHelpers::ulongToBits(a);

    for (int i = 0; i < 32; i++) {
        EXPECT_FALSE(bits[i]);
        EXPECT_TRUE(bits[i + 32]);
    }
}