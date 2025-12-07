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
    ASSERT_TRUE(fs.isInitialized());
}

TEST(PpFS, Format_Fails_UnsetParameters)
{
    StackDisk disk;
    PpFS fs(disk);

    FsConfig config;
    auto res = fs.format(config);
    ASSERT_FALSE(res.has_value());
    ASSERT_EQ(res.error(), FsError::InvalidRequest);
    ASSERT_FALSE(fs.isInitialized());
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
    ASSERT_FALSE(fs.isInitialized());
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
    ASSERT_FALSE(fs.isInitialized());
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
    ASSERT_FALSE(fs.isInitialized());
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
    ASSERT_FALSE(fs.isInitialized());
}

TEST(PpFS, Init_Fails_NoFormat)
{
    StackDisk disk;
    PpFS fs(disk);

    auto init_res = fs.init();
    ASSERT_FALSE(init_res.has_value());
    ASSERT_EQ(init_res.error(), FsError::DiskNotFormatted);
    ASSERT_FALSE(fs.isInitialized());
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
    ASSERT_TRUE(fs.isInitialized());

    PpFS fs2(disk);
    auto init_res = fs2.init();
    ASSERT_TRUE(init_res.has_value());
    ASSERT_TRUE(fs2.isInitialized());
}

TEST(PpFS, Create_Fails_NotInitialized)
{
    StackDisk disk;
    PpFS fs(disk);

    auto create_res = fs.create("/file.txt");
    ASSERT_FALSE(create_res.has_value());
    ASSERT_EQ(create_res.error(), FsError::NotInitialized);
}

TEST(PpFS, Create_Succeeds_AfterFormat)
{
    StackDisk disk;
    PpFS fs(disk);

    FsConfig config;
    config.total_size = 4096;
    config.block_size = 128;
    config.average_file_size = 256;
    auto format_res = fs.format(config);
    ASSERT_TRUE(format_res.has_value());
    ASSERT_TRUE(fs.isInitialized());

    auto create_res = fs.create("/file.txt");
    ASSERT_TRUE(create_res.has_value()) << "Create failed: " << toString(create_res.error());
}

TEST(PpFS, Create_Fails_DuplicateFile)
{
    StackDisk disk;
    PpFS fs(disk);

    FsConfig config;
    config.total_size = 4096;
    config.block_size = 128;
    config.average_file_size = 256;
    auto format_res = fs.format(config);
    ASSERT_TRUE(format_res.has_value());
    ASSERT_TRUE(fs.isInitialized());

    auto create_res1 = fs.create("/file.txt");
    ASSERT_TRUE(create_res1.has_value())
        << "First create failed: " << toString(create_res1.error());

    auto create_res2 = fs.create("/file.txt");
    ASSERT_EQ(create_res2.error(), FsError::NameTaken);
}

TEST(PpFS, Create_Fails_InvalidPath)
{
    StackDisk disk;
    PpFS fs(disk);

    FsConfig config;
    config.total_size = 4096;
    config.block_size = 128;
    config.average_file_size = 256;
    auto format_res = fs.format(config);
    ASSERT_TRUE(format_res.has_value());
    ASSERT_TRUE(fs.isInitialized());

    auto create_res1 = fs.create("file.txt");
    ASSERT_EQ(create_res1.error(), FsError::InvalidPath);

    auto create_res2 = fs.create("/dir//file.txt");
    ASSERT_EQ(create_res2.error(), FsError::InvalidPath);
}

TEST(PpFS, Create_Fails_ParentDirectoryDoesNotExist)
{
    StackDisk disk;
    PpFS fs(disk);

    FsConfig config;
    config.total_size = 4096;
    config.block_size = 128;
    config.average_file_size = 256;
    auto format_res = fs.format(config);
    ASSERT_TRUE(format_res.has_value());
    ASSERT_TRUE(fs.isInitialized());

    auto create_res = fs.create("/nonexistent_dir/file.txt");
    ASSERT_EQ(create_res.error(), FsError::NotFound);
}

TEST(PpFS, CreateDirectory_Fails_NotInitialized)
{
    StackDisk disk;
    PpFS fs(disk);

    auto create_res = fs.createDirectory("/mydir");
    ASSERT_FALSE(create_res.has_value());
    ASSERT_EQ(create_res.error(), FsError::NotInitialized);
}

TEST(PpFS, CreateDirectory_Succeeds_AfterFormat)
{
    StackDisk disk;
    PpFS fs(disk);

    FsConfig config;
    config.total_size = 4096;
    config.block_size = 128;
    config.average_file_size = 256;
    auto format_res = fs.format(config);
    ASSERT_TRUE(format_res.has_value());
    ASSERT_TRUE(fs.isInitialized());

    auto create_res = fs.createDirectory("/mydir");
    ASSERT_TRUE(create_res.has_value())
        << "CreateDirectory failed: " << toString(create_res.error());
}

TEST(PpFS, CreateDirectory_Fails_DuplicateDirectory)
{
    StackDisk disk;
    PpFS fs(disk);

    FsConfig config;
    config.total_size = 4096;
    config.block_size = 128;
    config.average_file_size = 256;
    auto format_res = fs.format(config);
    ASSERT_TRUE(format_res.has_value());
    ASSERT_TRUE(fs.isInitialized());

    auto create_res1 = fs.createDirectory("/mydir");
    ASSERT_TRUE(create_res1.has_value())
        << "First CreateDirectory failed: " << toString(create_res1.error());

    auto create_res2 = fs.createDirectory("/mydir");
    ASSERT_EQ(create_res2.error(), FsError::NameTaken);
}

TEST(PpFS, CreateDirectory_Fails_InvalidPath)
{
    StackDisk disk;
    PpFS fs(disk);

    FsConfig config;
    config.total_size = 4096;
    config.block_size = 128;
    config.average_file_size = 256;
    auto format_res = fs.format(config);
    ASSERT_TRUE(format_res.has_value());
    ASSERT_TRUE(fs.isInitialized());

    auto create_res1 = fs.createDirectory("mydir");
    ASSERT_EQ(create_res1.error(), FsError::InvalidPath);

    auto create_res2 = fs.createDirectory("/dir//mydir");
    ASSERT_EQ(create_res2.error(), FsError::InvalidPath);
}

TEST(PpFS, CreateDirectory_Fails_ParentDirectoryDoesNotExist)
{
    StackDisk disk;
    PpFS fs(disk);

    FsConfig config;
    config.total_size = 4096;
    config.block_size = 128;
    config.average_file_size = 256;
    auto format_res = fs.format(config);
    ASSERT_TRUE(format_res.has_value());
    ASSERT_TRUE(fs.isInitialized());

    auto create_res = fs.createDirectory("/nonexistent_dir/mydir");
    ASSERT_EQ(create_res.error(), FsError::NotFound);
}

TEST(PpFS, CreateFileInNewDirectory)
{
    StackDisk disk;
    PpFS fs(disk);

    FsConfig config;
    config.total_size = 4096;
    config.block_size = 128;
    config.average_file_size = 256;
    auto format_res = fs.format(config);
    ASSERT_TRUE(format_res.has_value());
    ASSERT_TRUE(fs.isInitialized());

    auto create_dir_res = fs.createDirectory("/mydir");
    ASSERT_TRUE(create_dir_res.has_value())
        << "CreateDirectory failed: " << toString(create_dir_res.error());
    auto create_file_res = fs.create("/mydir/file.txt");
    ASSERT_TRUE(create_file_res.has_value())
        << "Create file in directory failed: " << toString(create_file_res.error());
}