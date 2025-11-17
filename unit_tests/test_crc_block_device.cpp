#include "blockdevice/bit_helpers.hpp"
#include "blockdevice/crc_block_device.hpp"
#include "disk/stack_disk.hpp"

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

TEST(CrcPolynomial, division2)
{
    // Example form Wikipedia
    unsigned long num = 0b11010011101100100;
    auto poly1 = CrcPolynomial::MsgExplicit(0b1011);
    auto bits = BitHelpers::ulongToBits(num);
    auto remainder = poly1.divide(bits);
    ASSERT_EQ(remainder.size(), 3);
    EXPECT_FALSE(remainder[0]);
    EXPECT_FALSE(remainder[1]);
    EXPECT_FALSE(remainder[2]);
}

TEST(CrcBlockDevice, Compiles)
{
    StackDisk disk;

    CrcBlockDevice crc(CrcPolynomial::MsgImplicit(0xea), disk, 256);
    EXPECT_EQ(crc.rawBlockSize(), 256);
    EXPECT_EQ(crc.dataSize(), 256 - 1 /* one byte of redundancy with this polynomial*/);
}

TEST(CrcBlockDevice, ReadsAndWrites)
{
    StackDisk disk;
    auto poly = CrcPolynomial::MsgImplicit(0xea);
    CrcBlockDevice crc(poly, disk, 256);

    auto data_size = crc.dataSize();
    std::vector<std::uint8_t> data(data_size, static_cast<std::uint8_t>(0x55));
    ASSERT_TRUE(crc.formatBlock(0).has_value());
    ASSERT_TRUE(crc.writeBlock(data, DataLocation(0, 0)).has_value());
    auto read_ret = crc.readBlock({ 0, 0 }, data_size);
    ASSERT_TRUE(read_ret.has_value());
    for (int i = 0; i < data_size; i++) {
        EXPECT_EQ(data[i], read_ret.value()[i]);
    }
}

TEST(CrcBlockDevice, FindsError)
{
    StackDisk disk;
    auto poly = CrcPolynomial::MsgImplicit(0xea);
    CrcBlockDevice crc(poly, disk, 256);

    auto data_size = crc.dataSize();
    std::vector<std::uint8_t> data(data_size, static_cast<std::uint8_t>(0x00));
    ASSERT_TRUE(crc.formatBlock(0).has_value());
    ASSERT_TRUE(crc.writeBlock(data, DataLocation(0, 0)).has_value());

    // change one bit
    ASSERT_TRUE(disk.write(1, { static_cast<std::uint8_t>(0x01) }).has_value());

    auto read_ret = crc.readBlock({ 0, 0 }, data_size);
    EXPECT_EQ(read_ret.error(), FsError::CorrectionError);
}

TEST(CrcBlockDevice, FindEnoughErrors)
{
    // This polynomial guarantees error detection up to 3 bit flips in messages up to 500 000 bits
    // according to CrcZoo
    auto poly = CrcPolynomial::MsgImplicit(0xc1acf);
    StackDisk disk;
    CrcBlockDevice crc(poly, disk, 512);

    auto data_size = crc.dataSize();
    std::vector<std::uint8_t> data(data_size, static_cast<std::uint8_t>(0x00));
    ASSERT_TRUE(crc.formatBlock(0).has_value());
    ASSERT_TRUE(crc.writeBlock(data, DataLocation(0, 0)).has_value());

    // change 3 bits
    ASSERT_TRUE(disk.write(1, { static_cast<std::uint8_t>(0x01) }).has_value());
    ASSERT_TRUE(disk.write(111, { static_cast<std::uint8_t>(0x08) }).has_value());
    ASSERT_TRUE(disk.write(200, { static_cast<std::uint8_t>(0x02) }).has_value());

    auto read_ret = crc.readBlock({ 0, 0 }, data_size);
    EXPECT_EQ(read_ret.error(), FsError::CorrectionError);
}

TEST(CrcBlockDevice, FindEvenMoreErrors)
{
    // This polynomial guarantees error detection up to 5 bit flips in messages up to 30 000 bits
    // according to CrcZoo
    auto poly = CrcPolynomial::MsgImplicit(0x9960034c);
    StackDisk disk;
    CrcBlockDevice crc(poly, disk, 512);

    auto data_size = crc.dataSize();
    std::vector<std::uint8_t> data(data_size, static_cast<std::uint8_t>(0x00));
    ASSERT_TRUE(crc.formatBlock(0).has_value());
    ASSERT_TRUE(crc.writeBlock(data, DataLocation(0, 0)).has_value());

    // change 3 bits
    ASSERT_TRUE(disk.write(1, { static_cast<std::uint8_t>(0x01) }).has_value());
    ASSERT_TRUE(disk.write(111, { static_cast<std::uint8_t>(0x08) }).has_value());
    ASSERT_TRUE(disk.write(200, { static_cast<std::uint8_t>(0x02) }).has_value());
    ASSERT_TRUE(disk.write(11, { static_cast<std::uint8_t>(0x08) }).has_value());
    ASSERT_TRUE(disk.write(20, { static_cast<std::uint8_t>(0x02) }).has_value());

    auto read_ret = crc.readBlock({ 0, 0 }, data_size);
    EXPECT_EQ(read_ret.error(), FsError::CorrectionError);
}
