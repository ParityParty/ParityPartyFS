#include "blockdevice/bit_helpers.hpp"
#include "blockdevice/crc_block_device.hpp"
#include <gtest/gtest.h>

TEST(CrcPolynomial, ExplicitImplicitDifference)
{
    auto poly = CrcPolynomial::MsgExplicit(0xfff);
    auto poly2 = CrcPolynomial::MsgImplicit(0xfff);
    EXPECT_NE(poly2.getCoefficients(), poly.getCoefficients());
}

TEST(CrcPolynomial, Conversion)
{
    auto poly1 = CrcPolynomial::MsgImplicit(0xad0424f3);
    auto poly2 = CrcPolynomial::MsgExplicit(0x15a0849e7);
    EXPECT_EQ(poly1.getCoefficients(), poly2.getCoefficients());
    EXPECT_EQ(32, poly1.getDegree());
    EXPECT_EQ(32, poly2.getDegree());
}

TEST(CrcPolynomial, division)
{
    // Example form Wikipedia
    unsigned long num = 0b11010011101100000;
    auto poly1 = CrcPolynomial::MsgExplicit(0b1011);
    auto bits = BitHelpers::ulongToBits(num);
    auto remainder = poly1.divide(bits);
    ASSERT_EQ(remainder.size(), 3);
    EXPECT_TRUE(remainder[0]);
    EXPECT_FALSE(remainder[1]);
    EXPECT_FALSE(remainder[2]);
}

TEST(CrcBlockDevice, Compiles) { CrcBlockDevice crc(); }