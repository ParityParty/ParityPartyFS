#include "bitmap/bitmap.hpp"
#include "blockdevice/hamming_block_device.hpp"
#include "disk/stack_disk.hpp"

#include <algorithm>
#include <blockdevice/raw_block_device.hpp>
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

    ASSERT_TRUE(disk.write(0, { static_cast<std::byte>(0x01) }).has_value());

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

    auto read_ret = disk.read(1, 1);
    ASSERT_TRUE(read_ret.has_value());
    EXPECT_EQ(static_cast<std::byte>(1), read_ret.value()[0]);
}

TEST(Bitmap, SetBitGetBit)
{
    StackDisk disk;
    RawBlockDevice device(256, disk);
    Bitmap bm(device, 0, 512 * 8); // Two blocks of bitmap

    ASSERT_TRUE(disk.write(0, { 512, static_cast<std::byte>(0) }));

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

    ASSERT_TRUE(disk.write(3 * 256, { 512, std::byte { 0 } }));

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

    ASSERT_TRUE(disk.write(0, { 512, std::byte { 0xff } }));
    auto ret = bm.getFirstEq(false);
    ASSERT_FALSE(ret.has_value());
    ASSERT_EQ(ret.error(), FsError::NotFound);
}

TEST(Bitmap, FindFirst)
{
    StackDisk disk;
    RawBlockDevice device(256, disk);
    Bitmap bm(device, 0, 512 * 8); // Two blocks of bitmap

    ASSERT_TRUE(disk.write(0, { 512, std::byte { 0xff } }));
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

    ASSERT_TRUE(disk.write(4 * 256, { 512, std::byte { 0x00 } }));
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

    ASSERT_TRUE(disk.write(4 * 256, { 512, std::byte { 0x00 } }));
    ASSERT_TRUE(bm.setAll(true).has_value());

    for (int i = 0; i < 500 * 8; i++) {
        auto ret = bm.getBit(i);
        ASSERT_TRUE(ret.has_value());
        EXPECT_EQ(ret.value(), true) << "Mismatch at index: " << i;
    }
}
