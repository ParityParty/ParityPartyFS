#include "ppfs/super_block_manager/super_block.hpp"
#include "test_ppfs_parametrized_helpers.hpp"
#include <cstring>
#include <gtest/gtest.h>

// Dedicated test class for Reed-Solomon ECC tests
class PpFSParametrizedReedSolomonTest : public PpFSParametrizedTest { };

INSTANTIATE_TEST_SUITE_P(PpFSReedSolomon, PpFSParametrizedReedSolomonTest,
    ::testing::ValuesIn(generateRSConfigs()), [](const ::testing::TestParamInfo<TestConfig>& info) {
        return sanitizeTestName(info.param.test_name);
    });

TEST_P(PpFSParametrizedReedSolomonTest, ErrorCorrection_ReedSolomon_SingleByte)
{
    ASSERT_EQ(GetParam().ecc_type, ECCType::ReedSolomon) << "Test only for Reed-Solomon ECC";
    if (GetParam().rs_correctable_bytes < 1)
        GTEST_SKIP() << "Test requires rs_correctable_bytes >= 1";

    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());

    auto create_res = fs->create("/test.txt");
    ASSERT_TRUE(create_res.has_value());

    auto open_res = fs->open("/test.txt");
    ASSERT_TRUE(open_res.has_value());
    file_descriptor_t fd = open_res.value();

    // Write data
    size_t data_size = GetParam().block_size / 2; // RS has overhead
    auto write_data_vec = createTestData(data_size, 0x7E);
    std::array<uint8_t, 2048> write_buf;
    static_vector<uint8_t> write_data(write_buf.data(), write_buf.size(), write_data_vec.size());
    std::memcpy(write_data.data(), write_data_vec.data(), write_data_vec.size());

    auto write_res = fs->write(fd, write_data);
    ASSERT_TRUE(write_res.has_value());

    auto close_res = fs->close(fd);
    ASSERT_TRUE(close_res.has_value());

    // Corrupt one byte on disk - RS should correct
    std::array<uint8_t, 512> sb_buf;
    static_vector<uint8_t> sb_data(sb_buf.data(), sb_buf.size());
    auto sb_read = disk.read(0, sizeof(SuperBlock), sb_data);
    ASSERT_TRUE(sb_read.has_value());
    SuperBlock sb;
    std::memcpy(&sb, sb_data.data(), sizeof(SuperBlock));

    size_t data_region = findDataBlockRegion(disk, sb);
    injectByteError(disk, data_region + 100, 0x00);

    // Read back - should correct error
    auto open_res2 = fs->open("/test.txt");
    ASSERT_TRUE(open_res2.has_value());
    file_descriptor_t fd2 = open_res2.value();

    std::array<uint8_t, 2048> read_buf;
    static_vector<uint8_t> read_data(read_buf.data(), read_buf.size());
    auto read_res = fs->read(fd2, data_size, read_data);
    ASSERT_TRUE(read_res.has_value())
        << "Reed-Solomon should correct single byte error for " << GetParam().test_name;

    ASSERT_EQ(read_data.size(), data_size);
    for (size_t i = 0; i < data_size; ++i) {
        ASSERT_EQ(read_data[i], write_data_vec[i]) << "Data should be correctly recovered at index "
                                                   << i << " for " << GetParam().test_name;
    }

    auto close_res2 = fs->close(fd2);
    ASSERT_TRUE(close_res2.has_value());
}

TEST_P(PpFSParametrizedReedSolomonTest, ErrorCorrection_ReedSolomon_DoubleByte)
{
    ASSERT_EQ(GetParam().ecc_type, ECCType::ReedSolomon) << "Test only for Reed-Solomon ECC";
    if (GetParam().rs_correctable_bytes < 2)
        GTEST_SKIP() << "Test requires rs_correctable_bytes >= 2";

    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());

    auto create_res = fs->create("/test.txt");
    ASSERT_TRUE(create_res.has_value());

    auto open_res = fs->open("/test.txt");
    ASSERT_TRUE(open_res.has_value());
    file_descriptor_t fd = open_res.value();

    // Write data
    size_t data_size = GetParam().block_size / 2;
    auto write_data_vec = createTestData(data_size, 0xAB);
    std::array<uint8_t, 2048> write_buf;
    static_vector<uint8_t> write_data(write_buf.data(), write_buf.size(), write_data_vec.size());
    std::memcpy(write_data.data(), write_data_vec.data(), write_data_vec.size());

    auto write_res = fs->write(fd, write_data);
    ASSERT_TRUE(write_res.has_value());

    auto close_res = fs->close(fd);
    ASSERT_TRUE(close_res.has_value());

    // Corrupt two bytes on disk - RS should correct
    std::array<uint8_t, 512> sb_buf;
    static_vector<uint8_t> sb_data(sb_buf.data(), sb_buf.size());
    auto sb_read = disk.read(0, sizeof(SuperBlock), sb_data);
    ASSERT_TRUE(sb_read.has_value());
    SuperBlock sb;
    std::memcpy(&sb, sb_data.data(), sizeof(SuperBlock));

    size_t data_region = findDataBlockRegion(disk, sb);
    injectByteError(disk, data_region + 50, 0xEE);
    injectByteError(disk, data_region + 150, 0x44);

    // Read back - should correct error
    auto open_res2 = fs->open("/test.txt");
    ASSERT_TRUE(open_res2.has_value());
    file_descriptor_t fd2 = open_res2.value();

    std::array<uint8_t, 2048> read_buf;
    static_vector<uint8_t> read_data(read_buf.data(), read_buf.size());
    auto read_res = fs->read(fd2, data_size, read_data);
    ASSERT_TRUE(read_res.has_value())
        << "Reed-Solomon should correct double byte error for " << GetParam().test_name;

    ASSERT_EQ(read_data.size(), data_size);
    for (size_t i = 0; i < data_size; ++i) {
        ASSERT_EQ(read_data[i], write_data_vec[i]) << "Data should be correctly recovered at index "
                                                   << i << " for " << GetParam().test_name;
    }

    auto close_res2 = fs->close(fd2);
    ASSERT_TRUE(close_res2.has_value());
}

TEST_P(PpFSParametrizedReedSolomonTest, ErrorCorrection_ReedSolomon_TripleByte)
{
    ASSERT_EQ(GetParam().ecc_type, ECCType::ReedSolomon) << "Test only for Reed-Solomon ECC";
    if (GetParam().rs_correctable_bytes < 3) {
        GTEST_SKIP() << "Test only for higher correctable bytes";
    }

    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());

    auto create_res = fs->create("/test.txt");
    ASSERT_TRUE(create_res.has_value());

    auto open_res = fs->open("/test.txt");
    ASSERT_TRUE(open_res.has_value());
    file_descriptor_t fd = open_res.value();

    // Write data
    size_t data_size = GetParam().block_size / 2;
    auto write_data_vec = createTestData(data_size, 0xAB);
    std::array<uint8_t, 2048> write_buf;
    static_vector<uint8_t> write_data(write_buf.data(), write_buf.size(), write_data_vec.size());
    std::memcpy(write_data.data(), write_data_vec.data(), write_data_vec.size());

    auto write_res = fs->write(fd, write_data);
    ASSERT_TRUE(write_res.has_value());

    auto close_res = fs->close(fd);
    ASSERT_TRUE(close_res.has_value());

    // Corrupt three bytes on disk - RS should correct
    std::array<uint8_t, 512> sb_buf;
    static_vector<uint8_t> sb_data(sb_buf.data(), sb_buf.size());
    auto sb_read = disk.read(0, sizeof(SuperBlock), sb_data);
    ASSERT_TRUE(sb_read.has_value());
    SuperBlock sb;
    std::memcpy(&sb, sb_data.data(), sizeof(SuperBlock));

    size_t data_region = findDataBlockRegion(disk, sb);
    injectByteError(disk, data_region + 10, 0xEE);
    injectByteError(disk, data_region + 100, 0x61);
    injectByteError(disk, data_region + 200, 0x44);

    // Read back - should correct error
    auto open_res2 = fs->open("/test.txt");
    ASSERT_TRUE(open_res2.has_value());
    file_descriptor_t fd2 = open_res2.value();

    std::array<uint8_t, 2048> read_buf;
    static_vector<uint8_t> read_data(read_buf.data(), read_buf.size());
    auto read_res = fs->read(fd2, data_size, read_data);
    ASSERT_TRUE(read_res.has_value())
        << "Reed-Solomon should correct triple byte error for " << GetParam().test_name;

    ASSERT_EQ(read_data.size(), data_size);
    for (size_t i = 0; i < data_size; ++i) {
        ASSERT_EQ(read_data[i], write_data_vec[i]) << "Data should be correctly recovered at index "
                                                   << i << " for " << GetParam().test_name;
    }

    auto close_res2 = fs->close(fd2);
    ASSERT_TRUE(close_res2.has_value());
}

TEST_P(PpFSParametrizedReedSolomonTest, ErrorCorrection_ReedSolomon_FourByte)
{
    ASSERT_EQ(GetParam().ecc_type, ECCType::ReedSolomon) << "Test only for Reed-Solomon ECC";
    if (GetParam().rs_correctable_bytes < 4)
        GTEST_SKIP() << "Test requires rs_correctable_bytes >= 4";

    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());

    auto create_res = fs->create("/test.txt");
    ASSERT_TRUE(create_res.has_value());

    auto open_res = fs->open("/test.txt");
    ASSERT_TRUE(open_res.has_value());
    file_descriptor_t fd = open_res.value();

    // Write data
    size_t data_size = GetParam().block_size / 2;
    auto write_data_vec = createTestData(data_size, 0xCD);
    std::array<uint8_t, 2048> write_buf;
    static_vector<uint8_t> write_data(write_buf.data(), write_buf.size(), write_data_vec.size());
    std::memcpy(write_data.data(), write_data_vec.data(), write_data_vec.size());

    auto write_res = fs->write(fd, write_data);
    ASSERT_TRUE(write_res.has_value());

    auto close_res = fs->close(fd);
    ASSERT_TRUE(close_res.has_value());

    // Corrupt four bytes on disk - RS should correct
    std::array<uint8_t, 512> sb_buf;
    static_vector<uint8_t> sb_data(sb_buf.data(), sb_buf.size());
    auto sb_read = disk.read(0, sizeof(SuperBlock), sb_data);
    ASSERT_TRUE(sb_read.has_value());
    SuperBlock sb;
    std::memcpy(&sb, sb_data.data(), sizeof(SuperBlock));

    size_t data_region = findDataBlockRegion(disk, sb);
    injectByteError(disk, data_region + 20, 0xFF);
    injectByteError(disk, data_region + 60, 0x00);
    injectByteError(disk, data_region + 120, 0xAA);
    injectByteError(disk, data_region + 180, 0x55);

    // Read back - should correct error
    auto open_res2 = fs->open("/test.txt");
    ASSERT_TRUE(open_res2.has_value());
    file_descriptor_t fd2 = open_res2.value();

    std::array<uint8_t, 2048> read_buf;
    static_vector<uint8_t> read_data(read_buf.data(), read_buf.size());
    auto read_res = fs->read(fd2, data_size, read_data);
    ASSERT_TRUE(read_res.has_value())
        << "Reed-Solomon should correct four byte error for " << GetParam().test_name;

    ASSERT_EQ(read_data.size(), data_size);
    for (size_t i = 0; i < data_size; ++i) {
        ASSERT_EQ(read_data[i], write_data_vec[i]) << "Data should be correctly recovered at index "
                                                   << i << " for " << GetParam().test_name;
    }

    auto close_res2 = fs->close(fd2);
    ASSERT_TRUE(close_res2.has_value());
}

TEST_P(PpFSParametrizedReedSolomonTest, ErrorCorrection_ReedSolomon_FiveByte)
{
    ASSERT_EQ(GetParam().ecc_type, ECCType::ReedSolomon) << "Test only for Reed-Solomon ECC";
    if (GetParam().rs_correctable_bytes < 5)
        GTEST_SKIP() << "Test requires rs_correctable_bytes >= 5";

    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());

    auto create_res = fs->create("/test.txt");
    ASSERT_TRUE(create_res.has_value());

    auto open_res = fs->open("/test.txt");
    ASSERT_TRUE(open_res.has_value());
    file_descriptor_t fd = open_res.value();

    // Write data
    size_t data_size = GetParam().block_size / 2;
    auto write_data_vec = createTestData(data_size, 0xEF);
    std::array<uint8_t, 2048> write_buf;
    static_vector<uint8_t> write_data(write_buf.data(), write_buf.size(), write_data_vec.size());
    std::memcpy(write_data.data(), write_data_vec.data(), write_data_vec.size());

    auto write_res = fs->write(fd, write_data);
    ASSERT_TRUE(write_res.has_value());

    auto close_res = fs->close(fd);
    ASSERT_TRUE(close_res.has_value());

    // Corrupt five bytes on disk - RS should correct
    std::array<uint8_t, 512> sb_buf;
    static_vector<uint8_t> sb_data(sb_buf.data(), sb_buf.size());
    auto sb_read = disk.read(0, sizeof(SuperBlock), sb_data);
    ASSERT_TRUE(sb_read.has_value());
    SuperBlock sb;
    std::memcpy(&sb, sb_data.data(), sizeof(SuperBlock));

    size_t data_region = findDataBlockRegion(disk, sb);
    injectByteError(disk, data_region + 15, 0x11);
    injectByteError(disk, data_region + 45, 0x22);
    injectByteError(disk, data_region + 75, 0x33);
    injectByteError(disk, data_region + 105, 0x44);
    injectByteError(disk, data_region + 135, 0x55);

    // Read back - should correct error
    auto open_res2 = fs->open("/test.txt");
    ASSERT_TRUE(open_res2.has_value());
    file_descriptor_t fd2 = open_res2.value();

    std::array<uint8_t, 2048> read_buf;
    static_vector<uint8_t> read_data(read_buf.data(), read_buf.size());
    auto read_res = fs->read(fd2, data_size, read_data);
    ASSERT_TRUE(read_res.has_value())
        << "Reed-Solomon should correct five byte error for " << GetParam().test_name;

    ASSERT_EQ(read_data.size(), data_size);
    for (size_t i = 0; i < data_size; ++i) {
        ASSERT_EQ(read_data[i], write_data_vec[i]) << "Data should be correctly recovered at index "
                                                   << i << " for " << GetParam().test_name;
    }

    auto close_res2 = fs->close(fd2);
    ASSERT_TRUE(close_res2.has_value());
}
