#include "disk/stack_disk.hpp"
#include "blockdevice/hamming_block_device.hpp"
#include <gtest/gtest.h>
#include <random>

// Helper: flip a random bit in the StackDisk's memory
void flipBit(StackDisk& disk, size_t bitIndex) {
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
size_t randomBit(size_t maxBits) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, maxBits - 1);
    return dist(gen);
}

TEST(HammingBlockDevice, BasicWriteRead)
{
    StackDisk disk;
    HammingBlockDevice hbd(4, disk); // 2^4 = 16 bytes per block

    std::vector<std::byte> data = {std::byte('h'), std::byte('e'), std::byte('l'), std::byte('l'), std::byte('o')};
    DataLocation loc(0, 0);

    auto write_res = hbd.writeBlock(data, loc);
    ASSERT_TRUE(write_res.has_value());

    auto read_res = hbd.readBlock(loc, data.size());
    ASSERT_TRUE(read_res.has_value());
    auto read_data = read_res.value();

    ASSERT_EQ(read_data.size(), data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        EXPECT_EQ(read_data[i], data[i]);
    }
}

TEST(HammingBlockDevice, SingleBitErrorIsCorrected)
{
    StackDisk disk;
    HammingBlockDevice hbd(4, disk);
    DataLocation loc(0, 0);

    std::vector<std::byte> data = {std::byte('s'), std::byte('l'), std::byte('a'), std::byte('y')};
    auto write_res = hbd.writeBlock(data, loc);
    ASSERT_TRUE(write_res.has_value());

    // flip one random bit
    size_t totalBits = hbd.rawBlockSize() * 8;
    size_t bitToFlip = randomBit(totalBits);
    flipBit(disk, bitToFlip);

    auto read_res = hbd.readBlock(loc, data.size());
    ASSERT_TRUE(read_res.has_value());
    auto read_data = read_res.value();

    for (size_t i = 0; i < data.size(); ++i) {
        EXPECT_EQ(read_data[i], data[i]) << "Mismatch after correction at byte " << i;
    }
}

TEST(HammingBlockDevice, DoubleBitErrorTriggersFailure)
{
    StackDisk disk;
    HammingBlockDevice hbd(4, disk);
    DataLocation loc(0, 0);

    std::vector<std::byte> data = {std::byte('s'), std::byte('l'), std::byte('a'), std::byte('y')};
    auto write_res = hbd.writeBlock(data, loc);
    ASSERT_TRUE(write_res.has_value());

    // flip two random bits
    size_t totalBits = hbd.rawBlockSize() * 8;
    size_t bit1 = randomBit(totalBits);
    size_t bit2 = randomBit(totalBits);
    while (bit2 == bit1) bit2 = randomBit(totalBits);

    flipBit(disk, bit1);
    flipBit(disk, bit2);

    auto read_res = hbd.readBlock(loc, data.size());
    ASSERT_FALSE(read_res.has_value());
    EXPECT_EQ(read_res.error(), DiskError::CorrectionError);
}

TEST(HammingBlockDevice, MultipleRandomSingleBitCorrections)
{
    for (int i = 0; i < 10; ++i) {
        StackDisk disk;
        HammingBlockDevice hbd(4, disk);
        DataLocation loc(0, 0);

        std::string msg = "Round" + std::to_string(i);
        std::vector<std::byte> data;
        data.reserve(msg.size());
        for (char c : msg)
            data.push_back(static_cast<std::byte>(c));

        auto write_res = hbd.writeBlock(data, loc);
        ASSERT_TRUE(write_res.has_value());

        size_t totalBits = hbd.rawBlockSize() * 8;
        size_t bit = randomBit(totalBits);

        flipBit(disk, bit);

        auto read_res = hbd.readBlock(loc, data.size());
        ASSERT_TRUE(read_res.has_value());

        std::string decoded(reinterpret_cast<const char*>(read_res.value().data()), read_res.value().size());
        ASSERT_EQ(decoded, msg);
    }
}
