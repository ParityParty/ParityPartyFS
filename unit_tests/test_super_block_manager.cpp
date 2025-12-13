#include "blockdevice/hamming_block_device.hpp"
#include "blockdevice/raw_block_device.hpp"
#include "common/static_vector.hpp"
#include "disk/stack_disk.hpp"
#include "super_block_manager/super_block_manager.hpp"

#include <array>
#include <gtest/gtest.h>

TEST(SuperBlock, Compiles)
{
    StackDisk disk;

    SuperBlockManager superBlockManager(disk);

    EXPECT_TRUE(true);
}

TEST(SuperBlockManager, WriteAndReadConsistency)
{
    StackDisk disk;

    SuperBlockManager writer(disk);

    SuperBlock sb { .total_blocks = 100,
        .total_inodes = 200,
        .block_bitmap_address = 1,
        .inode_bitmap_address = 2,
        .inode_table_address = 3,
        .journal_address = 4,
        .first_data_blocks_address = 6,
        .last_data_block_address = 1024,
        .block_size = 50,
        .crc_polynomial = 348975,
        .rs_correctable_bytes = 5,
        .ecc_type = ECCType::Crc };

    auto write_res = writer.put(sb);
    ASSERT_TRUE(write_res.has_value())
        << "Failed to write superblock:" << toString(write_res.error());

    SuperBlockManager reader(disk);

    auto read_res = reader.get();
    ASSERT_TRUE(read_res.has_value())
        << "Failed to read superblock: " << toString(read_res.error());

    EXPECT_EQ(sb.total_blocks, read_res.value().total_blocks);
    EXPECT_EQ(sb.total_inodes, read_res.value().total_inodes);
    EXPECT_EQ(sb.block_bitmap_address, read_res.value().block_bitmap_address);
    EXPECT_EQ(sb.inode_bitmap_address, read_res.value().inode_bitmap_address);
    EXPECT_EQ(sb.inode_table_address, read_res.value().inode_table_address);
    EXPECT_EQ(sb.journal_address, read_res.value().journal_address);
    EXPECT_EQ(sb.first_data_blocks_address, read_res.value().first_data_blocks_address);
    EXPECT_EQ(sb.last_data_block_address, read_res.value().last_data_block_address);
    EXPECT_EQ(sb.block_size, read_res.value().block_size);
    EXPECT_EQ(sb.crc_polynomial, read_res.value().crc_polynomial);
    EXPECT_EQ(sb.rs_correctable_bytes, read_res.value().rs_correctable_bytes);
    EXPECT_EQ(sb.ecc_type, read_res.value().ecc_type);
}

TEST(SuperBlockManager, GetFreeBlocksIndexes)
{
    StackDisk disk;

    SuperBlockManager writer(disk);
    unsigned int block_size = 128;

    SuperBlock sb { .total_blocks = 100,
        .total_inodes = 200,
        .block_bitmap_address = 1,
        .inode_bitmap_address = 2,
        .inode_table_address = 3,
        .journal_address = 4,
        .first_data_blocks_address = 6,
        .last_data_block_address = 1024,
        .block_size = block_size,
        .crc_polynomial = 348975,
        .rs_correctable_bytes = 5,
        .ecc_type = ECCType::Crc };

    auto write_res = writer.put(sb);
    ASSERT_TRUE(write_res.has_value())
        << "Failed to write superblock:" << toString(write_res.error());

    auto indexes_res = writer.getFreeBlocksIndexes();
    ASSERT_TRUE(indexes_res.has_value())
        << "Error when getting indexes: " << toString(indexes_res.error());

    RawBlockDevice raw_block_device(block_size, disk);
    std::array<uint8_t, 1024> zero_buffer;
    std::fill(zero_buffer.begin(), zero_buffer.begin() + block_size, 0);
    static_vector<uint8_t> zero_data(zero_buffer.data(), zero_buffer.size(), block_size);
    for (block_index_t i = indexes_res->start_block; i < indexes_res->end_block; i++) {
        // Clear all non - superblock indexes
        ASSERT_TRUE(
            raw_block_device.writeBlock(zero_data, DataLocation(i, 0)))
            << "Failed to write to block device";
    }
    SuperBlockManager reader(disk);

    auto read_res = reader.get();
    ASSERT_TRUE(read_res.has_value())
        << "Failed to read superblock: " << toString(read_res.error());

    EXPECT_EQ(sb.total_blocks, read_res.value().total_blocks);
    EXPECT_EQ(sb.total_inodes, read_res.value().total_inodes);
    EXPECT_EQ(sb.block_bitmap_address, read_res.value().block_bitmap_address);
    EXPECT_EQ(sb.inode_bitmap_address, read_res.value().inode_bitmap_address);
    EXPECT_EQ(sb.inode_table_address, read_res.value().inode_table_address);
    EXPECT_EQ(sb.journal_address, read_res.value().journal_address);
    EXPECT_EQ(sb.first_data_blocks_address, read_res.value().first_data_blocks_address);
    EXPECT_EQ(sb.last_data_block_address, read_res.value().last_data_block_address);
    EXPECT_EQ(sb.block_size, read_res.value().block_size);
    EXPECT_EQ(sb.crc_polynomial, read_res.value().crc_polynomial);
    EXPECT_EQ(sb.rs_correctable_bytes, read_res.value().rs_correctable_bytes);
    EXPECT_EQ(sb.ecc_type, read_res.value().ecc_type);
}
