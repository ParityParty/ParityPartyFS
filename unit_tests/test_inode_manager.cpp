#include "blockdevice/hamming_block_device.hpp"
#include "disk/stack_disk.hpp"
#include "inode_manager/inode_manager.hpp"
#include <gtest/gtest.h>

TEST(InodeManager, Compiles)
{
    StackDisk disk;
    HammingBlockDevice device(8, disk);
    SuperBlock sb;

    InodeManager inode_manager(device, sb);

    EXPECT_TRUE(true);
}