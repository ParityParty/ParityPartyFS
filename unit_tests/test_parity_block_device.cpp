#include "ppfs/blockdevice/parity_block_device.hpp"
#include "ppfs/common/static_vector.hpp"
#include "ppfs/disk/stack_disk.hpp"
#include <array>
#include <bit>
#include <gtest/gtest.h>

TEST(ParityBlockDevice, BasicReadWrite)
{
    StackDisk disk;
    ParityBlockDevice parity(256, disk);

    auto data_size = parity.dataSize();
    std::array<uint8_t, 512> data_buffer;
    std::fill(
        data_buffer.begin(), data_buffer.begin() + data_size, static_cast<std::uint8_t>(0xAA));
    static_vector<uint8_t> data(data_buffer.data(), data_buffer.size(), data_size);

    ASSERT_TRUE(parity.formatBlock(0).has_value());
    ASSERT_TRUE(parity.writeBlock(data, DataLocation(0, 0)).has_value());

    std::array<uint8_t, 512> read_buffer;
    static_vector<uint8_t> read_data(read_buffer.data(), read_buffer.size());
    auto read_ret = parity.readBlock({ 0, 0 }, data_size, read_data);
    ASSERT_TRUE(read_ret.has_value());

    ASSERT_EQ(read_data.size(), data_size);

    for (size_t i = 0; i < data_size; i++)
        EXPECT_EQ(data[i], read_data[i]);
}

TEST(ParityBlockDevice, DetectsSingleBitFlip)
{
    StackDisk disk;
    ParityBlockDevice parity(256, disk);

    auto data_size = parity.dataSize();
    std::array<uint8_t, 512> data_buffer;
    std::fill(
        data_buffer.begin(), data_buffer.begin() + data_size, static_cast<std::uint8_t>(0x55));
    static_vector<uint8_t> data(data_buffer.data(), data_buffer.size(), data_size);

    ASSERT_TRUE(parity.formatBlock(0).has_value());
    ASSERT_TRUE(parity.writeBlock(data, DataLocation(0, 0)).has_value());

    std::array<uint8_t, 512> raw_buffer;
    static_vector<uint8_t> raw(raw_buffer.data(), raw_buffer.size());
    raw.resize(parity.rawBlockSize());
    auto read_res = disk.read(0, parity.rawBlockSize(), raw);
    ASSERT_TRUE(read_res.has_value());
    raw[10] ^= static_cast<std::uint8_t>(4);
    ASSERT_TRUE(disk.write(0, raw).has_value());

    std::array<uint8_t, 512> read_buffer;
    static_vector<uint8_t> read_data(read_buffer.data(), read_buffer.size());
    auto read_ret = parity.readBlock({ 0, 0 }, data_size, read_data);
    EXPECT_FALSE(read_ret.has_value()) << "ParityBlockDevice should detect bit flip error";
}
