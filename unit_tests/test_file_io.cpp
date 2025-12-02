#include "block_manager/block_manager.hpp"
#include "blockdevice/raw_block_device.hpp"
#include "disk/stack_disk.hpp"
#include "file_io/file_io.hpp"
#include "inode_manager/inode_manager.hpp"
#include <gtest/gtest.h>

TEST(FileIO, Compiles)
{
    StackDisk disk;
    RawBlockDevice block_device(32, disk);
    BlockManager block_manager(0, 512, block_device);
    InodeManager inode_manager(block_device, *(new SuperBlock()));
    FileIO file_io(block_device, block_manager, inode_manager);

    EXPECT_TRUE(true);
}
TEST(FileIO, WritesAndReadsDirectBlocks)
{
    StackDisk disk;
    RawBlockDevice block_device(128, disk);
    BlockManager block_manager(16, 1024, block_device);
    SuperBlock superblock { .total_inodes = 1, .inode_bitmap_address = 0, .inode_table_address = 1, .block_size = 128 };
    InodeManager inode_manager(block_device, superblock);
    FileIO file_io(block_device, block_manager, inode_manager);

    Inode inode {};
    inode.file_size = 0;
    // Assume formatted bitmap (1 means free)
    auto disk_write_res = disk.write(0, std::vector<uint8_t>{0xFF});
    ASSERT_TRUE(disk_write_res.has_value()) << "Failed to write to disk: " << toString(disk_write_res.error());
    auto inode_index = inode_manager.create(inode);
    ASSERT_TRUE(inode_index.has_value()) << "Failed to create inode: " << toString(inode_index.error());

    std::vector<uint8_t> data(block_device.dataSize() * 12);

    for (size_t i = 0; i < data.size(); ++i)
        data[i] = uint8_t(i % 251);

    auto write_res = file_io.writeFile(inode_index.value(), inode, 0, data);
    ASSERT_TRUE(write_res.has_value()) << "writeFile failed: " << toString(write_res.error());

    ASSERT_NE(inode.direct_blocks[0], 0);
    ASSERT_NE(inode.direct_blocks[1], 0);
    ASSERT_NE(inode.direct_blocks[1], inode.direct_blocks[0]);

    auto read_res = file_io.readFile(inode_index.value(), inode, 0, data.size());
    ASSERT_TRUE(read_res.has_value()) << "readFile failed";

    auto out = read_res.value();

    ASSERT_EQ(out, data);
}

TEST(FileIO, WritesAndReadsUndirectBlocks)
{
    StackDisk disk;
    RawBlockDevice block_device(128, disk);
    BlockManager block_manager(16, 1024, block_device);
    SuperBlock superblock { .total_inodes = 1, .inode_bitmap_address = 0, .inode_table_address = 1, .block_size = 128 };
    InodeManager inode_manager(block_device, superblock);
    FileIO file_io(block_device, block_manager, inode_manager);

    Inode inode {};
    inode.file_size = 0;
    // Assume formatted bitmap (1 means free)
    auto disk_write_res = disk.write(0, std::vector<uint8_t>{0xFF});
    ASSERT_TRUE(disk_write_res.has_value()) << "Failed to write to disk: " << toString(disk_write_res.error());
    auto inode_index = inode_manager.create(inode);
    ASSERT_TRUE(inode_index.has_value()) << "Failed to create inode: " << toString(inode_index.error());

    std::vector<uint8_t> data(block_device.dataSize() * 12
        + block_device.dataSize() * (block_device.dataSize() / sizeof(block_index_t)));

    for (size_t i = 0; i < data.size(); ++i)
        data[i] = uint8_t(i % 253);

    auto write_res = file_io.writeFile(inode_index.value(), inode, 0, data);
    ASSERT_TRUE(write_res.has_value()) << "writeFile failed: " << toString(write_res.error());

    ASSERT_NE(inode.direct_blocks[0], 0);
    ASSERT_NE(inode.direct_blocks[1], 0);
    ASSERT_NE(inode.direct_blocks[1], inode.direct_blocks[0]);

    auto read_res = file_io.readFile(inode_index.value(), inode, 0, data.size());
    ASSERT_TRUE(read_res.has_value()) << "readFile failed";

    auto out = read_res.value();

    ASSERT_EQ(out.size(), data.size());
    ASSERT_EQ(out, data);
}

TEST(FileIO, WritesAndReadsDoublyUndirectBlocks)
{
    StackDisk disk;
    RawBlockDevice block_device(128, disk);
    BlockManager block_manager(16, 2048, block_device);
    SuperBlock superblock { .total_inodes = 1, .inode_bitmap_address = 0, .inode_table_address = 1, .block_size = 128 };
    InodeManager inode_manager(block_device, superblock);
    FileIO file_io(block_device, block_manager, inode_manager);

    Inode inode {};
    inode.file_size = 0;
    // Assume formatted bitmap (1 means free)
    auto disk_write_res = disk.write(0, std::vector<uint8_t>{0xFF});
    ASSERT_TRUE(disk_write_res.has_value()) << "Failed to write to disk: " << toString(disk_write_res.error());
    auto inode_index = inode_manager.create(inode);
    ASSERT_TRUE(inode_index.has_value()) << "Failed to create inode: " << toString(inode_index.error());

    auto indexes_per_block = block_device.dataSize() / sizeof(block_index_t);
    std::vector<uint8_t> data(
        block_device.dataSize() * (12 + indexes_per_block + indexes_per_block * indexes_per_block));

    for (size_t i = 0; i < data.size(); ++i)
        data[i] = uint8_t(i % 227);

    auto write_res = file_io.writeFile(inode_index.value(), inode, 0, data);
    ASSERT_TRUE(write_res.has_value()) << "writeFile failed: " << toString(write_res.error());

    ASSERT_NE(inode.direct_blocks[0], 0);
    ASSERT_NE(inode.direct_blocks[1], 0);
    ASSERT_NE(inode.direct_blocks[1], inode.direct_blocks[0]);

    auto read_res = file_io.readFile(inode_index.value(), inode, 0, data.size());
    ASSERT_TRUE(read_res.has_value()) << "readFile failed";

    auto out = read_res.value();

    ASSERT_EQ(out.size(), data.size());
    ASSERT_EQ(out, data);
}

// TODO : Fix this test
// TEST(FileIO, WritesAndReadsTreblyUndirectBlocks)
// {
//     StackDisk disk;
//     RawBlockDevice block_device(128, disk);
//     BlockManager block_manager(16, 160000000, block_device);
//     SuperBlock superblock { .total_inodes = 1, .inode_bitmap_address = 0, .inode_table_address = 1, .block_size = 128 };
//     InodeManager inode_manager(block_device, superblock);
//     FileIO file_io(block_device, block_manager, inode_manager);
//
//     Inode inode {};
//     inode.file_size = 0;
//     // Assume formatted bitmap (1 means free)
//     auto disk_write_res = disk.write(0, std::vector<uint8_t>{0xFF});
//     ASSERT_TRUE(disk_write_res.has_value()) << "Failed to write to disk: " << toString(disk_write_res.error());
//     auto inode_index = inode_manager.create(inode);
//     ASSERT_TRUE(inode_index.has_value()) << "Failed to create inode: " << toString(inode_index.error());
//
//     auto indexes_per_block = block_device.dataSize() / sizeof(block_index_t);
//     std::vector<uint8_t> data(block_device.dataSize()
//         * (12 + indexes_per_block + indexes_per_block * indexes_per_block
//             + indexes_per_block * indexes_per_block * indexes_per_block));
//
//     for (size_t i = 0; i < data.size(); ++i)
//         data[i] = uint8_t(i % 231);
//
//     auto write_res = file_io.writeFile(inode_index.value(), inode, 0, data);
//     ASSERT_TRUE(write_res.has_value()) << "writeFile failed: " << toString(write_res.error());
//
//     ASSERT_NE(inode.direct_blocks[0], 0);
//     ASSERT_NE(inode.direct_blocks[1], 0);
//     ASSERT_NE(inode.direct_blocks[1], inode.direct_blocks[0]);
//
//     auto read_res = file_io.readFile(inode_index.value(), inode, 0, data.size());
//     ASSERT_TRUE(read_res.has_value()) << "readFile failed";
//
//     auto out = read_res.value();
//
//     ASSERT_EQ(out.size(), data.size());
//     ASSERT_EQ(out, data);
// }
