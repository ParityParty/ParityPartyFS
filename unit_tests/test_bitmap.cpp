#include "bitmap/bitmap.hpp"
#include "blockdevice/hamming_block_device.hpp"
#include "disk/stack_disk.hpp"

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
