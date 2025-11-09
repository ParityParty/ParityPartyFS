#include "disk/stack_disk.hpp"
#include "blockdevice/rs_block_device.hpp"
#include <gtest/gtest.h>
#include <random>

// Helper: flip a random bit in the StackDisk's memory
void _flipBit(StackDisk& disk, size_t bitIndex) {
    size_t byteIndex = bitIndex / 8;
    size_t bitInByte = bitIndex % 8;

    auto readRes = disk.read(byteIndex, 1);
    ASSERT_TRUE(readRes.has_value());

    auto bytes = readRes.value();
    bytes[0] ^= static_cast<std::byte>(1 << bitInByte);

    auto writeRes = disk.write(byteIndex, bytes);
    ASSERT_TRUE(writeRes.has_value());
}

// Helper: generate random bit index within disk size
size_t _randomBit(size_t maxBits) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, maxBits - 1);
    return dist(gen);
}

TEST(ReedSolomonBlockDevice, BasicWriteRead)
{
    StackDisk disk;
ReedSolomonBlockDevice block_device(disk); // 2^4 = 16 bytes per block

    std::vector<std::byte> data = {std::byte('h'), std::byte('e'), std::byte('l'), std::byte('l'), std::byte('o')};
    DataLocation loc(0, 0);

    auto write_res = block_device.writeBlock(data, loc);
    ASSERT_TRUE(write_res.has_value());

    auto read_res = block_device.readBlock(loc, data.size());
    ASSERT_TRUE(read_res.has_value());
    auto read_data = read_res.value();

    ASSERT_EQ(read_data.size(), data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        EXPECT_EQ(read_data[i], data[i]);
    }
}

TEST(ReedSolomonBlockDevice, SingleBitErrorIsCorrected)
{
    StackDisk disk;
ReedSolomonBlockDevice block_device(disk);
    DataLocation loc(0, 0);

    std::vector<std::byte> data = {std::byte('s'), std::byte('l'), std::byte('a'), std::byte('y')};
    auto write_res = block_device.writeBlock(data, loc);
    ASSERT_TRUE(write_res.has_value());

    // flip one random bit
    size_t totalBits = block_device.dataSize() * 8;
    size_t bitToFlip = _randomBit(totalBits);
    _flipBit(disk, bitToFlip);

    auto read_res = block_device.readBlock(loc, data.size());
    ASSERT_TRUE(read_res.has_value());
    auto read_data = read_res.value();

    for (size_t i = 0; i < data.size(); ++i) {
        EXPECT_EQ(read_data[i], data[i]) << "Mismatch after correction at byte " << i;
    }
}
TEST(ReedSolomonBlockDevice, MultipleRandomSingleBitCorrections)
{
    for (int i = 0; i < 10; ++i) {


        StackDisk disk;
        ReedSolomonBlockDevice block_device(disk);
        DataLocation loc(0, 0);

        std::string msg = "Round" + std::to_string(i);
        std::vector<std::byte> data;
        data.reserve(msg.size());
        for (char c : msg)
            data.push_back(static_cast<std::byte>(c));

        auto write_res = block_device.writeBlock(data, loc);
        ASSERT_TRUE(write_res.has_value());

        size_t totalBits = msg.length() * 8 + 32;
        size_t bit = _randomBit(totalBits);

        std::cout << "Round: " << i << " Flipped: " << bit << std::endl;

        _flipBit(disk, bit);

        auto read_res = block_device.readBlock(loc, data.size());

        ASSERT_TRUE(read_res.has_value());

        std::string decoded(reinterpret_cast<const char*>(read_res.value().data()), read_res.value().size());

        ASSERT_EQ(decoded, msg);
    }
}

