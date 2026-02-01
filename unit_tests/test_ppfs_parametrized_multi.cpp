#include "ppfs/common/static_vector.hpp"
#include "ppfs/directory_manager/directory.hpp"
#include "ppfs/super_block_manager/super_block.hpp"
#include "test_ppfs_parametrized_helpers.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <gtest/gtest.h>
#include <set>

INSTANTIATE_TEST_SUITE_P(PpFSMulti, PpFSParametrizedTest,
    ::testing::ValuesIn(generateTestConfigs()),
    [](const ::testing::TestParamInfo<TestConfig>& info) {
        return sanitizeTestName(info.param.test_name);
    });

TEST_P(PpFSParametrizedTest, CreateMultipleDirectories)
{
    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());

    // Create multiple directories
    std::vector<std::string> dirs = { "/dir1", "/dir2", "/dir3", "/dir4", "/dir5" };

    for (const auto& dir : dirs) {
        auto create_res = fs->createDirectory(dir);
        ASSERT_TRUE(create_res.has_value())
            << "Failed to create directory " << dir << " for " << GetParam().test_name;
    }

    // Verify all directories exist by reading root
    std::array<DirectoryEntry, 1000> dir_buf;
    static_vector<DirectoryEntry> entries(dir_buf.data(), dir_buf.size());
    auto read_res = fs->readDirectory("/", entries);
    ASSERT_TRUE(read_res.has_value());

    std::set<std::string> entry_set;
    for (size_t i = 0; i < entries.size(); ++i) {
        entry_set.insert(std::string(entries[i].name.data()));
    }

    for (const auto& dir : dirs) {
        std::string dir_name = dir.substr(1); // Remove leading '/'
        ASSERT_TRUE(entry_set.find(dir_name) != entry_set.end())
            << "Directory " << dir << " not found in root for " << GetParam().test_name;
    }
}

TEST_P(PpFSParametrizedTest, CreateMultipleFiles)
{
    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());

    // Create multiple files in root
    std::vector<std::string> root_files = { "/file1.txt", "/file2.txt", "/file3.txt" };
    for (const auto& file : root_files) {
        auto create_res = fs->create(file);
        ASSERT_TRUE(create_res.has_value())
            << "Failed to create file " << file << " for " << GetParam().test_name;
    }

    // Create subdirectory and files in it
    auto dir_res = fs->createDirectory("/subdir");
    ASSERT_TRUE(dir_res.has_value());

    std::vector<std::string> subdir_files = { "/subdir/file4.txt", "/subdir/file5.txt" };
    for (const auto& file : subdir_files) {
        auto create_res = fs->create(file);
        ASSERT_TRUE(create_res.has_value())
            << "Failed to create file " << file << " for " << GetParam().test_name;
    }

    // Verify root files
    std::array<DirectoryEntry, 1000> root_buf;
    static_vector<DirectoryEntry> root_entries(root_buf.data(), root_buf.size());
    auto read_root = fs->readDirectory("/", root_entries);
    ASSERT_TRUE(read_root.has_value());
    std::set<std::string> root_set;
    for (size_t i = 0; i < root_entries.size(); ++i) {
        root_set.insert(std::string(root_entries[i].name.data()));
    }

    for (const auto& file : root_files) {
        std::string file_name = file.substr(1);
        ASSERT_TRUE(root_set.find(file_name) != root_set.end())
            << "File " << file << " not found in root for " << GetParam().test_name;
    }

    // Verify subdirectory files
    std::array<DirectoryEntry, 1000> subdir_buf;
    static_vector<DirectoryEntry> subdir_entries(subdir_buf.data(), subdir_buf.size());
    auto read_subdir = fs->readDirectory("/subdir", subdir_entries);
    ASSERT_TRUE(read_subdir.has_value());
    std::set<std::string> subdir_set;
    for (size_t i = 0; i < subdir_entries.size(); ++i) {
        subdir_set.insert(std::string(subdir_entries[i].name.data()));
    }

    for (const auto& file : subdir_files) {
        std::string file_name = file.substr(file.find_last_of('/') + 1);
        ASSERT_TRUE(subdir_set.find(file_name) != subdir_set.end())
            << "File " << file << " not found in subdir for " << GetParam().test_name;
    }
}

TEST_P(PpFSParametrizedTest, ReadDirectory_MultipleEntries)
{
    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());

    // Create mix of files and directories
    ASSERT_TRUE(fs->create("/file1.txt").has_value());
    ASSERT_TRUE(fs->create("/file2.txt").has_value());
    ASSERT_TRUE(fs->createDirectory("/dir1").has_value());
    ASSERT_TRUE(fs->createDirectory("/dir2").has_value());
    ASSERT_TRUE(fs->create("/file3.txt").has_value());

    std::array<DirectoryEntry, 1000> dir_buf;
    static_vector<DirectoryEntry> entries(dir_buf.data(), dir_buf.size());
    auto read_res = fs->readDirectory("/", entries);
    ASSERT_TRUE(read_res.has_value());

    ASSERT_GE(entries.size(), 5) << "Should have at least 5 entries for " << GetParam().test_name;

    std::set<std::string> entry_set;
    for (size_t i = 0; i < entries.size(); ++i) {
        entry_set.insert(std::string(entries[i].name.data()));
    }
    ASSERT_TRUE(entry_set.find("file1.txt") != entry_set.end());
    ASSERT_TRUE(entry_set.find("file2.txt") != entry_set.end());
    ASSERT_TRUE(entry_set.find("file3.txt") != entry_set.end());
    ASSERT_TRUE(entry_set.find("dir1") != entry_set.end());
    ASSERT_TRUE(entry_set.find("dir2") != entry_set.end());
}

TEST_P(PpFSParametrizedTest, WriteRead_MultipleFiles)
{
    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());

    // Create multiple files with different data
    std::vector<std::pair<std::string, std::vector<uint8_t>>> files_data
        = { { "/file1.txt", createTestData(GetParam().block_size / 2, 0xAA) },
              { "/file2.txt", createTestData(GetParam().block_size / 2, 0xBB) },
              { "/file3.txt", createTestData(GetParam().block_size / 2, 0xCC) },
              { "/file4.txt", createAlternatingPattern(GetParam().block_size / 2) },
              { "/file5.txt", createIncrementalPattern(GetParam().block_size / 2) } };

    // Create and write to all files
    for (const auto& [path, data] : files_data) {
        auto create_res = fs->create(path);
        ASSERT_TRUE(create_res.has_value());

        auto open_res = fs->open(path);
        ASSERT_TRUE(open_res.has_value());
        file_descriptor_t fd = open_res.value();

        std::array<uint8_t, 2048> write_buf;
        static_vector<uint8_t> write_data(write_buf.data(), write_buf.size(), data.size());
        std::memcpy(write_data.data(), data.data(), data.size());
        auto write_res = fs->write(fd, write_data);
        ASSERT_TRUE(write_res.has_value())
            << "Write failed for " << path << " for " << GetParam().test_name;

        auto close_res = fs->close(fd);
        ASSERT_TRUE(close_res.has_value());
    }

    // Read back and verify all files
    for (const auto& [path, expected_data] : files_data) {
        auto open_res = fs->open(path);
        ASSERT_TRUE(open_res.has_value());
        file_descriptor_t fd = open_res.value();

        std::array<uint8_t, 2048> read_buf;
        static_vector<uint8_t> read_data(read_buf.data(), read_buf.size());
        auto read_res = fs->read(fd, expected_data.size(), read_data);
        ASSERT_TRUE(read_res.has_value())
            << "Read failed for " << path << " for " << GetParam().test_name;

        ASSERT_EQ(read_data.size(), expected_data.size())
            << "Size mismatch for " << path << " for " << GetParam().test_name;
        for (size_t i = 0; i < expected_data.size(); ++i) {
            ASSERT_EQ(read_data[i], expected_data[i]) << "Data mismatch at index " << i << " for "
                                                      << path << " for " << GetParam().test_name;
        }

        auto close_res = fs->close(fd);
        ASSERT_TRUE(close_res.has_value());
    }
}

TEST_P(PpFSParametrizedTest, NestedDirectoryStructure)
{
    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());

    // Create deep nested directory structure
    std::vector<std::string> nested_dirs
        = { "/level1", "/level1/level2", "/level1/level2/level3", "/level1/level2/level3/level4" };

    for (const auto& dir : nested_dirs) {
        auto create_res = fs->createDirectory(dir);
        ASSERT_TRUE(create_res.has_value())
            << "Failed to create nested directory " << dir << " for " << GetParam().test_name;
    }

    // Verify each level
    std::array<DirectoryEntry, 1000> level1_buf;
    static_vector<DirectoryEntry> level1_entries(level1_buf.data(), level1_buf.size());
    auto read_level1 = fs->readDirectory("/level1", level1_entries);
    ASSERT_TRUE(read_level1.has_value());
    bool found_level2 = false;
    for (size_t i = 0; i < level1_entries.size(); ++i) {
        if (std::string(level1_entries[i].name.data()) == "level2") {
            found_level2 = true;
            break;
        }
    }
    ASSERT_TRUE(found_level2);

    std::array<DirectoryEntry, 1000> level2_buf;
    static_vector<DirectoryEntry> level2_entries(level2_buf.data(), level2_buf.size());
    auto read_level2 = fs->readDirectory("/level1/level2", level2_entries);
    ASSERT_TRUE(read_level2.has_value());
    bool found_level3 = false;
    for (size_t i = 0; i < level2_entries.size(); ++i) {
        if (std::string(level2_entries[i].name.data()) == "level3") {
            found_level3 = true;
            break;
        }
    }
    ASSERT_TRUE(found_level3);

    std::array<DirectoryEntry, 1000> level3_buf;
    static_vector<DirectoryEntry> level3_entries(level3_buf.data(), level3_buf.size());
    auto read_level3 = fs->readDirectory("/level1/level2/level3", level3_entries);
    ASSERT_TRUE(read_level3.has_value());
    bool found_level4 = false;
    for (size_t i = 0; i < level3_entries.size(); ++i) {
        if (std::string(level3_entries[i].name.data()) == "level4") {
            found_level4 = true;
            break;
        }
    }
    ASSERT_TRUE(found_level4);
}

TEST_P(PpFSParametrizedTest, FilesInMultipleDirectories)
{
    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());

    // Create multiple directories
    ASSERT_TRUE(fs->createDirectory("/dir1").has_value());
    ASSERT_TRUE(fs->createDirectory("/dir2").has_value());
    ASSERT_TRUE(fs->createDirectory("/dir3").has_value());

    // Create files in each directory
    std::vector<std::pair<std::string, std::vector<uint8_t>>> files
        = { { "/dir1/file1.txt", createTestData(100, 0x11) },
              { "/dir2/file2.txt", createTestData(100, 0x22) },
              { "/dir3/file3.txt", createTestData(100, 0x33) },
              { "/dir1/file4.txt", createTestData(100, 0x44) },
              { "/dir2/file5.txt", createTestData(100, 0x55) } };

    // Create and write to files
    for (const auto& [path, data] : files) {
        auto create_res = fs->create(path);
        ASSERT_TRUE(create_res.has_value());

        auto open_res = fs->open(path);
        ASSERT_TRUE(open_res.has_value());
        file_descriptor_t fd = open_res.value();

        std::array<uint8_t, 100> write_buf;
        static_vector<uint8_t> write_data(write_buf.data(), write_buf.size(), data.size());
        std::memcpy(write_data.data(), data.data(), data.size());
        auto write_res = fs->write(fd, write_data);
        ASSERT_TRUE(write_res.has_value());

        auto close_res = fs->close(fd);
        ASSERT_TRUE(close_res.has_value());
    }

    // Read back from each directory
    for (const auto& [path, expected_data] : files) {
        auto open_res = fs->open(path);
        ASSERT_TRUE(open_res.has_value());
        file_descriptor_t fd = open_res.value();

        std::array<uint8_t, 100> read_buf;
        static_vector<uint8_t> read_data(read_buf.data(), read_buf.size());
        auto read_res = fs->read(fd, expected_data.size(), read_data);
        ASSERT_TRUE(read_res.has_value());

        for (size_t i = 0; i < expected_data.size(); ++i) {
            ASSERT_EQ(read_data[i], expected_data[i]) << "Data mismatch at index " << i << " for "
                                                      << path << " for " << GetParam().test_name;
        }

        auto close_res = fs->close(fd);
        ASSERT_TRUE(close_res.has_value());
    }
}

TEST_P(PpFSParametrizedTest, RemoveFilesFromMultipleDirectories)
{
    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());

    // Create directories and files
    ASSERT_TRUE(fs->createDirectory("/dir1").has_value());
    ASSERT_TRUE(fs->createDirectory("/dir2").has_value());

    ASSERT_TRUE(fs->create("/dir1/file1.txt").has_value());
    ASSERT_TRUE(fs->create("/dir1/file2.txt").has_value());
    ASSERT_TRUE(fs->create("/dir2/file3.txt").has_value());
    ASSERT_TRUE(fs->create("/dir2/file4.txt").has_value());

    // Remove files
    ASSERT_TRUE(fs->remove("/dir1/file1.txt").has_value());
    ASSERT_TRUE(fs->remove("/dir2/file3.txt").has_value());

    // Verify files are removed
    std::array<DirectoryEntry, 1000> dir1_buf;
    static_vector<DirectoryEntry> dir1_entries(dir1_buf.data(), dir1_buf.size());
    auto read_dir1 = fs->readDirectory("/dir1", dir1_entries);
    ASSERT_TRUE(read_dir1.has_value());
    bool found_file1 = false, found_file2 = false;
    for (size_t i = 0; i < dir1_entries.size(); ++i) {
        std::string name(dir1_entries[i].name.data());
        if (name == "file1.txt")
            found_file1 = true;
        if (name == "file2.txt")
            found_file2 = true;
    }
    ASSERT_FALSE(found_file1);
    ASSERT_TRUE(found_file2);

    std::array<DirectoryEntry, 1000> dir2_buf;
    static_vector<DirectoryEntry> dir2_entries(dir2_buf.data(), dir2_buf.size());
    auto read_dir2 = fs->readDirectory("/dir2", dir2_entries);
    ASSERT_TRUE(read_dir2.has_value());
    bool found_file3 = false, found_file4 = false;
    for (size_t i = 0; i < dir2_entries.size(); ++i) {
        std::string name(dir2_entries[i].name.data());
        if (name == "file3.txt")
            found_file3 = true;
        if (name == "file4.txt")
            found_file4 = true;
    }
    ASSERT_FALSE(found_file3);
    ASSERT_TRUE(found_file4);
}

TEST_P(PpFSParametrizedTest, RemoveDirectoriesRecursive)
{
    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());

    // Create nested directories with files
    ASSERT_TRUE(fs->createDirectory("/parent").has_value());
    ASSERT_TRUE(fs->createDirectory("/parent/child1").has_value());
    ASSERT_TRUE(fs->createDirectory("/parent/child2").has_value());
    ASSERT_TRUE(fs->createDirectory("/parent/child1/grandchild").has_value());

    ASSERT_TRUE(fs->create("/parent/file1.txt").has_value());
    ASSERT_TRUE(fs->create("/parent/child1/file2.txt").has_value());
    ASSERT_TRUE(fs->create("/parent/child2/file3.txt").has_value());
    ASSERT_TRUE(fs->create("/parent/child1/grandchild/file4.txt").has_value());

    // Get initial file count
    auto count_before = fs->getFileCount();
    ASSERT_TRUE(count_before.has_value());

    // Remove recursively
    auto remove_res = fs->remove("/parent", true);
    ASSERT_TRUE(remove_res.has_value()) << "Recursive remove failed for " << GetParam().test_name;

    // Verify directory is removed
    std::array<DirectoryEntry, 1000> root_buf;
    static_vector<DirectoryEntry> root_entries(root_buf.data(), root_buf.size());
    auto read_root = fs->readDirectory("/", root_entries);
    ASSERT_TRUE(read_root.has_value());
    bool found_parent = false;
    for (size_t i = 0; i < root_entries.size(); ++i) {
        if (std::string(root_entries[i].name.data()) == "parent") {
            found_parent = true;
            break;
        }
    }
    ASSERT_FALSE(found_parent);

    // Verify file count decreased
    auto count_after = fs->getFileCount();
    ASSERT_TRUE(count_after.has_value());
    ASSERT_LT(count_after.value(), count_before.value());
}

TEST_P(PpFSParametrizedTest, GetFileCount_MultipleFiles)
{
    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());

    // Initial count (just root directory)
    auto count_initial = fs->getFileCount();
    ASSERT_TRUE(count_initial.has_value());
    ASSERT_EQ(count_initial.value(), 1)
        << "Initial file count should be 1 (root) for " << GetParam().test_name;

    // Create directories
    ASSERT_TRUE(fs->createDirectory("/dir1").has_value());
    ASSERT_TRUE(fs->createDirectory("/dir2").has_value());

    auto count_after_dirs = fs->getFileCount();
    ASSERT_TRUE(count_after_dirs.has_value());
    ASSERT_EQ(count_after_dirs.value(), 3); // root + 2 dirs

    // Create files
    ASSERT_TRUE(fs->create("/file1.txt").has_value());
    ASSERT_TRUE(fs->create("/file2.txt").has_value());
    ASSERT_TRUE(fs->create("/dir1/file3.txt").has_value());
    ASSERT_TRUE(fs->create("/dir2/file4.txt").has_value());

    auto count_after_files = fs->getFileCount();
    ASSERT_TRUE(count_after_files.has_value());
    ASSERT_EQ(count_after_files.value(), 7) // root + 2 dirs + 4 files
        << "File count mismatch for " << GetParam().test_name;
}

TEST_P(PpFSParametrizedTest, OpenMultipleFiles)
{
    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());

    // Create multiple files
    std::vector<std::string> files
        = { "/file1.txt", "/file2.txt", "/file3.txt", "/file4.txt", "/file5.txt" };
    std::vector<std::vector<uint8_t>> file_data;

    for (size_t i = 0; i < files.size(); ++i) {
        auto create_res = fs->create(files[i]);
        ASSERT_TRUE(create_res.has_value());

        file_data.push_back(
            createTestData(GetParam().block_size / 2, static_cast<uint8_t>(0x10 + i)));
    }

    // Open all files and write to them
    std::vector<file_descriptor_t> fds;
    for (size_t i = 0; i < files.size(); ++i) {
        auto open_res = fs->open(files[i]);
        ASSERT_TRUE(open_res.has_value());
        fds.push_back(open_res.value());

        std::array<uint8_t, 2048> write_buf;
        static_vector<uint8_t> write_data(write_buf.data(), write_buf.size(), file_data[i].size());
        std::memcpy(write_data.data(), file_data[i].data(), file_data[i].size());
        auto write_res = fs->write(fds[i], write_data);
        ASSERT_TRUE(write_res.has_value())
            << "Write failed for " << files[i] << " for " << GetParam().test_name;
    }

    // Read back from all files
    for (size_t i = 0; i < files.size(); ++i) {
        auto seek_res = fs->seek(fds[i], 0);
        ASSERT_TRUE(seek_res.has_value());

        std::array<uint8_t, 2048> read_buf;
        static_vector<uint8_t> read_data(read_buf.data(), read_buf.size());
        auto read_res = fs->read(fds[i], file_data[i].size(), read_data);
        ASSERT_TRUE(read_res.has_value())
            << "Read failed for " << files[i] << " for " << GetParam().test_name;

        for (size_t j = 0; j < file_data[i].size(); ++j) {
            ASSERT_EQ(read_data[j], file_data[i][j]) << "Data mismatch at index " << j << " for "
                                                     << files[i] << " for " << GetParam().test_name;
        }
    }

    // Close all files
    for (auto fd : fds) {
        auto close_res = fs->close(fd);
        ASSERT_TRUE(close_res.has_value());
    }
}

TEST_P(PpFSParametrizedTest, ErrorCorrection_MultipleFiles)
{
    // Only test with ECC types that support correction
    if (GetParam().ecc_type != ECCType::Hamming && GetParam().ecc_type != ECCType::ReedSolomon) {
        GTEST_SKIP() << "Test only for Hamming or Reed-Solomon ECC types";
    }

    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());

    // Create multiple files with different data
    std::vector<std::string> files = { "/file1.txt", "/file2.txt", "/file3.txt" };
    std::vector<std::vector<uint8_t>> file_data;

    for (size_t i = 0; i < files.size(); ++i) {
        auto create_res = fs->create(files[i]);
        ASSERT_TRUE(create_res.has_value());

        file_data.push_back(
            createTestData(GetParam().block_size / 2, static_cast<uint8_t>(0xAA + i)));

        auto open_res = fs->open(files[i]);
        ASSERT_TRUE(open_res.has_value());
        file_descriptor_t fd = open_res.value();

        std::array<uint8_t, 2048> write_buf;
        static_vector<uint8_t> write_data(write_buf.data(), write_buf.size(), file_data[i].size());
        std::memcpy(write_data.data(), file_data[i].data(), file_data[i].size());
        auto write_res = fs->write(fd, write_data);
        ASSERT_TRUE(write_res.has_value());

        auto close_res = fs->close(fd);
        ASSERT_TRUE(close_res.has_value());
    }

    // Inject errors (different approach for Hamming vs RS)
    // Read superblock to find data region
    std::array<uint8_t, 512> sb_buf;
    static_vector<uint8_t> sb_data(sb_buf.data(), sb_buf.size());
    auto sb_read = disk.read(0, sizeof(SuperBlock), sb_data);
    ASSERT_TRUE(sb_read.has_value());
    SuperBlock sb;
    std::memcpy(&sb, sb_data.data(), sizeof(SuperBlock));

    size_t data_region = findDataBlockRegion(disk, sb);

    if (GetParam().ecc_type == ECCType::Hamming) {
        // Inject single bit errors in different locations
        injectBitFlip(disk, data_region + 50, 0x01);
        injectBitFlip(disk, data_region + 150, 0x02);
        injectBitFlip(disk, data_region + 250, 0x04);
    } else if (GetParam().ecc_type == ECCType::ReedSolomon) {
        // Inject byte errors (within correction capability)
        if (GetParam().rs_correctable_bytes >= 1) {
            injectByteError(disk, data_region + 50, 0xFF);
        }
        if (GetParam().rs_correctable_bytes >= 2) {
            injectByteError(disk, data_region + 150, 0x00);
        }
        if (GetParam().rs_correctable_bytes >= 3) {
            injectByteError(disk, data_region + 250, 0xAA);
        }
    }

    // Read back all files and verify data is correct
    for (size_t i = 0; i < files.size(); ++i) {
        auto open_res = fs->open(files[i]);
        ASSERT_TRUE(open_res.has_value());
        file_descriptor_t fd = open_res.value();

        std::array<uint8_t, 2048> read_buf;
        static_vector<uint8_t> read_data(read_buf.data(), read_buf.size());
        auto read_res = fs->read(fd, file_data[i].size(), read_data);
        ASSERT_TRUE(read_res.has_value()) << "Read failed for " << files[i]
                                          << " after error injection for " << GetParam().test_name;

        for (size_t j = 0; j < file_data[i].size(); ++j) {
            ASSERT_EQ(read_data[j], file_data[i][j])
                << "Data should be correctly recovered at index " << j << " for " << files[i]
                << " for " << GetParam().test_name;
        }

        auto close_res = fs->close(fd);
        ASSERT_TRUE(close_res.has_value());
    }
}
