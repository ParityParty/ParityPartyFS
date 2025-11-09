#include "blockdevice/hamming_block_device.hpp"
#include "disk/stack_disk.hpp"
#include "inode_manager/inode_manager.hpp"
#include <gtest/gtest.h>

TEST(InodeManager, Compiles)
{
    StackDisk disk;
    HammingBlockDevice device(8, disk);
    Bitmap bitmap(device, 0, 10);
    InodeManager inode_manager(bitmap, 2);

    EXPECT_TRUE(true);
}