#include "disk/stack_disk.hpp"
#include "blockdevice/rs_block_device.hpp"

#include <gtest/gtest.h>
#include <random>

TEST(ReedSolomonBlockDevice, BasicReadWrite)
{
    StackDisk disk;
    ReedSolomonBlockDevice rs(disk);

    auto data_size = rs.dataSize();
    std::vector<std::byte> data(data_size, static_cast<std::byte>(0xAB));

    ASSERT_TRUE(rs.formatBlock(0).has_value());
    ASSERT_TRUE(rs.writeBlock(data, DataLocation(0, 0)).has_value());

    auto read_ret = rs.readBlock({0, 0}, data_size);
    ASSERT_TRUE(read_ret.has_value());
    auto read_data = read_ret.value();

    for (size_t i = 0; i < data_size; i++) {
        EXPECT_EQ(data[i], read_data[i]) << "Mismatch at byte " << i;
    }
}

TEST(ReedSolomonBlockDevice, SingleBitFlip)
{
    StackDisk disk;
    ReedSolomonBlockDevice rs(disk);

    auto data_size = rs.dataSize();
    ASSERT_TRUE(rs.formatBlock(0).has_value());

    std::vector<std::byte> data(data_size, static_cast<std::byte>(0xAB));

    ASSERT_TRUE(rs.writeBlock(data, DataLocation(0, 0)).has_value());

    auto raw = disk.read(0, rs.rawBlockSize());
    ASSERT_TRUE(raw.has_value());
    auto bytes = raw.value();
    bytes[42] = std::byte(static_cast<uint8_t>(bytes[100]) ^ (1 << 3));
    auto write_res = disk.write(0, bytes);
    ASSERT_TRUE(write_res.has_value());

    auto read_ret = rs.readBlock({0, 0}, data_size);
    ASSERT_TRUE(read_ret.has_value());
    auto corrected = read_ret.value();

    for (size_t i = 0; i < data_size; i++) {
        EXPECT_EQ(corrected[i], data[i]) << "Data mismatch after single bit flip at " << i;
    }
}

TEST(ReedSolomonBlockDevice, SingleByteError)
{
    StackDisk disk;
    ReedSolomonBlockDevice rs(disk);

    auto data_size = rs.dataSize();
    std::vector<std::byte> data(data_size, std::byte{0x7E});

    ASSERT_TRUE(rs.formatBlock(0).has_value());
    ASSERT_TRUE(rs.writeBlock(data, DataLocation(0, 0)).has_value());

    auto raw = disk.read(0, rs.rawBlockSize());
    ASSERT_TRUE(raw.has_value());
    auto bytes = raw.value();

    // corrupt one byte completely
    bytes[120] = std::byte{0x00};
    disk.write(0, bytes);

    auto read_ret = rs.readBlock({0, 0}, data_size);
    ASSERT_TRUE(read_ret.has_value());
    auto fixed = read_ret.value();

    for (size_t i = 0; i < data_size; i++) {
        EXPECT_EQ(fixed[i], data[i]) << "Byte mismatch after single byte corruption at " << i;
    }
}

TEST(ReedSolomonBlockDevice, DoubleByteError)
{
    StackDisk disk;
    ReedSolomonBlockDevice rs(disk);

    auto data_size = rs.dataSize();
    std::vector<std::byte> data(data_size, static_cast<std::byte>(0xAB));

    ASSERT_TRUE(rs.formatBlock(0).has_value());
    ASSERT_TRUE(rs.writeBlock(data, DataLocation(0, 0)).has_value());

    auto raw = disk.read(0, rs.rawBlockSize());
    ASSERT_TRUE(raw.has_value());
    auto bytes = raw.value();

    // corrupt two bytes
    bytes[10] = std::byte{0xEE};
    bytes[200] = std::byte{0x44};
    disk.write(0, bytes);

    auto read_ret = rs.readBlock({0, 0}, data_size);
    ASSERT_TRUE(read_ret.has_value());
    auto fixed = read_ret.value();

    for (size_t i = 0; i < data_size; i++) {
        EXPECT_EQ(fixed[i], data[i]) << "Data mismatch after two-byte corruption at " << i;
    }
}
