#include "blockdevice/hamming_block_device.hpp"
#include "common/static_vector.hpp"
#include "disk/stack_disk.hpp"
#include <array>
#include <gtest/gtest.h>
#include <random>

// Helper: flip a random bit in the StackDisk's memory
void flipBit(StackDisk& disk, size_t bitIndex)
{
    size_t byteIndex = bitIndex / 8;
    size_t bitInByte = bitIndex % 8;

    std::array<uint8_t, 1> read_buffer;
    static_vector<uint8_t> bytes(read_buffer.data(), 1);
    auto readRes = disk.read(byteIndex, 1, bytes);
    ASSERT_TRUE(readRes.has_value());

    bytes[0] ^= static_cast<std::uint8_t>(1 << bitInByte);

    auto writeRes = disk.write(byteIndex, bytes);
    ASSERT_TRUE(writeRes.has_value());
}

// Helper: generate random bit index within disk size
size_t randomBit(size_t maxBits)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, maxBits - 1);
    return dist(gen);
}

TEST(HammingBlockDevice, BasicWriteRead)
{
    StackDisk disk;
    HammingBlockDevice hbd(4, disk); // 2^4 = 16 bytes per block

    std::array<uint8_t, 16> data_buffer = { std::uint8_t('h'), std::uint8_t('e'), std::uint8_t('l'),
        std::uint8_t('l'), std::uint8_t('o') };
    static_vector<uint8_t> data(data_buffer.data(), data_buffer.size(), 5);
    DataLocation loc(0, 0);

    auto write_res = hbd.writeBlock(data, loc);
    ASSERT_TRUE(write_res.has_value());

    std::array<uint8_t, 16> read_buffer;
    static_vector<uint8_t> read_data(read_buffer.data(), read_buffer.size());
    auto read_res = hbd.readBlock(loc, data.size(), read_data);
    ASSERT_TRUE(read_res.has_value());

    ASSERT_EQ(read_data.size(), data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        EXPECT_EQ(data[i], read_data[i]);
    }
}

TEST(HammingBlockDevice, SingleBitErrorIsCorrected)
{
    StackDisk disk;
    HammingBlockDevice hbd(4, disk);
    DataLocation loc(0, 0);

    std::array<uint8_t, 16> data_buffer
        = { std::uint8_t('s'), std::uint8_t('l'), std::uint8_t('a'), std::uint8_t('y') };
    static_vector<uint8_t> data(data_buffer.data(), data_buffer.size(), 4);
    auto write_res = hbd.writeBlock(data, loc);
    ASSERT_TRUE(write_res.has_value());

    // flip one random bit
    size_t totalBits = hbd.dataSize() * 8;
    size_t bitToFlip = randomBit(totalBits);
    flipBit(disk, bitToFlip);

    std::array<uint8_t, 16> read_buffer;
    static_vector<uint8_t> read_data(read_buffer.data(), read_buffer.size());
    auto read_res = hbd.readBlock(loc, data.size(), read_data);
    ASSERT_TRUE(read_res.has_value());

    for (size_t i = 0; i < data.size(); ++i) {
        EXPECT_EQ(read_data[i], data[i]) << "Mismatch after correction at byte " << i;
    }
}

TEST(HammingBlockDevice, DoubleBitErrorTriggersFailure)
{
    StackDisk disk;
    HammingBlockDevice hbd(4, disk);
    DataLocation loc(0, 0);

    std::array<uint8_t, 16> data_buffer
        = { std::uint8_t('s'), std::uint8_t('l'), std::uint8_t('a'), std::uint8_t('y') };
    static_vector<uint8_t> data(data_buffer.data(), data_buffer.size(), 4);
    auto write_res = hbd.writeBlock(data, loc);
    ASSERT_TRUE(write_res.has_value());

    // flip two random bits
    size_t totalBits = hbd.dataSize() * 8;
    size_t bit1 = randomBit(totalBits);
    size_t bit2 = randomBit(totalBits);
    while (bit2 == bit1)
        bit2 = randomBit(totalBits);

    flipBit(disk, bit1);
    flipBit(disk, bit2);

    std::array<uint8_t, 16> read_buffer;
    static_vector<uint8_t> read_data(read_buffer.data(), read_buffer.size());
    auto read_res = hbd.readBlock(loc, data.size(), read_data);
    ASSERT_FALSE(read_res.has_value());
    EXPECT_EQ(read_res.error(), FsError::BlockDevice_CorrectionError);
}

TEST(HammingBlockDevice, MultipleRandomSingleBitCorrections)
{
    for (int i = 0; i < 10; ++i) {
        StackDisk disk;
        HammingBlockDevice hbd(4, disk);
        DataLocation loc(0, 0);

        std::string msg = "Round" + std::to_string(i);
        std::array<uint8_t, 16> data_buffer;
        for (size_t j = 0; j < msg.size(); j++)
            data_buffer[j] = static_cast<std::uint8_t>(msg[j]);
        static_vector<uint8_t> data(data_buffer.data(), data_buffer.size(), msg.size());

        auto write_res = hbd.writeBlock(data, loc);
        ASSERT_TRUE(write_res.has_value());

        size_t totalBits = hbd.dataSize() * 8;
        size_t bit = randomBit(totalBits);

        flipBit(disk, bit);

        std::array<uint8_t, 16> read_buffer;
        static_vector<uint8_t> read_data(read_buffer.data(), read_buffer.size());
        auto read_res = hbd.readBlock(loc, data.size(), read_data);
        ASSERT_TRUE(read_res.has_value());

        std::string decoded(
            reinterpret_cast<const char*>(read_data.data()), read_data.size());
        ASSERT_EQ(decoded, msg);
    }
}
