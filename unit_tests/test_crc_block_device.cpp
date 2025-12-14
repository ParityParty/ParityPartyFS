#include "blockdevice/crc_block_device.hpp"
#include "common/bit_helpers.hpp"
#include "common/static_vector.hpp"
#include "disk/stack_disk.hpp"

#include <array>
#include <gtest/gtest.h>
#include <vector>

TEST(CrcPolynomial, ExplicitImplicitDifference)
{
    auto poly = CrcPolynomial::MsgExplicit(0xfff);
    auto poly2 = CrcPolynomial::MsgImplicit(0xfff);
    std::array<bool, MAX_CRC_POLYNOMIAL_SIZE> coeffs1_buffer, coeffs2_buffer;
    static_vector<bool> coeffs1(coeffs1_buffer.data(), MAX_CRC_POLYNOMIAL_SIZE);
    static_vector<bool> coeffs2(coeffs2_buffer.data(), MAX_CRC_POLYNOMIAL_SIZE);
    poly.getCoefficients(coeffs1);
    poly2.getCoefficients(coeffs2);
    ASSERT_NE(coeffs1.size(), coeffs2.size());
}

TEST(CrcPolynomial, Conversion)
{
    auto poly1 = CrcPolynomial::MsgImplicit(0xad0424f3);
    auto poly2 = CrcPolynomial::MsgExplicit(0x15a0849e7);
    std::array<bool, MAX_CRC_POLYNOMIAL_SIZE> coeffs1_buffer, coeffs2_buffer;
    static_vector<bool> coeffs1(coeffs1_buffer.data(), MAX_CRC_POLYNOMIAL_SIZE);
    static_vector<bool> coeffs2(coeffs2_buffer.data(), MAX_CRC_POLYNOMIAL_SIZE);
    poly1.getCoefficients(coeffs1);
    poly2.getCoefficients(coeffs2);
    ASSERT_EQ(coeffs1.size(), coeffs2.size());
    for (size_t i = 0; i < coeffs1.size(); i++) {
        EXPECT_EQ(coeffs1[i], coeffs2[i]) << "Mismatch at index " << i;
    }
    EXPECT_EQ(32, poly1.getDegree());
    EXPECT_EQ(32, poly2.getDegree());
}

TEST(CrcPolynomial, division)
{
    // Example form Wikipedia
    unsigned long num = 0b11010011101100000;
    auto poly1 = CrcPolynomial::MsgExplicit(0b1011);
    std::array<bool, 64> bits_buffer;
    static_vector<bool> bits(bits_buffer.data(), 64);
    BitHelpers::ulongToBits(num, bits);
    std::array<bool, MAX_CRC_POLYNOMIAL_SIZE> remainder_buffer;
    static_vector<bool> remainder(remainder_buffer.data(), MAX_CRC_POLYNOMIAL_SIZE);
    poly1.divide(bits, remainder);
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
    std::array<bool, 64> bits_buffer;
    static_vector<bool> bits(bits_buffer.data(), 64);
    BitHelpers::ulongToBits(num, bits);
    std::array<bool, MAX_CRC_POLYNOMIAL_SIZE> remainder_buffer;
    static_vector<bool> remainder(remainder_buffer.data(), MAX_CRC_POLYNOMIAL_SIZE);
    poly1.divide(bits, remainder);
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
    std::array<uint8_t, 512> data_buffer;
    std::fill(data_buffer.begin(), data_buffer.begin() + data_size, static_cast<std::uint8_t>(0x55));
    static_vector<uint8_t> data(data_buffer.data(), data_buffer.size(), data_size);
    ASSERT_TRUE(crc.formatBlock(0).has_value());
    ASSERT_TRUE(crc.writeBlock(data, DataLocation(0, 0)).has_value());
    
    std::array<uint8_t, 512> read_buffer;
    static_vector<uint8_t> read_data(read_buffer.data(), read_buffer.size());
    auto read_ret = crc.readBlock({ 0, 0 }, data_size, read_data);
    ASSERT_TRUE(read_ret.has_value());
    for (int i = 0; i < data_size; i++) {
        EXPECT_EQ(data[i], read_data[i]);
    }
}

TEST(CrcBlockDevice, FindsError)
{
    StackDisk disk;
    auto poly = CrcPolynomial::MsgImplicit(0xea);
    CrcBlockDevice crc(poly, disk, 256);

    auto data_size = crc.dataSize();
    std::array<uint8_t, 512> data_buffer;
    std::fill(data_buffer.begin(), data_buffer.begin() + data_size, static_cast<std::uint8_t>(0x00));
    static_vector<uint8_t> data(data_buffer.data(), data_buffer.size(), data_size);
    ASSERT_TRUE(crc.formatBlock(0).has_value());
    ASSERT_TRUE(crc.writeBlock(data, DataLocation(0, 0)).has_value());

    // change one bit
    std::array<uint8_t, 1> write_buffer = { static_cast<std::uint8_t>(0x01) };
    static_vector<uint8_t> write_data(write_buffer.data(), 1, 1);
    ASSERT_TRUE(disk.write(1, write_data).has_value());

    std::array<uint8_t, 512> read_buffer;
    static_vector<uint8_t> read_data(read_buffer.data(), read_buffer.size());
    auto read_ret = crc.readBlock({ 0, 0 }, data_size, read_data);
    EXPECT_FALSE(read_ret.has_value());
    EXPECT_EQ(read_ret.error(), FsError::BlockDevice_CorrectionError);
}

TEST(CrcBlockDevice, FindEnoughErrors)
{
    // This polynomial guarantees error detection up to 3 bit flips in messages up to 500 000 bits
    // according to CrcZoo
    auto poly = CrcPolynomial::MsgImplicit(0xc1acf);
    StackDisk disk;
    CrcBlockDevice crc(poly, disk, 512);

    auto data_size = crc.dataSize();
    std::array<uint8_t, 1024> data_buffer;
    std::fill(data_buffer.begin(), data_buffer.begin() + data_size, static_cast<std::uint8_t>(0x00));
    static_vector<uint8_t> data(data_buffer.data(), data_buffer.size(), data_size);
    ASSERT_TRUE(crc.formatBlock(0).has_value());
    ASSERT_TRUE(crc.writeBlock(data, DataLocation(0, 0)).has_value());

    // change 3 bits
    std::array<uint8_t, 1> write_buffer1 = { static_cast<std::uint8_t>(0x01) };
    static_vector<uint8_t> write_data1(write_buffer1.data(), 1, 1);
    ASSERT_TRUE(disk.write(1, write_data1).has_value());
    std::array<uint8_t, 1> write_buffer2 = { static_cast<std::uint8_t>(0x08) };
    static_vector<uint8_t> write_data2(write_buffer2.data(), 1, 1);
    ASSERT_TRUE(disk.write(111, write_data2).has_value());
    std::array<uint8_t, 1> write_buffer3 = { static_cast<std::uint8_t>(0x02) };
    static_vector<uint8_t> write_data3(write_buffer3.data(), 1, 1);
    ASSERT_TRUE(disk.write(200, write_data3).has_value());

    std::array<uint8_t, 1024> read_buffer;
    static_vector<uint8_t> read_data(read_buffer.data(), read_buffer.size());
    auto read_ret = crc.readBlock({ 0, 0 }, data_size, read_data);
    EXPECT_FALSE(read_ret.has_value());
    EXPECT_EQ(read_ret.error(), FsError::BlockDevice_CorrectionError);
}

TEST(CrcBlockDevice, FindEvenMoreErrors)
{
    // This polynomial guarantees error detection up to 5 bit flips in messages up to 30 000 bits
    // according to CrcZoo
    auto poly = CrcPolynomial::MsgImplicit(0x9960034c);
    StackDisk disk;
    CrcBlockDevice crc(poly, disk, 512);

    auto data_size = crc.dataSize();
    std::array<uint8_t, 1024> data_buffer;
    std::fill(data_buffer.begin(), data_buffer.begin() + data_size, static_cast<std::uint8_t>(0x00));
    static_vector<uint8_t> data(data_buffer.data(), data_buffer.size(), data_size);
    ASSERT_TRUE(crc.formatBlock(0).has_value());
    ASSERT_TRUE(crc.writeBlock(data, DataLocation(0, 0)).has_value());

    // change 5 bits
    std::array<uint8_t, 1> write_buffer;
    write_buffer[0] = static_cast<std::uint8_t>(0x01);
    static_vector<uint8_t> write_data(write_buffer.data(), 1, 1);
    ASSERT_TRUE(disk.write(1, write_data).has_value());
    write_buffer[0] = static_cast<std::uint8_t>(0x08);
    ASSERT_TRUE(disk.write(111, write_data).has_value());
    write_buffer[0] = static_cast<std::uint8_t>(0x02);
    ASSERT_TRUE(disk.write(200, write_data).has_value());
    write_buffer[0] = static_cast<std::uint8_t>(0x08);
    ASSERT_TRUE(disk.write(11, write_data).has_value());
    write_buffer[0] = static_cast<std::uint8_t>(0x02);
    ASSERT_TRUE(disk.write(20, write_data).has_value());

    std::array<uint8_t, 1024> read_buffer;
    static_vector<uint8_t> read_data(read_buffer.data(), read_buffer.size());
    auto read_ret = crc.readBlock({ 0, 0 }, data_size, read_data);
    EXPECT_FALSE(read_ret.has_value());
    EXPECT_EQ(read_ret.error(), FsError::BlockDevice_CorrectionError);
}
