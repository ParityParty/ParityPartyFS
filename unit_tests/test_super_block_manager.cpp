#include "../lib/super_block_manager/include/super_block_manager/super_block_manager.hpp"
#include "blockdevice/hamming_block_device.hpp"
#include "disk/stack_disk.hpp"
#include "super_block_manager/super_block_manager.hpp"

#include <gtest/gtest.h>

TEST(SuperBlock, Compiles)
{
    StackDisk disk;
    HammingBlockDevice device(8, disk);

    SuperBlockManager manager(device);

    EXPECT_TRUE(true);
}
