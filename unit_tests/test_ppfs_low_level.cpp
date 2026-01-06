#include "common/static_vector.hpp"
#include "disk/stack_disk.hpp"
#include "filesystem/ppfs_low_level.hpp"
#include <array>
#include <gtest/gtest.h>

static std::unique_ptr<PpFSLowLevel> prepareFS(IDisk& disk)
{
    auto ppfs = std::make_unique<PpFSLowLevel>(disk);

    FsConfig config { .total_size = disk.size(),
        .average_file_size = 256,
        .block_size = 128,
        .ecc_type = ECCType::None,
        .use_journal = false };

    auto res = ppfs->format(config);
    EXPECT_TRUE(res.has_value()) << "Format failed";

    return ppfs;
}

TEST(PpFSLowLevel, Compiles)
{
    StackDisk disk;
    auto ppfs = prepareFS(disk);
    EXPECT_TRUE(true);
}

TEST(PpFS, GetAttributesRoot)
{
    StackDisk disk;
    auto ppfs = prepareFS(disk);

    auto attrs = ppfs->getAttributes(0);
    ASSERT_TRUE(attrs.has_value());
    EXPECT_EQ(attrs->type, InodeType::Directory);
    EXPECT_EQ(attrs->size, 0);
}

TEST(PpFS, GetAttributesAfterCreateFile)
{
    StackDisk disk;
    auto ppfs = prepareFS(disk);

    auto created = ppfs->createWithParentInode("file", 0);
    ASSERT_TRUE(created.has_value());

    auto attrs = ppfs->getAttributes(created.value());
    ASSERT_TRUE(attrs.has_value());
    EXPECT_EQ(attrs->type, InodeType::File);
    EXPECT_EQ(attrs->size, 0);
}

TEST(PpFS, LookupExisting)
{
    StackDisk disk;
    auto ppfs = prepareFS(disk);

    auto c = ppfs->createWithParentInode("hello", 0);
    ASSERT_TRUE(c.has_value());

    auto res = ppfs->lookup(0, "hello");
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res.value(), c.value());
}

TEST(PpFS, LookupMissing)
{
    StackDisk disk;
    auto ppfs = prepareFS(disk);

    auto res = ppfs->lookup(0, "not_there");
    ASSERT_FALSE(res.has_value());
}

TEST(PpFS, DirEntriesRoot)
{
    StackDisk disk;
    auto ppfs = prepareFS(disk);

    std::array<DirectoryEntry, 1000> entries_buffer;
    static_vector<DirectoryEntry> entries(entries_buffer.data(), entries_buffer.size());
    auto entries_res = ppfs->getDirectoryEntries(0, entries, 0, 0);
    ASSERT_TRUE(entries_res.has_value());
    EXPECT_EQ(entries.size(), 0);
}

TEST(PpFS, DirEntriesAfterFileCreate)
{
    StackDisk disk;
    auto ppfs = prepareFS(disk);

    auto c = ppfs->createWithParentInode("x", 0);
    ASSERT_TRUE(c.has_value());

    std::array<DirectoryEntry, 1000> entries_buffer;
    static_vector<DirectoryEntry> entries(entries_buffer.data(), entries_buffer.size());
    auto entries_res = ppfs->getDirectoryEntries(0, entries, 0, 0);
    ASSERT_TRUE(entries_res.has_value());
    ASSERT_EQ(entries.size(), 1);
    EXPECT_EQ(std::string_view(entries[0].name.data()), "x")
        << "Name: " << std::string(entries[0].name.data()) << std::endl;
}

TEST(PpFS, CreateDirectorySimple)
{
    StackDisk disk;
    auto ppfs = prepareFS(disk);

    auto res = ppfs->createDirectoryByParent(0, "abc");
    ASSERT_TRUE(res.has_value());

    auto attrs = ppfs->getAttributes(res.value());
    ASSERT_TRUE(attrs.has_value());
    EXPECT_EQ(attrs->type, InodeType::Directory);
}

TEST(PpFS, OpenByInodeSimple)
{
    StackDisk disk;
    auto ppfs = prepareFS(disk);

    auto c = ppfs->createWithParentInode("f", 0);
    ASSERT_TRUE(c.has_value());

    auto fd = ppfs->openByInode(c.value(), OpenMode::Normal);
    ASSERT_TRUE(fd.has_value());
}

TEST(PpFS, OpenByInodeInvalid)
{
    StackDisk disk;
    auto ppfs = prepareFS(disk);

    auto fd = ppfs->openByInode(9999, OpenMode::Normal);
    ASSERT_FALSE(fd.has_value());
}

TEST(PpFS, CreateFileSimple)
{
    StackDisk disk;
    auto ppfs = prepareFS(disk);

    auto c = ppfs->createWithParentInode("file", 0);
    ASSERT_TRUE(c.has_value());
}

TEST(PpFS, CreateFileInvalidParent)
{
    StackDisk disk;
    auto ppfs = prepareFS(disk);

    auto c = ppfs->createWithParentInode("file", 9999);
    ASSERT_FALSE(c.has_value());
}

TEST(PpFS, RemoveSimpleFile)
{
    StackDisk disk;
    auto ppfs = prepareFS(disk);

    auto c = ppfs->createWithParentInode("delme", 0);
    ASSERT_TRUE(c.has_value());

    auto r = ppfs->removeByNameAndParent(0, "delme");
    ASSERT_TRUE(r.has_value());

    auto l = ppfs->lookup(0, "delme");
    ASSERT_FALSE(l.has_value());
}

TEST(PpFS, RemoveMissing)
{
    StackDisk disk;
    auto ppfs = prepareFS(disk);

    auto r = ppfs->removeByNameAndParent(0, "idk");
    ASSERT_FALSE(r.has_value());
}

TEST(PpFSLowLevel, TruncateFileSuccessfully)
{
    StackDisk disk;
    auto ppfs = prepareFS(disk);
    auto inode_res = ppfs->createWithParentInode("test_file", 0);
    ASSERT_TRUE(inode_res.has_value());
    
    inode_index_t file_inode = inode_res.has_value();

    auto res = ppfs->truncate(file_inode, 50);
    ASSERT_TRUE(res.has_value());

    res = ppfs->truncate(file_inode, 150);
    ASSERT_TRUE(res.has_value());
}

TEST(PpFSLowLevel, TruncateDirectoryFails)
{
    StackDisk disk;
    auto ppfs = prepareFS(disk);
    auto inode_res = ppfs->createDirectoryByParent(0, "abc");
    ASSERT_TRUE(inode_res.has_value());
    
    inode_index_t dir_inode = inode_res.has_value();

    auto res = ppfs->truncate(dir_inode, 50);
    ASSERT_FALSE(res.has_value());
}

TEST(PpFS, FullFlowCreateWriteRead)
{
    StackDisk disk;
    auto ppfs = prepareFS(disk);

    auto c = ppfs->createWithParentInode("data", 0);
    ASSERT_TRUE(c.has_value());

    auto fd = ppfs->openByInode(c.value(), OpenMode::Normal);
    ASSERT_TRUE(fd.has_value());

    std::array<uint8_t, 4> write_buf = { 1, 2, 3, 4 };
    static_vector<uint8_t> write_data(write_buf.data(), write_buf.size(), write_buf.size());
    auto w = ppfs->write(fd.value(), write_data);
    ASSERT_TRUE(w.has_value());
    EXPECT_EQ(w.value(), 4);

    ppfs->seek(fd.value(), 0);

    std::array<uint8_t, 4> read_buf;
    static_vector<uint8_t> read_data(read_buf.data(), read_buf.size());
    auto r = ppfs->read(fd.value(), 4, read_data);
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(read_data.size(), 4);
    EXPECT_EQ(read_data[2], 3);
}

TEST(PpFS, DirectoryTreeCreation)
{
    StackDisk disk;
    auto ppfs = prepareFS(disk);

    auto d1 = ppfs->createDirectoryByParent(0, "a");
    ASSERT_TRUE(d1.has_value());

    auto d2 = ppfs->createDirectoryByParent(d1.value(), "b");
    ASSERT_TRUE(d2.has_value());

    auto file = ppfs->createWithParentInode("x", d2.value());
    ASSERT_TRUE(file.has_value());

    std::array<DirectoryEntry, 100> dir_buf;
    static_vector<DirectoryEntry> dir_entries(dir_buf.data(), dir_buf.size());
    auto dir_res = ppfs->readDirectory("/a/b", dir_entries);
    ASSERT_TRUE(dir_res.has_value());

    ASSERT_EQ(dir_entries.size(), 1);
    EXPECT_EQ(std::string_view(dir_entries[0].name.data()), "x");
}

TEST(PpFS, RecursiveRemove)
{
    StackDisk disk;
    auto ppfs = prepareFS(disk);

    auto d1 = ppfs->createDirectoryByParent(0, "folder");
    ASSERT_TRUE(d1.has_value());

    auto f = ppfs->createWithParentInode("file", d1.value());
    ASSERT_TRUE(f.has_value());

    auto rm = ppfs->removeByNameAndParent(0, "folder", true);
    ASSERT_TRUE(rm.has_value());

    auto check = ppfs->lookup(0, "folder");
    ASSERT_FALSE(check.has_value());
}
