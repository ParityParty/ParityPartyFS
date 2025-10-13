#include "disk/stack_disk.hpp"
#include "ppfs/ppfs.hpp"
#include <gtest/gtest.h>

#include <fuse3/fuse.h>

fuse_context g_fakeContext;

// Fuse context is created on syscall. We need to set it up ourselves
void setupFakeFuseContext(PpFS* fs)
{
    g_fakeContext.uid = getuid();
    g_fakeContext.gid = getgid();
    g_fakeContext.pid = getpid();
    g_fakeContext.private_data = fs;
}

// override fuse_get_context so code sees fake one
extern "C" struct fuse_context* fuse_get_context(void) { return &g_fakeContext; }

TEST(PpFS, Compiles)
{
    StackDisk disk;
    PpFS fs(disk);
    SUCCEED();
}

TEST(PpFS, CreatesFiles)
{
    StackDisk disk;
    PpFS fs(disk);
    setupFakeFuseContext(&fs);

    ASSERT_EQ(fs.getRootDirectory().entries.size(), 0);

    PpFS::create("/file1", 0666, nullptr);
    ASSERT_EQ(fs.getRootDirectory().entries.size(), 1);
    PpFS::create("/file2", 0666, nullptr);
    ASSERT_EQ(fs.getRootDirectory().entries.size(), 2);
}

TEST(PpFS, RemovesFiles)
{
    StackDisk disk;
    PpFS fs(disk);
    setupFakeFuseContext(&fs);

    PpFS::create("/file1", 0666, nullptr);
    ASSERT_EQ(fs.getRootDirectory().entries.size(), 1);
    PpFS::create("/file2", 0666, nullptr);
    ASSERT_EQ(fs.getRootDirectory().entries.size(), 2);

    PpFS::unlink("/file1");
    ASSERT_EQ(fs.getRootDirectory().entries.size(), 1);
    PpFS::unlink("/file2");
    ASSERT_EQ(fs.getRootDirectory().entries.size(), 0);
}

TEST(PpFS, WritesAndReads)
{
    StackDisk disk;
    PpFS fs(disk);
    setupFakeFuseContext(&fs);

    PpFS::create("/file", 0666, nullptr);
    ASSERT_EQ(fs.getRootDirectory().entries.size(), 1);

    PpFS::write("/file", "haha", 5, 0, nullptr);

    char* buf = new char[5];

    PpFS::read("/file", buf, 5, 0, nullptr);

    ASSERT_STREQ("haha", buf);

    delete[] buf;
}

TEST(PpFS, MulitBlockMultiFileIO)
{
    StackDisk disk;
    PpFS fs(disk);
    setupFakeFuseContext(&fs);

    PpFS::create("/file1", 0666, nullptr);
    ASSERT_EQ(fs.getRootDirectory().entries.size(), 1);
    PpFS::create("/file2", 0666, nullptr);
    ASSERT_EQ(fs.getRootDirectory().entries.size(), 2);

    std::string buf1
        = "Magna adipiscing minim enim dolore sit et quis exercitation ex adipiscing nisi sit "
          "lorem ut et ut aliqua et adipiscing do sed consequat minim eiusmod ea laboris eiusmod "
          "et dolor consectetur adipiscing sed consequat consectetur ex dolor nostrud aliquip "
          "nostrud aliquip tempor dolore amet ad ipsum nostrud do nostrud lorem ex adipiscing ut "
          "nostrud sed lorem dolor ipsum sit ad nostrud enim incididunt eiusmod eiusmod veniam "
          "ipsum commodo dolor ipsum tempor minim nostrud quis laboris aliquip amet quis labore "
          "adipiscing consectetur.";
    PpFS::write("/file1", buf1.c_str(), buf1.size(), 0, nullptr);
    PpFS::write("/file2", buf1.c_str(), buf1.size(), 0, nullptr);
    char* buf2 = new char[buf1.size()];

    PpFS::read("/file1", buf2, buf1.size(), 0, nullptr);
    ASSERT_STREQ(buf1.c_str(), buf2);
    PpFS::read("/file2", buf2, buf1.size(), 0, nullptr);
    ASSERT_STREQ(buf1.c_str(), buf2);

    delete[] buf2;
}

TEST(PpFS, WriteChangesAttr)
{
    StackDisk disk;
    PpFS fs(disk);
    setupFakeFuseContext(&fs);

    PpFS::create("/file1", 0666, nullptr);
    ASSERT_EQ(fs.getRootDirectory().entries.size(), 1);

    struct stat s;
    PpFS::getattr("/file1", &s, nullptr);
    EXPECT_EQ(s.st_size, 0);

    std::string buf1
        = "Magna adipiscing minim enim dolore sit et quis exercitation ex adipiscing nisi sit "
          "lorem ut et ut aliqua et adipiscing do sed consequat minim eiusmod ea laboris eiusmod "
          "et dolor consectetur adipiscing sed consequat consectetur ex dolor nostrud aliquip "
          "nostrud aliquip tempor dolore amet ad ipsum nostrud do nostrud lorem ex adipiscing ut "
          "nostrud sed lorem dolor ipsum sit ad nostrud enim incididunt eiusmod eiusmod veniam "
          "ipsum commodo dolor ipsum tempor minim nostrud quis laboris aliquip amet quis labore "
          "adipiscing consectetur.";
    PpFS::write("/file1", buf1.c_str(), buf1.size(), 0, nullptr);
    PpFS::getattr("/file1", &s, nullptr);
    EXPECT_EQ(s.st_size, buf1.size());
}

TEST(PpFS, Truncates)
{
    StackDisk disk;
    PpFS fs(disk);
    setupFakeFuseContext(&fs);

    PpFS::create("/file1", 0666, nullptr);
    ASSERT_EQ(fs.getRootDirectory().entries.size(), 1);

    struct stat s;
    std::string buf1
        = "Magna adipiscing minim enim dolore sit et quis exercitation ex adipiscing nisi sit "
          "lorem ut et ut aliqua et adipiscing do sed consequat minim eiusmod ea laboris eiusmod "
          "et dolor consectetur adipiscing sed consequat consectetur ex dolor nostrud aliquip "
          "nostrud aliquip tempor dolore amet ad ipsum nostrud do nostrud lorem ex adipiscing ut "
          "nostrud sed lorem dolor ipsum sit ad nostrud enim incididunt eiusmod eiusmod veniam "
          "ipsum commodo dolor ipsum tempor minim nostrud quis laboris aliquip amet quis labore "
          "adipiscing consectetur.";
    PpFS::write("/file1", buf1.c_str(), buf1.size(), 0, nullptr);
    PpFS::getattr("/file1", &s, nullptr);
    EXPECT_EQ(s.st_size, buf1.size());

    PpFS::truncate("/file1", 5, nullptr);
    PpFS::getattr("/file1", &s, nullptr);
    EXPECT_EQ(s.st_size, 5);
}