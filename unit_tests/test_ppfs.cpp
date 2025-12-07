#include "disk/stack_disk.hpp"
#include "filesystem/ppfs.hpp"
#include <gtest/gtest.h>

TEST(PpFS, Compiles)
{
    StackDisk disk;
    PpFS fs(disk);
    EXPECT_TRUE(true);
}

TEST(PpFS, Format_Succeeds)
{
    StackDisk disk;
    PpFS fs(disk);

    FsConfig config;
    config.total_size = 1024;
    config.block_size = 128;
    config.average_file_size = 256;
    auto res = fs.format(config);
    ASSERT_TRUE(res.has_value());
}

TEST(PpFS, Format_Fails_UnsetParameters)
{
    StackDisk disk;
    PpFS fs(disk);

    FsConfig config;
    auto res = fs.format(config);
    ASSERT_FALSE(res.has_value());
    ASSERT_EQ(res.error(), FsError::InvalidRequest);
}

TEST(PpFS, Format_Fails_BlockTooSmall)
{
    StackDisk disk;
    PpFS fs(disk);

    FsConfig config;
    config.total_size = 1024;
    config.block_size = 4;
    config.average_file_size = 256;
    auto res = fs.format(config);
    ASSERT_FALSE(res.has_value());
    ASSERT_EQ(res.error(), FsError::InvalidRequest);
}

TEST(PpFS, Format_Fails_TotalSizeNotMultipleOfBlockSize)
{
    StackDisk disk;
    PpFS fs(disk);

    FsConfig config;
    config.total_size = 1000;
    config.block_size = 128;
    config.average_file_size = 256;
    auto res = fs.format(config);
    ASSERT_FALSE(res.has_value());
    ASSERT_EQ(res.error(), FsError::InvalidRequest);
}

TEST(PpFS, Format_Fails_BlockSizeNotPowerOfTwo)
{
    StackDisk disk;
    PpFS fs(disk);

    FsConfig config;
    config.total_size = 1024;
    config.block_size = 120;
    config.average_file_size = 256;
    auto res = fs.format(config);
    ASSERT_FALSE(res.has_value());
    ASSERT_EQ(res.error(), FsError::InvalidRequest);
}

TEST(PpFS, Format_Fails_BadECCType)
{
    StackDisk disk;
    PpFS fs(disk);

    FsConfig config;
    config.total_size = 1024;
    config.block_size = 128;
    config.average_file_size = 256;
    config.ecc_type = static_cast<ECCType>(999); // Invalid ECC type
    auto res = fs.format(config);
    ASSERT_FALSE(res.has_value());
    ASSERT_EQ(res.error(), FsError::InvalidRequest);
}

TEST(PpFS, Init_Fails_NoFormat)
{
    StackDisk disk;
    PpFS fs(disk);

    auto init_res = fs.init();
    ASSERT_FALSE(init_res.has_value());
    ASSERT_EQ(init_res.error(), FsError::DiskNotFormatted);
}

TEST(PpFS, Init_Succeeds)
{
    StackDisk disk;
    PpFS fs(disk);

    FsConfig config;
    config.total_size = 1024;
    config.block_size = 128;
    config.average_file_size = 256;
    auto format_res = fs.format(config);
    ASSERT_TRUE(format_res.has_value());

    PpFS fs2(disk);
    auto init_res = fs2.init();
    ASSERT_TRUE(init_res.has_value());
}