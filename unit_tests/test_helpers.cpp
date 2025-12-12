#include "common/math_helpers.hpp"

#include <gtest/gtest.h>

TEST(MathHelpers, BinLog_PowersOfTwo)
{
    EXPECT_EQ(binLog(1u), 0u);
    EXPECT_EQ(binLog(2u), 1u);
    EXPECT_EQ(binLog(4u), 2u);
    EXPECT_EQ(binLog(8u), 3u);
    EXPECT_EQ(binLog(16u), 4u);
    EXPECT_EQ(binLog(32u), 5u);
    EXPECT_EQ(binLog(64u), 6u);
    EXPECT_EQ(binLog(128u), 7u);
    EXPECT_EQ(binLog(256u), 8u);
}