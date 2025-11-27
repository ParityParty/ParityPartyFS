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

    // przygotowanie inode
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
