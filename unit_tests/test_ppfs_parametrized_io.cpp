#include "ppfs/common/static_vector.hpp"
#include "test_ppfs_parametrized_helpers.hpp"
#include <array>
#include <gtest/gtest.h>

INSTANTIATE_TEST_SUITE_P(PpFSIO, PpFSParametrizedTest, ::testing::ValuesIn(generateTestConfigs()),
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
    auto write_data_vec = createTestData(data_size, 0xAA);
    std::array<uint8_t, 4096> write_buf;
    static_vector<uint8_t> write_data(write_buf.data(), write_buf.size(), write_data_vec.size());
    std::memcpy(write_data.data(), write_data_vec.data(), write_data_vec.size());

    auto write_res = fs->write(fd, write_data);
    ASSERT_TRUE(write_res.has_value()) << "Write failed for " << GetParam().test_name;

    // Seek to beginning
    auto seek_res = fs->seek(fd, 0);
    ASSERT_TRUE(seek_res.has_value());

    // Read back
    std::array<uint8_t, 4096> read_buf;
    static_vector<uint8_t> read_data(read_buf.data(), read_buf.size());
    auto read_res = fs->read(fd, data_size, read_data);
    ASSERT_TRUE(read_res.has_value()) << "Read failed for " << GetParam().test_name;

    ASSERT_EQ(read_data.size(), data_size)
        << "Read data size mismatch for " << GetParam().test_name;
    for (size_t i = 0; i < data_size; ++i) {
        ASSERT_EQ(read_data[i], write_data_vec[i])
            << "Read data doesn't match written data at index " << i << " for "
            << GetParam().test_name;
    }

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
    auto write_data_vec = createIncrementalPattern(data_size);
    std::array<uint8_t, 12288> write_buf; // 3 * 4096 max
    static_vector<uint8_t> write_data(write_buf.data(), write_buf.size(), write_data_vec.size());
    std::memcpy(write_data.data(), write_data_vec.data(), write_data_vec.size());

    auto write_res = fs->write(fd, write_data);
    ASSERT_TRUE(write_res.has_value()) << "Write failed for " << GetParam().test_name;

    auto seek_res = fs->seek(fd, 0);
    ASSERT_TRUE(seek_res.has_value());

    std::array<uint8_t, 12288> read_buf;
    static_vector<uint8_t> read_data(read_buf.data(), read_buf.size());
    auto read_res = fs->read(fd, data_size, read_data);
    ASSERT_TRUE(read_res.has_value()) << "Read failed for " << GetParam().test_name;

    ASSERT_EQ(read_data.size(), data_size)
        << "Read data size mismatch for " << GetParam().test_name;
    for (size_t i = 0; i < data_size; ++i) {
        ASSERT_EQ(read_data[i], write_data_vec[i])
            << "Read data doesn't match written data at index " << i << " for "
            << GetParam().test_name;
    }

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
    auto write_data_vec = createTestData(data_size, 0x42);
    std::array<uint8_t, 8192> write_buf;
    static_vector<uint8_t> write_data(write_buf.data(), write_buf.size(), write_data_vec.size());
    std::memcpy(write_data.data(), write_data_vec.data(), write_data_vec.size());

    auto write_res = fs->write(fd, write_data);
    ASSERT_TRUE(write_res.has_value()) << "Write failed for " << GetParam().test_name;

    auto seek_res = fs->seek(fd, 0);
    ASSERT_TRUE(seek_res.has_value());

    std::array<uint8_t, 8192> read_buf;
    static_vector<uint8_t> read_data(read_buf.data(), read_buf.size());
    auto read_res = fs->read(fd, data_size, read_data);
    ASSERT_TRUE(read_res.has_value()) << "Read failed for " << GetParam().test_name;

    ASSERT_EQ(read_data.size(), data_size)
        << "Read data size mismatch for " << GetParam().test_name;
    for (size_t i = 0; i < data_size; ++i) {
        ASSERT_EQ(read_data[i], write_data_vec[i])
            << "Read data doesn't match written data at index " << i << " for "
            << GetParam().test_name;
    }

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
    auto write_data_vec = createIncrementalPattern(data_size);
    std::array<uint8_t, 8192> write_buf;
    static_vector<uint8_t> write_data(write_buf.data(), write_buf.size(), write_data_vec.size());
    std::memcpy(write_data.data(), write_data_vec.data(), write_data_vec.size());

    auto write_res = fs->write(fd, write_data);
    ASSERT_TRUE(write_res.has_value());

    // Seek to middle
    size_t seek_pos = GetParam().block_size;
    auto seek_res = fs->seek(fd, seek_pos);
    ASSERT_TRUE(seek_res.has_value());

    // Read from middle
    size_t read_size = GetParam().block_size / 2;
    std::array<uint8_t, 2048> read_buf;
    static_vector<uint8_t> read_data(read_buf.data(), read_buf.size());
    auto read_res = fs->read(fd, read_size, read_data);
    ASSERT_TRUE(read_res.has_value());

    ASSERT_EQ(read_data.size(), read_size);

    // Verify data matches
    for (size_t i = 0; i < read_size; ++i) {
        ASSERT_EQ(read_data[i], write_data_vec[seek_pos + i])
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

    auto write_data1_vec = createTestData(GetParam().block_size / 2, 0xAA);
    std::array<uint8_t, 2048> write_buf1;
    static_vector<uint8_t> write_data1(
        write_buf1.data(), write_buf1.size(), write_data1_vec.size());
    std::memcpy(write_data1.data(), write_data1_vec.data(), write_data1_vec.size());
    auto write_res1 = fs->write(fd1, write_data1);
    ASSERT_TRUE(write_res1.has_value());

    auto close_res1 = fs->close(fd1);
    ASSERT_TRUE(close_res1.has_value());

    // Append more data
    auto open_res2 = fs->open("/test.txt", OpenMode::Append);
    ASSERT_TRUE(open_res2.has_value());
    file_descriptor_t fd2 = open_res2.value();

    auto write_data2_vec = createTestData(GetParam().block_size / 2, 0xBB);
    std::array<uint8_t, 2048> write_buf2;
    static_vector<uint8_t> write_data2(
        write_buf2.data(), write_buf2.size(), write_data2_vec.size());
    std::memcpy(write_data2.data(), write_data2_vec.data(), write_data2_vec.size());
    auto write_res2 = fs->write(fd2, write_data2);
    ASSERT_TRUE(write_res2.has_value());

    auto close_res2 = fs->close(fd2);
    ASSERT_TRUE(close_res2.has_value());

    // Read back all data
    auto open_res3 = fs->open("/test.txt");
    ASSERT_TRUE(open_res3.has_value());
    file_descriptor_t fd3 = open_res3.value();

    size_t total_size = write_data1.size() + write_data2.size();
    std::array<uint8_t, 4096> read_buf;
    static_vector<uint8_t> read_data(read_buf.data(), read_buf.size());
    auto read_res = fs->read(fd3, total_size, read_data);
    ASSERT_TRUE(read_res.has_value());

    ASSERT_EQ(read_data.size(), total_size);

    // Verify first part
    for (size_t i = 0; i < write_data1_vec.size(); ++i) {
        ASSERT_EQ(read_data[i], write_data1_vec[i])
            << "First part data mismatch at position " << i << " for " << GetParam().test_name;
    }

    // Verify second part
    for (size_t i = 0; i < write_data2_vec.size(); ++i) {
        ASSERT_EQ(read_data[write_data1_vec.size() + i], write_data2_vec[i])
            << "Second part data mismatch at position " << i << " for " << GetParam().test_name;
    }

    auto close_res3 = fs->close(fd3);
    ASSERT_TRUE(close_res3.has_value());
}
