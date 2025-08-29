#include <gtest/gtest.h>
#include "ppfs/ppfs.hpp"
#include "disk/stack_disk.hpp"


#include <fuse3/fuse.h>

fuse_context g_fakeContext;

// Fuse context is created on syscall. We need to set it up ourselves
void setupFakeFuseContext(PpFS* fs) {
    g_fakeContext.uid = getuid();
    g_fakeContext.gid = getgid();
    g_fakeContext.pid = getpid();
    g_fakeContext.private_data = fs;
}

// override fuse_get_context so code sees fake one
extern "C" struct fuse_context* fuse_get_context(void) {
    return &g_fakeContext;
}

TEST(PpFS, Compiles)
{
    StackDisk disk;
    PpFS fs(disk);
    SUCCEED();
}

TEST(PpFS, WritesAndReads)
{
    StackDisk disk;
    PpFS fs(disk);
    setupFakeFuseContext(&fs);
    const std::string test_message = "Write test!";

    // write
    const int res = fs.write("/hello", test_message.c_str(), test_message.size(), 0, nullptr);
    ASSERT_EQ(res, test_message.size());

    // read
    const auto buf = new char[1024];
    const int res_read = fs.read("/hello", buf, test_message.size(), 0, nullptr);
    ASSERT_EQ(res_read, test_message.size());
    buf[test_message.size()] = 0;
    EXPECT_STREQ(test_message.c_str(), buf);

    delete[] buf;
}