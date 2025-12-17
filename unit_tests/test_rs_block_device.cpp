#include "blockdevice/rs_block_device.hpp"
#include "common/static_vector.hpp"
#include "disk/stack_disk.hpp"

#include <array>
#include <gtest/gtest.h>
#include <random>

TEST(ReedSolomonBlockDevice, BasicReadWrite)
{
    StackDisk disk;
    ReedSolomonBlockDevice rs(disk, 255, 2);

    auto data_size = rs.dataSize();
    std::array<uint8_t, 512> data_buffer;
    std::fill(data_buffer.begin(), data_buffer.begin() + data_size, static_cast<std::uint8_t>(0xAB));
    static_vector<uint8_t> data(data_buffer.data(), data_buffer.size(), data_size);

    ASSERT_TRUE(rs.formatBlock(0).has_value());
    ASSERT_TRUE(rs.writeBlock(data, DataLocation(0, 0)).has_value());

    std::array<uint8_t, 512> read_buffer;
    static_vector<uint8_t> read_data(read_buffer.data(), read_buffer.size());
    auto read_ret = rs.readBlock({ 0, 0 }, data_size, read_data);
    ASSERT_TRUE(read_ret.has_value());

    for (size_t i = 0; i < data_size; i++) {
        EXPECT_EQ(data[i], read_data[i]) << "Mismatch at byte " << i;
    }
}

TEST(ReedSolomonBlockDevice, SingleByteError)
{
    StackDisk disk;
    ReedSolomonBlockDevice rs(disk, 255, 1);

    auto data_size = rs.dataSize();
    std::array<uint8_t, 512> data_buffer;
    std::fill(data_buffer.begin(), data_buffer.begin() + data_size, std::uint8_t { 0x7E });
    static_vector<uint8_t> data(data_buffer.data(), data_buffer.size(), data_size);

    ASSERT_TRUE(rs.formatBlock(0).has_value());
    ASSERT_TRUE(rs.writeBlock(data, DataLocation(0, 0)).has_value());

    std::array<uint8_t, 512> raw_buffer;
    static_vector<uint8_t> raw(raw_buffer.data(), raw_buffer.size());
    raw.resize(rs.rawBlockSize());
    auto read_res = disk.read(0, rs.rawBlockSize(), raw);
    ASSERT_TRUE(read_res.has_value());

    // corrupt one byte completely
    raw[120] = std::uint8_t { 0x00 };
    auto write_ret = disk.write(0, raw);
    ASSERT_TRUE(write_ret.has_value());

    std::array<uint8_t, 512> read_buffer;
    static_vector<uint8_t> fixed(read_buffer.data(), read_buffer.size());
    auto read_ret = rs.readBlock({ 0, 0 }, data_size, fixed);
    ASSERT_TRUE(read_ret.has_value());

    for (size_t i = 0; i < data_size; i++) {
        EXPECT_EQ(fixed[i], data[i]) << "Byte mismatch after single byte corruption at " << i;
    }
}

TEST(ReedSolomonBlockDevice, DoubleByteError)
{
    StackDisk disk;
    ReedSolomonBlockDevice rs(disk, 255, 2);

    auto data_size = rs.dataSize();
    std::array<uint8_t, 512> data_buffer;
    std::fill(data_buffer.begin(), data_buffer.begin() + data_size, static_cast<std::uint8_t>(0xAB));
    static_vector<uint8_t> data(data_buffer.data(), data_buffer.size(), data_size);

    ASSERT_TRUE(rs.formatBlock(0).has_value());
    ASSERT_TRUE(rs.writeBlock(data, DataLocation(0, 0)).has_value());

    std::array<uint8_t, 512> raw_buffer;
    static_vector<uint8_t> raw(raw_buffer.data(), raw_buffer.size());
    raw.resize(rs.rawBlockSize());
    auto read_res = disk.read(0, rs.rawBlockSize(), raw);
    ASSERT_TRUE(read_res.has_value());

    // corrupt two bytes
    raw[10] = std::uint8_t { 0xEE };
    raw[200] = std::uint8_t { 0x44 };
    auto write_ret = disk.write(0, raw);
    ASSERT_TRUE(write_ret.has_value());

    std::array<uint8_t, 512> read_buffer;
    static_vector<uint8_t> fixed(read_buffer.data(), read_buffer.size());
    auto read_ret = rs.readBlock({ 0, 0 }, data_size, fixed);
    ASSERT_TRUE(read_ret.has_value());

    for (size_t i = 0; i < data_size; i++) {
        EXPECT_EQ(fixed[i], data[i]) << "Data mismatch after two-byte corruption at " << i;
    }
}

TEST(ReedSolomonBlockDevice, TripleByteError)
{
    StackDisk disk;
    ReedSolomonBlockDevice rs(disk, 255, 3);

    auto data_size = rs.dataSize();
    std::array<uint8_t, 512> data_buffer;
    std::fill(data_buffer.begin(), data_buffer.begin() + data_size, static_cast<std::uint8_t>(0xAB));
    static_vector<uint8_t> data(data_buffer.data(), data_buffer.size(), data_size);

    ASSERT_TRUE(rs.formatBlock(0).has_value());
    ASSERT_TRUE(rs.writeBlock(data, DataLocation(0, 0)).has_value());

    std::array<uint8_t, 512> raw_buffer;
    static_vector<uint8_t> raw(raw_buffer.data(), raw_buffer.size());
    raw.resize(rs.rawBlockSize());
    auto read_res = disk.read(0, rs.rawBlockSize(), raw);
    ASSERT_TRUE(read_res.has_value());

    // corrupt three bytes
    raw[10] = std::uint8_t { 0xEE };
    raw[100] = std::uint8_t { 0x61 };
    raw[200] = std::uint8_t { 0x44 };
    auto write_ret = disk.write(0, raw);
    ASSERT_TRUE(write_ret.has_value());

    std::array<uint8_t, 512> read_buffer;
    static_vector<uint8_t> fixed(read_buffer.data(), read_buffer.size());
    auto read_ret = rs.readBlock({ 0, 0 }, data_size, fixed);
    ASSERT_TRUE(read_ret.has_value());

    for (size_t i = 0; i < data_size; i++) {
        EXPECT_EQ(fixed[i], data[i]) << "Data mismatch after three-byte corruption at " << i;
    }
}
