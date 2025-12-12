#include "test_ppfs_parametrized_helpers.hpp"
#include <gtest/gtest.h>

INSTANTIATE_TEST_SUITE_P(
    PpFSIO,
    PpFSParametrizedTest,
    ::testing::ValuesIn(generateTestConfigs()),
    [](const ::testing::TestParamInfo<TestConfig>& info) {
        return sanitizeTestName(info.param.test_name);
    });

TEST_P(PpFSParametrizedTest, WriteRead_SmallData)
{
    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());
    
    auto create_res = fs->create("/test.txt");
    ASSERT_TRUE(create_res.has_value());
    
    auto open_res = fs->open("/test.txt");
    ASSERT_TRUE(open_res.has_value());
    file_descriptor_t fd = open_res.value();
    
    // Write small data (less than block size)
    size_t data_size = GetParam().block_size / 2;
    auto write_data = createTestData(data_size, 0xAA);
    
    auto write_res = fs->write(fd, write_data);
    ASSERT_TRUE(write_res.has_value())
        << "Write failed for " << GetParam().test_name;
    
    // Seek to beginning
    auto seek_res = fs->seek(fd, 0);
    ASSERT_TRUE(seek_res.has_value());
    
    // Read back
    auto read_res = fs->read(fd, data_size);
    ASSERT_TRUE(read_res.has_value())
        << "Read failed for " << GetParam().test_name;
    
    auto read_data = read_res.value();
    ASSERT_EQ(read_data.size(), data_size)
        << "Read data size mismatch for " << GetParam().test_name;
    ASSERT_EQ(read_data, write_data)
        << "Read data doesn't match written data for " << GetParam().test_name;
    
    auto close_res = fs->close(fd);
    ASSERT_TRUE(close_res.has_value());
}


TEST_P(PpFSParametrizedTest, WriteRead_MultipleBlocks)
{
    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());
    
    auto create_res = fs->create("/test.txt");
    ASSERT_TRUE(create_res.has_value());
    
    auto open_res = fs->open("/test.txt");
    ASSERT_TRUE(open_res.has_value());
    file_descriptor_t fd = open_res.value();
    
    // Write data spanning multiple blocks
    size_t data_size = GetParam().block_size * 3;
    auto write_data = createIncrementalPattern(data_size);
    
    auto write_res = fs->write(fd, write_data);
    ASSERT_TRUE(write_res.has_value())
        << "Write failed for " << GetParam().test_name;
    
    auto seek_res = fs->seek(fd, 0);
    ASSERT_TRUE(seek_res.has_value());
    
    auto read_res = fs->read(fd, data_size);
    ASSERT_TRUE(read_res.has_value())
        << "Read failed for " << GetParam().test_name;
    
    auto read_data = read_res.value();
    ASSERT_EQ(read_data.size(), data_size)
        << "Read data size mismatch for " << GetParam().test_name;
    ASSERT_EQ(read_data, write_data)
        << "Read data doesn't match written data for " << GetParam().test_name;
    
    auto close_res = fs->close(fd);
    ASSERT_TRUE(close_res.has_value());
}

TEST_P(PpFSParametrizedTest, WriteRead_LargeData)
{
    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());
    
    auto create_res = fs->create("/test.txt");
    ASSERT_TRUE(create_res.has_value());
    
    auto open_res = fs->open("/test.txt");
    ASSERT_TRUE(open_res.has_value());
    file_descriptor_t fd = open_res.value();
    
    // Write large data (several KB)
    size_t data_size = 8192; // 8KB
    auto write_data = createTestData(data_size, 0x42);
    
    auto write_res = fs->write(fd, write_data);
    ASSERT_TRUE(write_res.has_value())
        << "Write failed for " << GetParam().test_name;
    
    auto seek_res = fs->seek(fd, 0);
    ASSERT_TRUE(seek_res.has_value());
    
    auto read_res = fs->read(fd, data_size);
    ASSERT_TRUE(read_res.has_value())
        << "Read failed for " << GetParam().test_name;
    
    auto read_data = read_res.value();
    ASSERT_EQ(read_data.size(), data_size)
        << "Read data size mismatch for " << GetParam().test_name;
    ASSERT_EQ(read_data, write_data)
        << "Read data doesn't match written data for " << GetParam().test_name;
    
    auto close_res = fs->close(fd);
    ASSERT_TRUE(close_res.has_value());
}

TEST_P(PpFSParametrizedTest, WriteSeekRead)
{
    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());
    
    auto create_res = fs->create("/test.txt");
    ASSERT_TRUE(create_res.has_value());
    
    auto open_res = fs->open("/test.txt");
    ASSERT_TRUE(open_res.has_value());
    file_descriptor_t fd = open_res.value();
    
    // Write initial data
    size_t data_size = GetParam().block_size * 2;
    auto write_data = createIncrementalPattern(data_size);
    
    auto write_res = fs->write(fd, write_data);
    ASSERT_TRUE(write_res.has_value());
    
    // Seek to middle
    size_t seek_pos = GetParam().block_size;
    auto seek_res = fs->seek(fd, seek_pos);
    ASSERT_TRUE(seek_res.has_value());
    
    // Read from middle
    size_t read_size = GetParam().block_size / 2;
    auto read_res = fs->read(fd, read_size);
    ASSERT_TRUE(read_res.has_value());
    
    auto read_data = read_res.value();
    ASSERT_EQ(read_data.size(), read_size);
    
    // Verify data matches
    for (size_t i = 0; i < read_size; ++i) {
        ASSERT_EQ(read_data[i], write_data[seek_pos + i])
            << "Data mismatch at position " << (seek_pos + i) << " for " << GetParam().test_name;
    }
    
    auto close_res = fs->close(fd);
    ASSERT_TRUE(close_res.has_value());
}

TEST_P(PpFSParametrizedTest, WriteAppendRead)
{
    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());
    
    auto create_res = fs->create("/test.txt");
    ASSERT_TRUE(create_res.has_value());
    
    // Write initial data in normal mode
    auto open_res1 = fs->open("/test.txt");
    ASSERT_TRUE(open_res1.has_value());
    file_descriptor_t fd1 = open_res1.value();
    
    auto write_data1 = createTestData(GetParam().block_size / 2, 0xAA);
    auto write_res1 = fs->write(fd1, write_data1);
    ASSERT_TRUE(write_res1.has_value());
    
    auto close_res1 = fs->close(fd1);
    ASSERT_TRUE(close_res1.has_value());
    
    // Append more data
    auto open_res2 = fs->open("/test.txt", OpenMode::Append);
    ASSERT_TRUE(open_res2.has_value());
    file_descriptor_t fd2 = open_res2.value();
    
    auto write_data2 = createTestData(GetParam().block_size / 2, 0xBB);
    auto write_res2 = fs->write(fd2, write_data2);
    ASSERT_TRUE(write_res2.has_value());
    
    auto close_res2 = fs->close(fd2);
    ASSERT_TRUE(close_res2.has_value());
    
    // Read back all data
    auto open_res3 = fs->open("/test.txt");
    ASSERT_TRUE(open_res3.has_value());
    file_descriptor_t fd3 = open_res3.value();
    
    size_t total_size = write_data1.size() + write_data2.size();
    auto read_res = fs->read(fd3, total_size);
    ASSERT_TRUE(read_res.has_value());
    
    auto read_data = read_res.value();
    ASSERT_EQ(read_data.size(), total_size);
    
    // Verify first part
    for (size_t i = 0; i < write_data1.size(); ++i) {
        ASSERT_EQ(read_data[i], write_data1[i])
            << "First part data mismatch at position " << i << " for " << GetParam().test_name;
    }
    
    // Verify second part
    for (size_t i = 0; i < write_data2.size(); ++i) {
        ASSERT_EQ(read_data[write_data1.size() + i], write_data2[i])
            << "Second part data mismatch at position " << i << " for " << GetParam().test_name;
    }
    
    auto close_res3 = fs->close(fd3);
    ASSERT_TRUE(close_res3.has_value());
}

