#include "blockdevice/raw_block_device.hpp"
#include "disk/stack_disk.hpp"
#include "inode_manager/inode_manager.hpp"
#include <gtest/gtest.h>

TEST(InodeManager, Compiles)
{
    StackDisk disk;
    RawBlockDevice device(8, disk);
    SuperBlock sb;
    InodeManager inode_manager(device, sb);

    EXPECT_TRUE(true);
}

TEST(InodeManager, FormatsInodeTable)
{
    StackDisk disk;
    RawBlockDevice device(128, disk);
    SuperBlock superblock {
        .total_inodes = 8, .inode_bitmap_address = 0, .inode_table_address = 1, .block_size = 128
    };
    InodeManager inode_manager(device, superblock);

    auto set_disk_0s_res = disk.write(0, std::vector<std::uint8_t>(1, 0x00));
    ASSERT_TRUE(set_disk_0s_res.has_value())
        << "Failed to initialize disk: " << toString(set_disk_0s_res.error());

    auto format_res = inode_manager.format();
    ASSERT_TRUE(format_res.has_value())
        << "Failed to format inode table: " << toString(format_res.error());

    auto bitmap_data_res = disk.read(0, 1);
    ASSERT_TRUE(bitmap_data_res.has_value())
        << "Failed to read bitmap from disk: " << toString(bitmap_data_res.error());

    // After formatting, all inodes should be free (1)
    ASSERT_EQ(bitmap_data_res.value()[0], 0xFF);
}

TEST(InodeManager, CreatesAndGetsInode)
{
    StackDisk disk;
    RawBlockDevice device(128, disk);
    SuperBlock superblock {
        .total_inodes = 1, .inode_bitmap_address = 0, .inode_table_address = 1, .block_size = 128
    };
    InodeManager inode_manager(device, superblock);

    Inode inode {};
    inode.file_size = 1234;

    auto format_res = inode_manager.format();
    ASSERT_TRUE(format_res.has_value())
        << "Failed to format inode table: " << toString(format_res.error());

    auto inode_index = inode_manager.create(inode);
    ASSERT_TRUE(inode_index.has_value())
        << "Failed to create inode: " << toString(inode_index.error());

    auto read_inode_res = inode_manager.get(inode_index.value());
    ASSERT_TRUE(read_inode_res.has_value())
        << "Failed to get inode: " << toString(read_inode_res.error());

    ASSERT_EQ(read_inode_res.value().file_size, inode.file_size);
}

TEST(InodeManager, CreatesAndRemovesInode)
{
    StackDisk disk;
    RawBlockDevice device(128, disk);
    SuperBlock superblock {
        .total_inodes = 1, .inode_bitmap_address = 0, .inode_table_address = 1, .block_size = 128
    };
    InodeManager inode_manager(device, superblock);

    Inode inode {};
    inode.file_size = 1234;

    auto format_res = inode_manager.format();
    ASSERT_TRUE(format_res.has_value())
        << "Failed to format inode table: " << toString(format_res.error());

    auto inode_index = inode_manager.create(inode);
    ASSERT_TRUE(inode_index.has_value())
        << "Failed to create inode: " << toString(inode_index.error());

    auto read_inode_res = inode_manager.get(inode_index.value());
    ASSERT_TRUE(read_inode_res.has_value())
        << "Failed to get inode: " << toString(read_inode_res.error());
    ASSERT_EQ(read_inode_res.value().file_size, inode.file_size);

    auto remove_res = inode_manager.remove(inode_index.value());
    ASSERT_TRUE(remove_res.has_value())
        << "Failed to remove inode: " << toString(remove_res.error());

    auto get_res = inode_manager.get(inode_index.value());
    ASSERT_FALSE(get_res.has_value());
    ASSERT_EQ(get_res.error(), FsError::NotFound);
}

TEST(InodeManager, UpdatesInode)
{
    StackDisk disk;
    RawBlockDevice device(128, disk);
    SuperBlock superblock {
        .total_inodes = 1, .inode_bitmap_address = 0, .inode_table_address = 1, .block_size = 128
    };
    InodeManager inode_manager(device, superblock);

    Inode inode {};
    inode.file_size = 1234;

    auto format_res = inode_manager.format();
    ASSERT_TRUE(format_res.has_value())
        << "Failed to format inode table: " << toString(format_res.error());

    auto inode_index = inode_manager.create(inode);
    ASSERT_TRUE(inode_index.has_value())
        << "Failed to create inode: " << toString(inode_index.error());

    inode.file_size = 5678;
    auto update_res = inode_manager.update(inode_index.value(), inode);
    ASSERT_TRUE(update_res.has_value())
        << "Failed to update inode: " << toString(update_res.error());

    auto read_inode_res = inode_manager.get(inode_index.value());
    ASSERT_TRUE(read_inode_res.has_value())
        << "Failed to get inode: " << toString(read_inode_res.error());

    ASSERT_EQ(read_inode_res.value().file_size, inode.file_size);
}

TEST(InodeManager, CountsFreeInodes)
{
    StackDisk disk;
    RawBlockDevice device(128, disk);
    SuperBlock superblock {
        .total_inodes = 8, .inode_bitmap_address = 0, .inode_table_address = 1, .block_size = 128
    };
    InodeManager inode_manager(device, superblock);

    auto format_res = inode_manager.format();
    ASSERT_TRUE(format_res.has_value())
        << "Failed to format inode table: " << toString(format_res.error());

    auto free_count_res = inode_manager.numFree();
    ASSERT_TRUE(free_count_res.has_value())
        << "Failed to count free inodes: " << toString(free_count_res.error());
    ASSERT_EQ(free_count_res.value(), 8);

    for (int i = 0; i < 3; i++) {
        Inode inode {};
        inode.file_size = 1234;

        auto inode_index = inode_manager.create(inode);
        ASSERT_TRUE(inode_index.has_value())
            << "Failed to create inode: " << toString(inode_index.error());
    }

    free_count_res = inode_manager.numFree();
    ASSERT_TRUE(free_count_res.has_value())
        << "Failed to count free inodes: " << toString(free_count_res.error());

    ASSERT_EQ(free_count_res.value(), 5);
}

TEST(InodeManager, WorksForMultipleInodes)
{
    StackDisk disk;
    RawBlockDevice device(128, disk);
    SuperBlock superblock { .total_inodes = 1000,
        .inode_bitmap_address = 0,
        .inode_table_address = 1000 / 8 + 1,
        .block_size = 128 };
    InodeManager inode_manager(device, superblock);

    auto format_res = inode_manager.format();
    ASSERT_TRUE(format_res.has_value())
        << "Failed to format inode table: " << toString(format_res.error());

    for (int i = 0; i < 512; i++) {
        Inode inode {};
        inode.file_size = 1234 + i;

        auto inode_index = inode_manager.create(inode);
        ASSERT_TRUE(inode_index.has_value())
            << "Failed to create inode: " << toString(inode_index.error());

        auto read_inode_res = inode_manager.get(inode_index.value());
        ASSERT_TRUE(read_inode_res.has_value())
            << "Failed to get inode: " << toString(read_inode_res.error());

        ASSERT_EQ(read_inode_res.value().file_size, inode.file_size);
    }

    auto free_count_res = inode_manager.numFree();
    ASSERT_TRUE(free_count_res.has_value())
        << "Failed to count free inodes: " << toString(free_count_res.error());
    ASSERT_EQ(free_count_res.value(), 1000 - 512);
}