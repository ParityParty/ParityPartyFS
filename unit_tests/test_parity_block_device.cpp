#include "blockdevice/parity_block_device.hpp"
#include "disk/stack_disk.hpp"
#include <bit>
#include <gtest/gtest.h>

TEST(ParityBlockDevice, BasicReadWrite)
{
    StackDisk disk;
    ParityBlockDevice parity(256, disk);

    auto data_size = parity.dataSize();
    std::vector<std::uint8_t> data(data_size, static_cast<std::uint8_t>(0xAA));

    ASSERT_TRUE(parity.formatBlock(0).has_value());
    ASSERT_TRUE(parity.writeBlock(data, DataLocation(0, 0)).has_value());

    auto read_ret = parity.readBlock({ 0, 0 }, data_size);
    ASSERT_TRUE(read_ret.has_value());

    const auto& read_data = read_ret.value();
    ASSERT_EQ(read_data.size(), data_size);

    for (size_t i = 0; i < data_size; i++)
        EXPECT_EQ(data[i], read_data[i]);
}

TEST(ParityBlockDevice, DetectsSingleBitFlip)
{
    StackDisk disk;
    ParityBlockDevice parity(256, disk);

    auto data_size = parity.dataSize();
    std::vector<std::uint8_t> data(data_size, static_cast<std::uint8_t>(0x55));

    ASSERT_TRUE(parity.formatBlock(0).has_value());
    ASSERT_TRUE(parity.writeBlock(data, DataLocation(0, 0)).has_value());

    auto raw = disk.read(0, parity.rawBlockSize());
    ASSERT_TRUE(raw.has_value());
    raw.value()[10] ^= static_cast<std::uint8_t>(4);
    ASSERT_TRUE(disk.write(0, raw.value()).has_value());

    auto read_ret = parity.readBlock({ 0, 0 }, data_size);
    EXPECT_FALSE(read_ret.has_value()) << "ParityBlockDevice should detect bit flip error";
}
