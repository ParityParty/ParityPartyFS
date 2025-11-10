#include "blockdevice/hamming_block_device.hpp"
#include "blockdevice/raw_block_device.hpp"
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

TEST(SuperBlockManager, WriteAndReadConsistency)
{
    StackDisk disk;
    RawBlockDevice block_device(512, disk);

    SuperBlockManager writer(block_device);
    SuperBlock sb { .total_blocks = 100,
        .free_blocks = 50,
        .total_inodes = 200,
        .free_inodes = 150,
        .block_bitmap_address = 1,
        .inode_bitmap_address = 2,
        .inode_table_address = 3,
        .journal_address = 4,
        .data_blocks_address = 5 };

    auto write_res = writer.update(sb);
    ASSERT_TRUE(write_res.has_value()) << "Failed to write superblock";

    SuperBlockManager reader(block_device);
    auto read_res = reader.get();
    ASSERT_TRUE(read_res.has_value()) << "Failed to read superblock";

    SuperBlock sb_read = read_res.value();

    EXPECT_EQ(sb.total_blocks, sb_read.total_blocks);
    EXPECT_EQ(sb.free_blocks, sb_read.free_blocks);
    EXPECT_EQ(sb.total_inodes, sb_read.total_inodes);
    EXPECT_EQ(sb.free_inodes, sb_read.free_inodes);
    EXPECT_EQ(sb.block_bitmap_address, sb_read.block_bitmap_address);
    EXPECT_EQ(sb.inode_bitmap_address, sb_read.inode_bitmap_address);
    EXPECT_EQ(sb.inode_table_address, sb_read.inode_table_address);
    EXPECT_EQ(sb.journal_address, sb_read.journal_address);
    EXPECT_EQ(sb.data_blocks_address, sb_read.data_blocks_address);
}

TEST(SuperBlockManager, WriteAndReadConsistencyWhenSuperBlockTakesMultipleBlocks)
{
    StackDisk disk;
    RawBlockDevice block_device(sizeof(SuperBlock) / 2, disk);

    SuperBlockManager writer(block_device);
    SuperBlock sb { .total_blocks = 100,
        .free_blocks = 50,
        .total_inodes = 200,
        .free_inodes = 150,
        .block_bitmap_address = 1,
        .inode_bitmap_address = 2,
        .inode_table_address = 3,
        .journal_address = 4,
        .data_blocks_address = 5 };

    auto write_res = writer.update(sb);
    ASSERT_TRUE(write_res.has_value()) << "Failed to write superblock";

    SuperBlockManager reader(block_device);
    auto read_res = reader.get();
    ASSERT_TRUE(read_res.has_value()) << "Failed to read superblock";

    SuperBlock sb_read = read_res.value();

    EXPECT_EQ(sb.total_blocks, sb_read.total_blocks);
    EXPECT_EQ(sb.free_blocks, sb_read.free_blocks);
    EXPECT_EQ(sb.total_inodes, sb_read.total_inodes);
    EXPECT_EQ(sb.free_inodes, sb_read.free_inodes);
    EXPECT_EQ(sb.block_bitmap_address, sb_read.block_bitmap_address);
    EXPECT_EQ(sb.inode_bitmap_address, sb_read.inode_bitmap_address);
    EXPECT_EQ(sb.inode_table_address, sb_read.inode_table_address);
    EXPECT_EQ(sb.journal_address, sb_read.journal_address);
    EXPECT_EQ(sb.data_blocks_address, sb_read.data_blocks_address);
}