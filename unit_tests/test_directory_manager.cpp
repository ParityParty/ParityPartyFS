#include "block_manager/block_manager.hpp"
#include "blockdevice/raw_block_device.hpp"
#include "directory_manager/directory_manager.hpp"
#include "disk/stack_disk.hpp"
#include "inode_manager/inode_manager.hpp"
#include <gtest/gtest.h>

TEST(DirectoryManager, compiles)
{
    StackDisk disk;
    RawBlockDevice dev(64, disk);
    BlockManager bm(0, 1024, dev);
    InodeManager im(dev, *(new SuperBlock()));
    FileIO fio(dev, bm, im);
    DirectoryManager dm(dev, im, fio);
}

TEST(DirectoryManager, AddAndReadEntries)
{
    StackDisk disk;
    RawBlockDevice dev(64, disk);
    BlockManager bm(0, 1024, dev);
    InodeManager im(dev, *(new SuperBlock()));
    FileIO fio(dev, bm, im);
    DirectoryManager dm(dev, im, fio);

    Inode dir_inode;
    auto root = im.create(dir_inode);
    ASSERT_TRUE(root.has_value());
    inode_index_t dir = root.value();

    DirectoryEntry e1;
    e1.inode = 42;
    strcpy(e1.name.data(), "hello");

    ASSERT_TRUE(dm.addEntry(dir, e1).has_value());

    auto entries = dm.getEntries(dir);
    ASSERT_TRUE(entries.has_value());
    ASSERT_EQ(entries->size(), 1);
    EXPECT_EQ(entries->at(0).inode, 42);
    EXPECT_STREQ(entries->at(0).name.data(), "hello");
}

TEST(DirectoryManager, RemoveFirstOfMultipleEntries)
{
    StackDisk disk;
    RawBlockDevice dev(64, disk);
    BlockManager bm(0, 1024, dev);
    InodeManager im(dev, *(new SuperBlock()));
    FileIO fio(dev, bm, im);
    DirectoryManager dm(dev, im, fio);

    auto dir = im.create(Inode()).value();

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

    // Usuwamy pierwszy (A)
    ASSERT_TRUE(dm.removeEntry(dir, 1).has_value());

    auto entries = dm.getEntries(dir);
    ASSERT_TRUE(entries.has_value());
    ASSERT_EQ(entries->size(), 2);

    std::set<int> expected = { 2, 3 };
    std::set<int> got = { entries->at(0).inode, entries->at(1).inode };
    EXPECT_EQ(got, expected);
}

TEST(DirectoryManager, AddDuplicateNameFails)
{
    StackDisk disk;
    RawBlockDevice dev(64, disk);
    BlockManager bm(0, 1024, dev);
    InodeManager im(dev, *(new SuperBlock()));
    FileIO fio(dev, bm, im);
    DirectoryManager dm(dev, im, fio);

    auto dir = im.create(Inode()).value();

    DirectoryEntry e1;
    e1.inode = 10;
    strcpy(e1.name.data(), "dup");

    DirectoryEntry e2;
    e2.inode = 20;
    strcpy(e2.name.data(), "dup");

    ASSERT_TRUE(dm.addEntry(dir, e1).has_value());
    auto res = dm.addEntry(dir, e2);
    ASSERT_FALSE(res.has_value());
    ASSERT_EQ(res.error(), FsError::NameTaken);
}

TEST(DirectoryManager, RemoveNonexistentEntryFails)
{
    StackDisk disk;
    RawBlockDevice dev(64, disk);
    BlockManager bm(0, 1024, dev);
    InodeManager im(dev, *(new SuperBlock()));
    FileIO fio(dev, bm, im);
    DirectoryManager dm(dev, im, fio);

    auto dir = im.create(Inode()).value();

    DirectoryEntry e;
    e.inode = 123;
    strcpy(e.name.data(), "ghost");

    ASSERT_TRUE(dm.addEntry(dir, e).has_value());

    auto res = dm.removeEntry(dir, 999);
    ASSERT_FALSE(res.has_value());
    ASSERT_EQ(res.error(), FsError::NotFound);
}
