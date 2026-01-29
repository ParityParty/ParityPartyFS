#include "ppfs/bitmap/bitmap.hpp"
#include "ppfs/blockdevice/hamming_block_device.hpp"
#include "ppfs/blockdevice/raw_block_device.hpp"
#include "ppfs/common/static_vector.hpp"
#include "ppfs/disk/stack_disk.hpp"

#include <algorithm>
#include <array>
#include <gtest/gtest.h>

TEST(Bitmap, Compiles)
{
    StackDisk disk;
    HammingBlockDevice device(8, disk);
    Bitmap bm(device, 0, 1024);

    ASSERT_TRUE(true);
}

TEST(Bitmap, GetBit_GetsBit)
{
    StackDisk disk;
    RawBlockDevice device(256, disk);
    Bitmap bm(device, 0, 512 * 8); // Two blocks of bitmap
    auto get_bit_ret = bm.getBit(22);
    ASSERT_TRUE(get_bit_ret.has_value());
    ASSERT_FALSE(get_bit_ret.value());
}

TEST(Bitmap, GetBit_GetsCorrectBit)
{
    StackDisk disk;
    RawBlockDevice device(256, disk);
    Bitmap bm(device, 0, 512 * 8); // Two blocks of bitmap

    std::array<uint8_t, 1> write_buffer = { static_cast<std::uint8_t>(0x01) };
    static_vector<uint8_t> write_data(write_buffer.data(), 1, 1);
    ASSERT_TRUE(disk.write(0, write_data).has_value());

    auto bit_ret = bm.getBit(7);
    ASSERT_TRUE(bit_ret.has_value());
    EXPECT_TRUE(bit_ret.value());
}

TEST(Bitmap, SetBit)
{
    StackDisk disk;
    RawBlockDevice device(256, disk);
    Bitmap bm(device, 0, 512 * 8); // Two blocks of bitmap

    ASSERT_TRUE(bm.setBit(15, true).has_value());

    std::array<uint8_t, 1> read_buffer;
    static_vector<uint8_t> read_data(read_buffer.data(), 1);
    auto read_ret = disk.read(1, 1, read_data);
    ASSERT_TRUE(read_ret.has_value());
    EXPECT_EQ(static_cast<std::uint8_t>(1), read_data[0]);
}

TEST(Bitmap, SetBitGetBit)
{
    StackDisk disk;
    RawBlockDevice device(256, disk);
    Bitmap bm(device, 0, 512 * 8); // Two blocks of bitmap

    std::array<uint8_t, 512> zero_buffer;
    std::fill(zero_buffer.begin(), zero_buffer.end(), std::uint8_t { 0 });
    static_vector<uint8_t> zero_data(zero_buffer.data(), 512, 512);
    ASSERT_TRUE(disk.write(0, zero_data).has_value());

    std::vector<unsigned int> true_indexes = { 1, 70, 420, 999, 1000, 1024, 1300 };
    for (auto index : true_indexes) {
        ASSERT_TRUE(bm.setBit(index, true).has_value());
    }

    for (int i = 0; i < 512 * 8; i++) {
        auto bit_ret = bm.getBit(i);
        ASSERT_TRUE(bit_ret.has_value());
        auto bit = bit_ret.value();
        auto expected = std::ranges::contains(true_indexes.begin(), true_indexes.end(), i);
        EXPECT_EQ(bit, expected) << "Index: " << i;
    }
}

TEST(Bitmap, SetBitGetBitOffset)
{
    StackDisk disk;
    RawBlockDevice device(256, disk);
    Bitmap bm(device, 3, 512 * 8); // Two blocks of bitmap

    std::array<uint8_t, 512> zero_buffer;
    std::fill(zero_buffer.begin(), zero_buffer.end(), std::uint8_t { 0 });
    static_vector<uint8_t> zero_data(zero_buffer.data(), 512, 512);
    ASSERT_TRUE(disk.write(3 * 256, zero_data).has_value());

    std::vector<unsigned int> true_indexes = { 1, 70, 420, 999, 1000, 1024, 1300 };
    for (auto index : true_indexes) {
        ASSERT_TRUE(bm.setBit(index, true).has_value());
    }

    for (int i = 0; i < 512 * 8; i++) {
        auto bit_ret = bm.getBit(i);
        ASSERT_TRUE(bit_ret.has_value());
        auto bit = bit_ret.value();
        auto expected = std::ranges::contains(true_indexes.begin(), true_indexes.end(), i);
        EXPECT_EQ(bit, expected) << "Index: " << i;
    }
}

TEST(Bitmap, FindFirst_NoSpace)
{
    StackDisk disk;
    RawBlockDevice device(256, disk);
    Bitmap bm(device, 0, 512 * 8); // Two blocks of bitmap

    std::array<uint8_t, 512> ff_buffer;
    std::fill(ff_buffer.begin(), ff_buffer.end(), std::uint8_t { 0xFF });
    static_vector<uint8_t> ff_data(ff_buffer.data(), 512, 512);
    ASSERT_TRUE(disk.write(0, ff_data).has_value());
    auto ret = bm.getFirstEq(false);
    ASSERT_FALSE(ret.has_value());
    ASSERT_EQ(ret.error(), FsError::Bitmap_NotFound);
}

TEST(Bitmap, FindFirst)
{
    StackDisk disk;
    RawBlockDevice device(256, disk);
    Bitmap bm(device, 0, 512 * 8); // Two blocks of bitmap

    std::array<uint8_t, 512> ff_buffer;
    std::fill(ff_buffer.begin(), ff_buffer.end(), std::uint8_t { 0xFF });
    static_vector<uint8_t> ff_data(ff_buffer.data(), 512, 512);
    ASSERT_TRUE(disk.write(0, ff_data).has_value());
    ASSERT_TRUE(bm.setBit(166, false).has_value());
    auto ret = bm.getFirstEq(false);
    ASSERT_TRUE(ret.has_value());
    EXPECT_EQ(ret.value(), 166);
}

TEST(Bitamp, FindFirst_true)
{
    StackDisk disk;
    RawBlockDevice device(256, disk);
    Bitmap bm(device, 4, 512 * 8); // Two blocks of bitmap

    std::array<uint8_t, 512> zero_buffer;
    std::fill(zero_buffer.begin(), zero_buffer.end(), std::uint8_t { 0 });
    static_vector<uint8_t> zero_data(zero_buffer.data(), 512, 512);
    ASSERT_TRUE(disk.write(4 * 256, zero_data).has_value());
    ASSERT_TRUE(bm.setBit(250, true).has_value());
    auto ret = bm.getFirstEq(true);
    ASSERT_TRUE(ret.has_value());
    EXPECT_EQ(ret.value(), 250);
}

TEST(Bitmap, SetAll)
{
    StackDisk disk;
    RawBlockDevice device(256, disk);
    Bitmap bm(device, 4, 500 * 8); // A bit less than two blocks of bitmap

    std::array<uint8_t, 512> zero_buffer;
    std::fill(zero_buffer.begin(), zero_buffer.end(), std::uint8_t { 0 });
    static_vector<uint8_t> zero_data(zero_buffer.data(), 512, 512);
    ASSERT_TRUE(disk.write(4 * 256, zero_data).has_value());
    ASSERT_TRUE(bm.setAll(true).has_value());

    for (int i = 0; i < 500 * 8; i++) {
        auto ret = bm.getBit(i);
        ASSERT_TRUE(ret.has_value());
        EXPECT_EQ(ret.value(), true) << "Mismatch at index: " << i;
    }
}

TEST(Bitmap, Count_allOnes)
{
    StackDisk disk;
    RawBlockDevice device(256, disk);
    Bitmap bm(device, 4, 500 * 8); // A bit less than two blocks of bitmap

    std::array<uint8_t, 512> ff_buffer;
    std::fill(ff_buffer.begin(), ff_buffer.end(), std::uint8_t { 0xFF });
    static_vector<uint8_t> ff_data(ff_buffer.data(), 512, 512);
    ASSERT_TRUE(disk.write(4 * 256, ff_data).has_value());
    auto count1 = bm.count(1);
    auto count0 = bm.count(0);
    ASSERT_TRUE(count1.has_value());
    ASSERT_TRUE(count0.has_value());

    EXPECT_EQ(count1.value(), 500 * 8);
    EXPECT_EQ(count0.value(), 0);
}

TEST(Bitmap, Count_allZeros)
{
    StackDisk disk;
    RawBlockDevice device(256, disk);
    Bitmap bm(device, 4, 500 * 8); // A bit less than two blocks of bitmap

    std::array<uint8_t, 512> zero_buffer;
    std::fill(zero_buffer.begin(), zero_buffer.end(), std::uint8_t { 0x00 });
    static_vector<uint8_t> zero_data(zero_buffer.data(), 512, 512);
    ASSERT_TRUE(disk.write(4 * 256, zero_data).has_value());
    auto count1 = bm.count(1);
    auto count0 = bm.count(0);
    ASSERT_TRUE(count1.has_value());
    ASSERT_TRUE(count0.has_value());

    EXPECT_EQ(count0.value(), 500 * 8);
    EXPECT_EQ(count1.value(), 0);
}

TEST(Bitmap, Count_checkerboard)
{
    StackDisk disk;
    RawBlockDevice device(256, disk);
    Bitmap bm(device, 4, 500 * 8); // A bit less than two blocks of bitmap

    std::array<uint8_t, 512> aa_buffer;
    std::fill(aa_buffer.begin(), aa_buffer.end(), std::uint8_t { 0xAA });
    static_vector<uint8_t> aa_data(aa_buffer.data(), 512, 512);
    ASSERT_TRUE(disk.write(4 * 256, aa_data).has_value());
    auto count1 = bm.count(1);
    auto count0 = bm.count(0);
    ASSERT_TRUE(count1.has_value());
    ASSERT_TRUE(count0.has_value());

    EXPECT_EQ(count1.value(), 500 * 8 / 2);
    EXPECT_EQ(count1.value(), count0.value());
}

TEST(Bitmap, Count_setBitChangesCount)
{
    StackDisk disk;
    RawBlockDevice device(32, disk);
    Bitmap bm(device, 0, 32 * 8);

    // Initialize with ones
    std::array<uint8_t, 32> ff_buffer;
    std::fill(ff_buffer.begin(), ff_buffer.end(), std::uint8_t { 0xFF });
    static_vector<uint8_t> ff_data(ff_buffer.data(), 32, 32);
    ASSERT_TRUE(disk.write(0, ff_data).has_value());
    auto count = bm.count(0);
    ASSERT_TRUE(count.has_value());
    EXPECT_EQ(count.value(), 0);

    // Set bit 16 to 0
    ASSERT_TRUE(bm.setBit(16, false));
    auto count2 = bm.count(0);
    ASSERT_TRUE(count2.has_value());
    EXPECT_EQ(count2.value(), 1);

    // Set bit 16 to 1
    ASSERT_TRUE(bm.setBit(16, true));
    auto count3 = bm.count(0);
    ASSERT_TRUE(count3.has_value());
    EXPECT_EQ(count3.value(), 0);
}
