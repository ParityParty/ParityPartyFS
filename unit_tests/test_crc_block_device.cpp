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

TEST(CrcBlockDevice, Compiles) { CrcBlockDevice crc(); }