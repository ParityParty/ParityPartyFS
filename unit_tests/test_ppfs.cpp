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

TEST(PpFS, ReadDirectory_Fails_NotInitialized)
{
    StackDisk disk;
    PpFS fs(disk);

    auto read_dir_res = fs.readDirectory("/");
    ASSERT_FALSE(read_dir_res.has_value());
    ASSERT_EQ(read_dir_res.error(), FsError::NotInitialized);
}

TEST(PpFS, ReadDirectory_Succeeds)
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

    auto create_file_res1 = fs.create("/file1.txt");
    ASSERT_TRUE(create_file_res1.has_value())
        << "Create file1 in directory failed: " << toString(create_file_res1.error());

    auto create_file_res2 = fs.create("/file2.txt");
    ASSERT_TRUE(create_file_res2.has_value())
        << "Create file2 in directory failed: " << toString(create_file_res2.error());

    auto read_dir_res = fs.readDirectory("/");
    ASSERT_TRUE(read_dir_res.has_value())
        << "ReadDirectory failed: " << toString(read_dir_res.error());

    std::vector<std::string> entries = read_dir_res.value();
    ASSERT_EQ(entries.size(), 2);
    ASSERT_EQ(entries[0], "file1.txt");
    ASSERT_EQ(entries[1], "file2.txt");
}

TEST(PpFS, ReadDirectory_Fails_DirectoryDoesNotExist)
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

    auto read_dir_res = fs.readDirectory("/nonexistent_dir");
    ASSERT_FALSE(read_dir_res.has_value());
    ASSERT_EQ(read_dir_res.error(), FsError::NotFound);
}

TEST(PpFS, ReadDirectory_EmptyDirectory)
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

    auto read_dir_res = fs.readDirectory("/");
    ASSERT_TRUE(read_dir_res.has_value())
        << "ReadDirectory failed: " << toString(read_dir_res.error());

    std::vector<std::string> entries = read_dir_res.value();
    ASSERT_EQ(entries.size(), 0);
}

TEST(PpFS, ReadDirectory_Fails_InvalidPath)
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

    auto read_dir_res1 = fs.readDirectory("mydir");
    ASSERT_EQ(read_dir_res1.error(), FsError::InvalidPath);

    auto read_dir_res2 = fs.readDirectory("/dir//mydir");
    ASSERT_EQ(read_dir_res2.error(), FsError::InvalidPath);
}

TEST(PpFS, ReadDirectory_AfterCreatingSubdirectory)
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

    auto create_dir_res = fs.createDirectory("/subdir");
    ASSERT_TRUE(create_dir_res.has_value())
        << "CreateDirectory failed: " << toString(create_dir_res.error());

    auto add_file_res = fs.create("/subdir/file.txt");
    ASSERT_TRUE(add_file_res.has_value())
        << "Create file in subdir failed: " << toString(add_file_res.error());

    add_file_res = fs.create("/subdir/file2.txt");
    ASSERT_TRUE(add_file_res.has_value())
        << "Create file2 in subdir failed: " << toString(add_file_res.error());

    auto read_dir_res = fs.readDirectory("/");
    ASSERT_TRUE(read_dir_res.has_value())
        << "ReadDirectory failed: " << toString(read_dir_res.error());

    std::vector<std::string> entries = read_dir_res.value();
    ASSERT_EQ(entries.size(), 1);
    ASSERT_EQ(entries[0], "subdir");
}

TEST(PpFS, Open_Fails_NotInitialized)
{
    StackDisk disk;
    PpFS fs(disk);

    auto open_res = fs.open("/file.txt");
    ASSERT_FALSE(open_res.has_value());
    ASSERT_EQ(open_res.error(), FsError::NotInitialized);
}

TEST(PpFS, Open_Fails_FileDoesNotExist)
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

    auto open_res = fs.open("/nonexistent_file.txt");
    ASSERT_FALSE(open_res.has_value());
    ASSERT_EQ(open_res.error(), FsError::NotFound);
}

TEST(PpFS, Open_Fails_InvalidPath)
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

    auto open_res1 = fs.open("file.txt");
    ASSERT_EQ(open_res1.error(), FsError::InvalidPath);

    auto open_res2 = fs.open("/dir//file.txt");
    ASSERT_EQ(open_res2.error(), FsError::InvalidPath);
}

TEST(PpFS, Open_Succeeds_AfterCreatingFile)
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
    ASSERT_TRUE(create_res.has_value()) << "Create file failed: " << toString(create_res.error());

    auto open_res = fs.open("/file.txt");
    ASSERT_TRUE(open_res.has_value()) << "Open file failed: " << toString(open_res.error());
}

TEST(PpFS, Open_MultipleFileDescriptors)
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
    ASSERT_TRUE(create_res.has_value()) << "Create file failed: " << toString(create_res.error());

    const int num_opens = 5;
    std::vector<file_descriptor_t> fds;
    for (int i = 0; i < num_opens; ++i) {
        auto open_res = fs.open("/file.txt");
        ASSERT_TRUE(open_res.has_value()) << "Open file failed: " << toString(open_res.error());
        fds.push_back(open_res.value());
    }

    // Ensure all file descriptors are unique
    std::sort(fds.begin(), fds.end());
    auto last = std::unique(fds.begin(), fds.end());
    ASSERT_EQ(std::distance(fds.begin(), last), num_opens);
}

TEST(PpFS, Open_Fails_TooManyOpenFiles)
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
    ASSERT_TRUE(create_res.has_value()) << "Create file failed: " << toString(create_res.error());

    const int max_files = MAX_OPEN_FILES;
    std::vector<file_descriptor_t> fds;
    for (int i = 0; i < max_files; ++i) {
        auto open_res = fs.open("/file.txt");
        ASSERT_TRUE(open_res.has_value()) << "Open file failed: " << toString(open_res.error());
        fds.push_back(open_res.value());
    }

    // Next open should fail
    auto open_res = fs.open("/file.txt");
    ASSERT_FALSE(open_res.has_value());
    ASSERT_EQ(open_res.error(), FsError::OpenFilesTableFull);
}

TEST(PpFS, Open_Fails_ExclusiveAlreadyOpen)
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
    ASSERT_TRUE(create_res.has_value()) << "Create file failed: " << toString(create_res.error());

    auto open_res1 = fs.open("/file.txt", OpenMode::Exclusive); // Exclusive open
    ASSERT_TRUE(open_res1.has_value()) << "First open failed: " << toString(open_res1.error());

    auto open_res2 = fs.open("/file.txt", OpenMode::Normal);
    ASSERT_FALSE(open_res2.has_value());
    ASSERT_EQ(open_res2.error(), FsError::AlreadyOpen);
}

TEST(PpFS, Close_Fails_NotInitialized)
{
    StackDisk disk;
    PpFS fs(disk);

    auto close_res = fs.close(0);
    ASSERT_FALSE(close_res.has_value());
    ASSERT_EQ(close_res.error(), FsError::NotInitialized);
}

TEST(PpFS, Close_Fails_InvalidFileDescriptor)
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

    auto close_res = fs.close(MAX_OPEN_FILES + 1); // Invalid FD
    ASSERT_FALSE(close_res.has_value());
    ASSERT_EQ(close_res.error(), FsError::OutOfBounds);
}

TEST(PpFS, Close_Fails_FileDescriptorNotOpen)
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

    auto close_res = fs.close(0); // FD 0 not opened yet
    ASSERT_FALSE(close_res.has_value());
    ASSERT_EQ(close_res.error(), FsError::NotFound);
}

TEST(PpFS, Close_Succeeds_AfterOpen)
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
    ASSERT_TRUE(create_res.has_value()) << "Create file failed: " << toString(create_res.error());

    auto open_res = fs.open("/file.txt");
    ASSERT_TRUE(open_res.has_value()) << "Open file failed: " << toString(open_res.error());
    file_descriptor_t fd = open_res.value();

    auto close_res = fs.close(fd);
    ASSERT_TRUE(close_res.has_value()) << "Close file failed: " << toString(close_res.error());
}

TEST(PpFS, Close_Fails_TwiceOnSameFileDescriptor)
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
    ASSERT_TRUE(create_res.has_value()) << "Create file failed: " << toString(create_res.error());

    auto open_res = fs.open("/file.txt");
    ASSERT_TRUE(open_res.has_value()) << "Open file failed: " << toString(open_res.error());
    file_descriptor_t fd = open_res.value();

    auto close_res1 = fs.close(fd);
    ASSERT_TRUE(close_res1.has_value()) << "First close failed: " << toString(close_res1.error());

    auto close_res2 = fs.close(fd);
    ASSERT_FALSE(close_res2.has_value());
    ASSERT_EQ(close_res2.error(), FsError::NotFound);
}

TEST(PpFS, Close_Succeeds_MultipleFileDescriptors)
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
    ASSERT_TRUE(create_res.has_value()) << "Create file failed: " << toString(create_res.error());

    const int num_opens = 5;
    std::vector<file_descriptor_t> fds;
    for (int i = 0; i < num_opens; ++i) {
        auto open_res = fs.open("/file.txt");
        ASSERT_TRUE(open_res.has_value()) << "Open file failed: " << toString(open_res.error());
        fds.push_back(open_res.value());
    }

    for (file_descriptor_t fd : fds) {
        auto close_res = fs.close(fd);
        ASSERT_TRUE(close_res.has_value()) << "Close file failed: " << toString(close_res.error());
    }
}

TEST(PpFS, Remove_Fails_NotInitialized)
{
    StackDisk disk;
    PpFS fs(disk);

    auto remove_res = fs.remove("/file.txt");
    ASSERT_FALSE(remove_res.has_value());
    ASSERT_EQ(remove_res.error(), FsError::NotInitialized);
}

TEST(PpFS, Remove_Fails_FileDoesNotExist)
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

    auto remove_res = fs.remove("/nonexistent_file.txt");
    ASSERT_FALSE(remove_res.has_value());
    ASSERT_EQ(remove_res.error(), FsError::NotFound);
}

TEST(PpFS, Remove_Succeeds_AfterCreatingFile)
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
    ASSERT_TRUE(create_res.has_value()) << "Create file failed: " << toString(create_res.error());

    auto remove_res = fs.remove("/file.txt");
    ASSERT_TRUE(remove_res.has_value()) << "Remove file failed: " << toString(remove_res.error());

    auto read_dir_res = fs.readDirectory("/");
    ASSERT_TRUE(read_dir_res.has_value())
        << "ReadDirectory failed: " << toString(read_dir_res.error());
    std::vector<std::string> entries = read_dir_res.value();
    ASSERT_EQ(entries.size(), 0);
}

TEST(PpFS, Remove_Fails_InvalidPath)
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

    auto remove_res1 = fs.remove("file.txt");
    ASSERT_EQ(remove_res1.error(), FsError::InvalidPath);

    auto remove_res2 = fs.remove("/dir//file.txt");
    ASSERT_EQ(remove_res2.error(), FsError::InvalidPath);
}

TEST(PpFS, Remove_Fails_ParentDirectoryDoesNotExist)
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

    auto remove_res = fs.remove("/nonexistent_dir/file.txt");
    ASSERT_EQ(remove_res.error(), FsError::NotFound);
}

TEST(PpFS, Remove_Fails_FileOpen)
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
    ASSERT_TRUE(create_res.has_value()) << "Create file failed: " << toString(create_res.error());

    auto open_res = fs.open("/file.txt");
    ASSERT_TRUE(open_res.has_value()) << "Open file failed: " << toString(open_res.error());

    auto remove_res = fs.remove("/file.txt");
    ASSERT_FALSE(remove_res.has_value());
    ASSERT_EQ(remove_res.error(), FsError::FileInUse);

    auto close_res = fs.close(open_res.value());
    ASSERT_TRUE(close_res.has_value()) << "Close file failed: " << toString(close_res.error());
}

TEST(PpFS, Remove_Succeeds_AfterClosingFile)
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
    ASSERT_TRUE(create_res.has_value()) << "Create file failed: " << toString(create_res.error());

    auto open_res = fs.open("/file.txt");
    ASSERT_TRUE(open_res.has_value()) << "Open file failed: " << toString(open_res.error());
    file_descriptor_t fd = open_res.value();

    auto close_res = fs.close(fd);
    ASSERT_TRUE(close_res.has_value()) << "Close file failed: " << toString(close_res.error());

    auto remove_res = fs.remove("/file.txt");
    ASSERT_TRUE(remove_res.has_value()) << "Remove file failed: " << toString(remove_res.error());

    auto read_dir_res = fs.readDirectory("/");
    ASSERT_TRUE(read_dir_res.has_value())
        << "ReadDirectory failed: " << toString(read_dir_res.error());
    std::vector<std::string> entries = read_dir_res.value();
    ASSERT_EQ(entries.size(), 0);
}

TEST(PpFS, Remove_Succeeds_Recursive)
{
    StackDisk disk;
    PpFS fs(disk);

    FsConfig config;
    config.total_size = 8192;
    config.block_size = 128;
    config.average_file_size = 256;
    auto format_res = fs.format(config);
    ASSERT_TRUE(format_res.has_value());
    ASSERT_TRUE(fs.isInitialized());

    auto create_dir_res = fs.createDirectory("/dir");
    ASSERT_TRUE(create_dir_res.has_value())
        << "CreateDirectory failed: " << toString(create_dir_res.error());

    auto create_file_res1 = fs.create("/dir/file1.txt");
    ASSERT_TRUE(create_file_res1.has_value())
        << "Create file1 failed: " << toString(create_file_res1.error());

    auto create_file_res2 = fs.create("/dir/file2.txt");
    ASSERT_TRUE(create_file_res2.has_value())
        << "Create file2 failed: " << toString(create_file_res2.error());

    auto count_res = fs.getFileCount();
    ASSERT_TRUE(count_res.has_value()) << "GetFileCount failed: " << toString(count_res.error());
    ASSERT_EQ(count_res.value(), 4);

    auto remove_res = fs.remove("/dir", true); // Recursive remove
    ASSERT_TRUE(remove_res.has_value())
        << "Recursive remove failed: " << toString(remove_res.error());

    count_res = fs.getFileCount();
    ASSERT_TRUE(count_res.has_value()) << "GetFileCount failed: " << toString(count_res.error());
    ASSERT_EQ(count_res.value(), 1);
}

TEST(PpFS, Remove_Fails_NonRecursiveOnDirectory)
{
    StackDisk disk;
    PpFS fs(disk);

    FsConfig config;
    config.total_size = 8192;
    config.block_size = 128;
    config.average_file_size = 256;
    auto format_res = fs.format(config);
    ASSERT_TRUE(format_res.has_value());
    ASSERT_TRUE(fs.isInitialized());

    auto create_dir_res = fs.createDirectory("/dir");
    ASSERT_TRUE(create_dir_res.has_value())
        << "CreateDirectory failed: " << toString(create_dir_res.error());

    auto create_file_res = fs.create("/dir/file.txt");
    ASSERT_TRUE(create_file_res.has_value())
        << "Create file failed: " << toString(create_file_res.error());

    auto remove_res = fs.remove("/dir"); // Non-recursive remove
    ASSERT_FALSE(remove_res.has_value());
    ASSERT_EQ(remove_res.error(), FsError::DirectoryNotEmpty);
}

TEST(PpFS, GetFileCount_Succeeds)
{
    StackDisk disk;
    PpFS fs(disk);

    FsConfig config;
    config.total_size = 8192;
    config.block_size = 128;
    config.average_file_size = 256;
    auto format_res = fs.format(config);
    ASSERT_TRUE(format_res.has_value());
    ASSERT_TRUE(fs.isInitialized());

    auto count_res = fs.getFileCount();
    ASSERT_TRUE(count_res.has_value()) << "GetFileCount failed: " << toString(count_res.error());
    ASSERT_EQ(count_res.value(), 1); // Only root directory exists

    auto create_file_res1 = fs.create("/file1.txt");
    ASSERT_TRUE(create_file_res1.has_value())
        << "Create file1 failed: " << toString(create_file_res1.error());

    auto create_file_res2 = fs.create("/file2.txt");
    ASSERT_TRUE(create_file_res2.has_value())
        << "Create file2 failed: " << toString(create_file_res2.error());

    count_res = fs.getFileCount();
    ASSERT_TRUE(count_res.has_value()) << "GetFileCount failed: " << toString(count_res.error());
    ASSERT_EQ(count_res.value(), 3); // root + 2 files
}

TEST(PpFS, GetFileCount_Fails_NotInitialized)
{
    StackDisk disk;
    PpFS fs(disk);

    auto count_res = fs.getFileCount();
    ASSERT_FALSE(count_res.has_value());
    ASSERT_EQ(count_res.error(), FsError::NotInitialized);
}

TEST(PpFS, Read_Fails_NotInitialized)
{
    StackDisk disk;
    PpFS fs(disk);

    auto read_res = fs.read(0, 0);
    ASSERT_FALSE(read_res.has_value());
    ASSERT_EQ(read_res.error(), FsError::NotInitialized);
}

TEST(PpFS, Read_Fails_InvalidFileDescriptor)
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

    auto read_res = fs.read(1, 0);
    ASSERT_FALSE(read_res.has_value());
    ASSERT_EQ(read_res.error(), FsError::NotFound);
}

TEST(PpFS, Read_Fails_AppendMode)
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
    ASSERT_TRUE(create_res.has_value()) << "Create file failed: " << toString(create_res.error());
    auto open_res = fs.open("/file.txt", OpenMode::Append);
    ASSERT_TRUE(open_res.has_value()) << "Open file failed: " << toString(open_res.error());
    file_descriptor_t fd = open_res.value();

    auto read_res = fs.read(fd, 10);
    ASSERT_FALSE(read_res.has_value());
    ASSERT_EQ(read_res.error(), FsError::InvalidRequest);

    auto close_res = fs.close(fd);
    ASSERT_TRUE(close_res.has_value()) << "Close file failed: " << toString(close_res.error());
}

TEST(PpFS, Write_Fails_NotInitialized)
{
    StackDisk disk;
    PpFS fs(disk);

    std::vector<uint8_t> data = { 1, 2, 3 };
    auto write_res = fs.write(0, data);
    ASSERT_FALSE(write_res.has_value());
    ASSERT_EQ(write_res.error(), FsError::NotInitialized);
}

TEST(PpFS, Write_Fails_InvalidFileDescriptor)
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

    std::vector<uint8_t> data = { 1, 2, 3 };
    auto write_res = fs.write(1, data);
    ASSERT_FALSE(write_res.has_value());
    ASSERT_EQ(write_res.error(), FsError::NotFound);
}

TEST(PpFS, Seek_Fails_NotInitialized)
{
    StackDisk disk;
    PpFS fs(disk);

    auto seek_res = fs.seek(0, 0);
    ASSERT_FALSE(seek_res.has_value());
    ASSERT_EQ(seek_res.error(), FsError::NotInitialized);
}

TEST(PpFS, Seek_Fails_InvalidFileDescriptor)
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

    auto seek_res = fs.seek(1, 0);
    ASSERT_FALSE(seek_res.has_value());
    ASSERT_EQ(seek_res.error(), FsError::NotFound);
}

TEST(PpFS, Seek_Fails_OutOfBounds)
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
    ASSERT_TRUE(create_res.has_value()) << "Create file failed: " << toString(create_res.error());
    auto open_res = fs.open("/file.txt");
    ASSERT_TRUE(open_res.has_value()) << "Open file failed: " << toString(open_res.error());
    file_descriptor_t fd = open_res.value();

    auto seek_res = fs.seek(fd, 1000); // Out of bounds
    ASSERT_FALSE(seek_res.has_value());
    ASSERT_EQ(seek_res.error(), FsError::OutOfBounds);

    auto close_res = fs.close(fd);
    ASSERT_TRUE(close_res.has_value()) << "Close file failed: " << toString(close_res.error());
}

TEST(PpFS, Seek_Succeeds)
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
    ASSERT_TRUE(create_res.has_value()) << "Create file failed: " << toString(create_res.error());
    auto open_res = fs.open("/file.txt");
    ASSERT_TRUE(open_res.has_value()) << "Open file failed: " << toString(open_res.error());
    file_descriptor_t fd = open_res.value();

    auto seek_res = fs.seek(fd, 0);
    ASSERT_TRUE(seek_res.has_value()) << "Seek failed: " << toString(seek_res.error());

    auto close_res = fs.close(fd);
    ASSERT_TRUE(close_res.has_value()) << "Close file failed: " << toString(close_res.error());
}

TEST(PpFS, Seek_Fails_AppendMode)
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
    ASSERT_TRUE(create_res.has_value()) << "Create file failed: " << toString(create_res.error());
    auto open_res = fs.open("/file.txt", OpenMode::Append);
    ASSERT_TRUE(open_res.has_value()) << "Open file failed: " << toString(open_res.error());
    file_descriptor_t fd = open_res.value();

    auto seek_res = fs.seek(fd, 0);
    ASSERT_FALSE(seek_res.has_value());
    ASSERT_EQ(seek_res.error(), FsError::InvalidRequest);

    auto close_res = fs.close(fd);
    ASSERT_TRUE(close_res.has_value()) << "Close file failed: " << toString(close_res.error());
}

TEST(PpFS, WriteSeekRead_Succeeds)
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
    ASSERT_TRUE(create_res.has_value()) << "Create file failed: " << toString(create_res.error());
    auto open_res = fs.open("/file.txt");
    ASSERT_TRUE(open_res.has_value()) << "Open file failed: " << toString(open_res.error());
    file_descriptor_t fd = open_res.value();

    const std::vector<uint8_t> write_data = { 1, 2, 3, 4, 5 };
    auto write_res = fs.write(fd, write_data);
    ASSERT_TRUE(write_res.has_value()) << "Write failed: " << toString(write_res.error());
    auto seek_res = fs.seek(fd, 0);
    ASSERT_TRUE(seek_res.has_value()) << "Seek failed: " << toString(seek_res.error());
    auto read_res = fs.read(fd, write_data.size());
    ASSERT_TRUE(read_res.has_value()) << "Read failed: " << toString(read_res.error());

    auto close_res = fs.close(fd);
    ASSERT_TRUE(close_res.has_value()) << "Close file failed: " << toString(close_res.error());

    std::vector<uint8_t> read_data = read_res.value();
    ASSERT_EQ(read_data.size(), write_data.size());
    ASSERT_EQ(read_data, write_data);
}

TEST(PpFS, WriteSeekWriteRead_Succeeds)
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
    ASSERT_TRUE(create_res.has_value()) << "Create file failed: " << toString(create_res.error());
    auto open_res = fs.open("/file.txt");
    ASSERT_TRUE(open_res.has_value()) << "Open file failed: " << toString(open_res.error());
    file_descriptor_t fd = open_res.value();

    const std::vector<uint8_t> first_write_data = { 1, 2, 3, 4, 5 };
    auto write_res1 = fs.write(fd, first_write_data);
    ASSERT_TRUE(write_res1.has_value()) << "First write failed: " << toString(write_res1.error());

    auto seek_res = fs.seek(fd, 3);
    ASSERT_TRUE(seek_res.has_value()) << "Seek failed: " << toString(seek_res.error());

    const std::vector<uint8_t> second_write_data = { 6, 7, 8, 9, 10 };
    auto write_res2 = fs.write(fd, second_write_data);
    ASSERT_TRUE(write_res2.has_value()) << "Second write failed: " << toString(write_res2.error());

    seek_res = fs.seek(fd, 0);
    ASSERT_TRUE(seek_res.has_value()) << "Seek failed: " << toString(seek_res.error());

    auto read_res = fs.read(fd, 8);
    ASSERT_TRUE(read_res.has_value()) << "Read failed: " << toString(read_res.error());

    auto close_res = fs.close(fd);
    ASSERT_TRUE(close_res.has_value()) << "Close file failed: " << toString(close_res.error());

    std::vector<uint8_t> read_data = read_res.value();
    std::vector<uint8_t> expected_data = { 1, 2, 3, 6, 7, 8, 9, 10 };
    ASSERT_EQ(read_data.size(), expected_data.size());
    ASSERT_EQ(read_data, expected_data);
}

TEST(PpFS, Write_Succeeds_MultipleFD_AppendMode)
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
    ASSERT_TRUE(create_res.has_value()) << "Create file failed: " << toString(create_res.error());

    auto open_res1 = fs.open("/file.txt", OpenMode::Append);
    ASSERT_TRUE(open_res1.has_value()) << "First open failed: " << toString(open_res1.error());
    file_descriptor_t fd1 = open_res1.value();

    auto open_res2 = fs.open("/file.txt", OpenMode::Append);
    ASSERT_TRUE(open_res2.has_value()) << "Second open failed: " << toString(open_res2.error());
    file_descriptor_t fd2 = open_res2.value();

    const std::vector<uint8_t> data1 = { 1, 2, 3 };
    auto write_res1 = fs.write(fd1, data1);
    ASSERT_TRUE(write_res1.has_value()) << "First write failed: " << toString(write_res1.error());

    const std::vector<uint8_t> data2 = { 4, 5, 6 };
    auto write_res2 = fs.write(fd2, data2);
    ASSERT_TRUE(write_res2.has_value()) << "Second write failed: " << toString(write_res2.error());

    const std::vector<uint8_t> data3 = { 7, 8, 9 };
    auto write_res3 = fs.write(fd1, data3);
    ASSERT_TRUE(write_res3.has_value()) << "Third write failed: " << toString(write_res3.error());

    auto open_res3 = fs.open("/file.txt");
    ASSERT_TRUE(open_res3.has_value()) << "Re-open file failed: " << toString(open_res3.error());
    file_descriptor_t fd3 = open_res3.value();

    auto read_res = fs.read(fd3, 9);
    ASSERT_TRUE(read_res.has_value()) << "Read failed: " << toString(read_res.error());

    auto close_res1 = fs.close(fd1);
    ASSERT_TRUE(close_res1.has_value())
        << "Close first FD failed: " << toString(close_res1.error());

    auto close_res2 = fs.close(fd2);
    ASSERT_TRUE(close_res2.has_value())
        << "Close second FD failed: " << toString(close_res2.error());

    auto close_res3 = fs.close(fd3);
    ASSERT_TRUE(close_res3.has_value())
        << "Close re-opened FD failed: " << toString(close_res3.error());

    std::vector<uint8_t> read_data = read_res.value();
    std::vector<uint8_t> expected_data = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    ASSERT_EQ(read_data.size(), expected_data.size());
    ASSERT_EQ(read_data, expected_data);
}