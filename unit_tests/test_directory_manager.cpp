#include "block_manager/block_manager.hpp"
#include "blockdevice/raw_block_device.hpp"
#include "common/static_vector.hpp"
#include "directory_manager/directory_manager.hpp"
#include "disk/stack_disk.hpp"
#include "inode_manager/inode_manager.hpp"
#include <array>
#include <gtest/gtest.h>

TEST(DirectoryManager, compiles)
{
    StackDisk disk;
    RawBlockDevice dev(64, disk);
    BlockManager bm(SuperBlock { }, dev);
    InodeManager im(dev, *(new SuperBlock()));
    FileIO fio(dev, bm, im);
    DirectoryManager dm(dev, im, fio);
}

TEST(DirectoryManager, AddAndReadEntries)
{
    StackDisk disk;
    RawBlockDevice dev(1024, disk);
    SuperBlock superblock {
        .total_inodes = 1,
        .block_bitmap_address = 2,
        .inode_bitmap_address = 0,
        .inode_table_address = 1,
        .first_data_blocks_address = 3,
        .last_data_block_address = 1024,
        .block_size = 1024,
    };
    InodeManager im(dev, superblock);

    BlockManager bm(superblock, dev);
    FileIO fio(dev, bm, im);
    DirectoryManager dm(dev, im, fio);

    ASSERT_TRUE(im.format());
    inode_index_t dir = 0; // root directory inode
    DirectoryEntry e1;
    e1.inode = 42;
    strcpy(e1.name.data(), "hello");

    auto add_res = dm.addEntry(dir, e1);
    ASSERT_TRUE(add_res.has_value()) << "Adding entry failed: " << toString(add_res.error());

    std::array<DirectoryEntry, 100> entries_buffer;
    static_vector<DirectoryEntry> entries(entries_buffer.data(), entries_buffer.size());
    auto entries_res = dm.getEntries(dir, 0, 0, entries);
    ASSERT_TRUE(entries_res.has_value());
    ASSERT_EQ(entries.size(), 1);
    EXPECT_EQ(entries[0].inode, 42);
    EXPECT_STREQ(entries[0].name.data(), "hello");
}

TEST(DirectoryManager, RemoveFirstOfMultipleEntries)
{
    StackDisk disk;
    RawBlockDevice dev(1024, disk);
    SuperBlock superblock {
        .total_inodes = 1,
        .block_bitmap_address = 2,
        .inode_bitmap_address = 0,
        .inode_table_address = 1,
        .first_data_blocks_address = 3,
        .last_data_block_address = 1024,
        .block_size = 1024,
    };
    InodeManager im(dev, superblock);
    BlockManager bm(superblock, dev);
    FileIO fio(dev, bm, im);
    DirectoryManager dm(dev, im, fio);

    ASSERT_TRUE(im.format());
    inode_index_t dir = 0; // root directory inode

    DirectoryEntry a, b, c;
    a.inode = 1;
    strcpy(a.name.data(), "A");
    b.inode = 2;
    strcpy(b.name.data(), "B");
    c.inode = 3;
    strcpy(c.name.data(), "C");

    ASSERT_TRUE(dm.addEntry(dir, a).has_value());
    ASSERT_TRUE(dm.addEntry(dir, b).has_value());
    ASSERT_TRUE(dm.addEntry(dir, c).has_value());

    auto rm_res = dm.removeEntry(dir, 1);
    ASSERT_TRUE(rm_res.has_value()) << "Removing file failed: " << toString(rm_res.error());

    std::array<DirectoryEntry, 100> entries_buffer;
    static_vector<DirectoryEntry> entries(entries_buffer.data(), entries_buffer.size());
    auto entries_res = dm.getEntries(dir, 0, 0, entries);
    ASSERT_TRUE(entries_res.has_value());
    ASSERT_EQ(entries.size(), 2);

    std::set<int> expected = { 2, 3 };
    std::set<int> got = { entries[0].inode, entries[1].inode };
    EXPECT_EQ(got, expected);
}

TEST(DirectoryManager, AddDuplicateNameFails)
{
    StackDisk disk;
    RawBlockDevice dev(1024, disk);
    SuperBlock superblock {
        .total_inodes = 1,
        .block_bitmap_address = 2,
        .inode_bitmap_address = 0,
        .inode_table_address = 1,
        .first_data_blocks_address = 3,
        .last_data_block_address = 1024,
        .block_size = 1024,
    };
    InodeManager im(dev, superblock);
    BlockManager bm(superblock, dev);
    FileIO fio(dev, bm, im);
    DirectoryManager dm(dev, im, fio);

    ASSERT_TRUE(im.format());
    inode_index_t dir = 0; // root directory inode

    DirectoryEntry e1;
    e1.inode = 10;
    strcpy(e1.name.data(), "dup");

    DirectoryEntry e2;
    e2.inode = 20;
    strcpy(e2.name.data(), "dup");

    ASSERT_TRUE(dm.addEntry(dir, e1).has_value());
    auto res = dm.addEntry(dir, e2);
    ASSERT_FALSE(res.has_value());
    ASSERT_EQ(res.error(), FsError::DirectoryManager_NameTaken);
}

TEST(DirectoryManager, RemoveNonexistentEntryFails)
{
    StackDisk disk;
    RawBlockDevice dev(1024, disk);
    SuperBlock superblock {
        .total_inodes = 1,
        .block_bitmap_address = 2,
        .inode_bitmap_address = 0,
        .inode_table_address = 1,
        .first_data_blocks_address = 3,
        .last_data_block_address = 1024,
        .block_size = 1024,
    };
    InodeManager im(dev, superblock);
    BlockManager bm(superblock, dev);
    FileIO fio(dev, bm, im);
    DirectoryManager dm(dev, im, fio);

    ASSERT_TRUE(im.format());
    inode_index_t dir = 0; // root directory inode

    DirectoryEntry e;
    e.inode = 123;
    strcpy(e.name.data(), "ghost");

    ASSERT_TRUE(dm.addEntry(dir, e).has_value());

    auto res = dm.removeEntry(dir, 999);
    ASSERT_FALSE(res.has_value());
    ASSERT_EQ(res.error(), FsError::DirectoryManager_NotFound);
}
