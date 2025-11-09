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

    ASSERT_TRUE(device.formatBlock(0).has_value());
    ASSERT_TRUE(device.formatBlock(1).has_value());

    ASSERT_FALSE(bm.getBit(22).value());
}

TEST(Bitmap, GetBit_GetsCorrectBit)
{
    StackDisk disk;
    RawBlockDevice device(256, disk);
    Bitmap bm(device, 0, 512 * 8); // Two blocks of bitmap

    ASSERT_TRUE(device.formatBlock(0).has_value());
    ASSERT_TRUE(device.formatBlock(1).has_value());

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

    ASSERT_TRUE(device.formatBlock(0).has_value());
    ASSERT_TRUE(device.formatBlock(1).has_value());

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

    ASSERT_TRUE(device.formatBlock(0).has_value());
    ASSERT_TRUE(device.formatBlock(1).has_value());

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
