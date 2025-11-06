#include "block_manager/block_manager.hpp"
#include "blockdevice/hamming_block_device.hpp"
#include "disk/stack_disk.hpp"
#include <gtest/gtest.h>

TEST(BlockManager, Compiles)
{
    StackDisk disk;
    HammingBlockDevice device(8, disk);
    Bitmap bitmap(device, 0, 10);
    BlockManager block_manager(bitmap, 2);

    EXPECT_TRUE(true);
}