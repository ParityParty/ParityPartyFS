#include "block_manager/block_manager.hpp"
#include "blockdevice/hamming_block_device.hpp"
#include "blockdevice/raw_block_device.hpp"
#include "disk/stack_disk.hpp"
#include <gtest/gtest.h>

TEST(BlockManager, Compiles)
{
    StackDisk disk;
    HammingBlockDevice device(8, disk);
    BlockManager block_manager(0, 500, device);

    EXPECT_TRUE(true);
}

TEST(BlockManager, Formats)
{
    StackDisk disk;
    RawBlockDevice device(512, disk);
    BlockManager block_manager(1, 8, device);
    ASSERT_TRUE(block_manager.format().has_value());

    auto free_ret = block_manager.numFree();
    ASSERT_TRUE(free_ret.has_value());
    EXPECT_EQ(free_ret.value(), 7); // one byte for bitmap, 7 data blocks
    auto read_ret = (disk.read(1, 1));
    ASSERT_TRUE(read_ret.has_value());
    // 7 bits free 8th unknown
    EXPECT_TRUE(
        static_cast<int>(read_ret.value()[0]) == 1 || static_cast<int>(read_ret.value()[0]) == 0);
}

TEST(BlockManager, FormatsMore)
{
    StackDisk disk;
    RawBlockDevice device(512, disk);
    BlockManager block_manager(1, 9, device);
    ASSERT_TRUE(block_manager.format().has_value());

    auto free_ret = block_manager.numFree();
    ASSERT_TRUE(free_ret.has_value());
    EXPECT_EQ(free_ret.value(), 8); // one byte for bitmap, 7 data blocks
    auto read_ret = (disk.read(1, 1));
    ASSERT_TRUE(read_ret.has_value());
    EXPECT_EQ(static_cast<int>(read_ret.value()[0]), 0);
}

TEST(BlockManager, FindsFreeEasy)
{
    StackDisk disk;
    RawBlockDevice device(512, disk);
    BlockManager block_manager(1, 9, device);
    ASSERT_TRUE(block_manager.format().has_value());
    auto free_ret = block_manager.getFree();
    ASSERT_TRUE(free_ret.has_value());
    EXPECT_EQ(free_ret.value(), 2); // Start at 1 block and one block for bitmap
}

TEST(BlockManager, FindsFreeHarder)
{
    StackDisk disk;
    RawBlockDevice device(512, disk);
    BlockManager block_manager(1, 9, device);
    ASSERT_TRUE(block_manager.format().has_value());
    auto free_ret = block_manager.getFree();
    ASSERT_TRUE(free_ret.has_value());
    EXPECT_EQ(free_ret.value(), 2); // Start at 1 block and one block for bitmap
    ASSERT_TRUE(block_manager.reserve(free_ret.value()));
    auto next_free_ret = block_manager.getFree();
    ASSERT_TRUE(next_free_ret.has_value());
    EXPECT_EQ(next_free_ret.value(), 3);
}
