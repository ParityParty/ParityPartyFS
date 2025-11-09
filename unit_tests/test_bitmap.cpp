#include "bitmap/bitmap.hpp"
#include "blockdevice/hamming_block_device.hpp"
#include "disk/stack_disk.hpp"

#include <gtest/gtest.h>

TEST(Bitmap, Compiles)
{
    StackDisk disk;
    HammingBlockDevice device(8, disk);
    Bitmap bm(device, 0, 1024);

    ASSERT_TRUE(true);
}
