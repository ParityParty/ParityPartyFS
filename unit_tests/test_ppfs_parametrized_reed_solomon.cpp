#include "super_block_manager/super_block.hpp"
#include "test_ppfs_parametrized_helpers.hpp"
#include <cstring>
#include <gtest/gtest.h>

// Dedicated test class for Reed-Solomon ECC tests
class PpFSReedSolomonTest : public PpFSParametrizedTest { };

INSTANTIATE_TEST_SUITE_P(PpFSReedSolomon, PpFSReedSolomonTest,
    ::testing::ValuesIn(generateRSConfigs()), [](const ::testing::TestParamInfo<TestConfig>& info) {
        return sanitizeTestName(info.param.test_name);
    });

TEST_P(PpFSReedSolomonTest, ErrorCorrection_ReedSolomon_SingleByte)
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
    auto write_data = createTestData(data_size, 0x7E);

    auto write_res = fs->write(fd, write_data);
    ASSERT_TRUE(write_res.has_value());

    auto close_res = fs->close(fd);
    ASSERT_TRUE(close_res.has_value());

    // Corrupt one byte on disk - RS should correct
    auto sb_read = disk.read(0, sizeof(SuperBlock));
    ASSERT_TRUE(sb_read.has_value());
    SuperBlock sb;
    std::memcpy(&sb, sb_read.value().data(), sizeof(SuperBlock));

    size_t data_region = findDataBlockRegion(disk, sb);
    injectByteError(disk, data_region + 100, 0x00);

    // Read back - should correct error
    auto open_res2 = fs->open("/test.txt");
    ASSERT_TRUE(open_res2.has_value());
    file_descriptor_t fd2 = open_res2.value();

    auto read_res = fs->read(fd2, data_size);
    ASSERT_TRUE(read_res.has_value())
        << "Reed-Solomon should correct single byte error for " << GetParam().test_name;

    auto read_data = read_res.value();
    ASSERT_EQ(read_data.size(), data_size);
    ASSERT_EQ(read_data, write_data)
        << "Data should be correctly recovered for " << GetParam().test_name;

    auto close_res2 = fs->close(fd2);
    ASSERT_TRUE(close_res2.has_value());
}

TEST_P(PpFSReedSolomonTest, ErrorCorrection_ReedSolomon_DoubleByte)
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
    auto write_data = createTestData(data_size, 0xAB);

    auto write_res = fs->write(fd, write_data);
    ASSERT_TRUE(write_res.has_value());

    auto close_res = fs->close(fd);
    ASSERT_TRUE(close_res.has_value());

    // Corrupt two bytes on disk - RS should correct
    auto sb_read = disk.read(0, sizeof(SuperBlock));
    ASSERT_TRUE(sb_read.has_value());
    SuperBlock sb;
    std::memcpy(&sb, sb_read.value().data(), sizeof(SuperBlock));

    size_t data_region = findDataBlockRegion(disk, sb);
    injectByteError(disk, data_region + 50, 0xEE);
    injectByteError(disk, data_region + 150, 0x44);

    // Read back - should correct error
    auto open_res2 = fs->open("/test.txt");
    ASSERT_TRUE(open_res2.has_value());
    file_descriptor_t fd2 = open_res2.value();

    auto read_res = fs->read(fd2, data_size);
    ASSERT_TRUE(read_res.has_value())
        << "Reed-Solomon should correct double byte error for " << GetParam().test_name;

    auto read_data = read_res.value();
    ASSERT_EQ(read_data.size(), data_size);
    ASSERT_EQ(read_data, write_data)
        << "Data should be correctly recovered for " << GetParam().test_name;

    auto close_res2 = fs->close(fd2);
    ASSERT_TRUE(close_res2.has_value());
}

TEST_P(PpFSReedSolomonTest, ErrorCorrection_ReedSolomon_TripleByte)
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
    auto write_data = createTestData(data_size, 0xAB);

    auto write_res = fs->write(fd, write_data);
    ASSERT_TRUE(write_res.has_value());

    auto close_res = fs->close(fd);
    ASSERT_TRUE(close_res.has_value());

    // Corrupt three bytes on disk - RS should correct
    auto sb_read = disk.read(0, sizeof(SuperBlock));
    ASSERT_TRUE(sb_read.has_value());
    SuperBlock sb;
    std::memcpy(&sb, sb_read.value().data(), sizeof(SuperBlock));

    size_t data_region = findDataBlockRegion(disk, sb);
    injectByteError(disk, data_region + 10, 0xEE);
    injectByteError(disk, data_region + 100, 0x61);
    injectByteError(disk, data_region + 200, 0x44);

    // Read back - should correct error
    auto open_res2 = fs->open("/test.txt");
    ASSERT_TRUE(open_res2.has_value());
    file_descriptor_t fd2 = open_res2.value();

    auto read_res = fs->read(fd2, data_size);
    ASSERT_TRUE(read_res.has_value())
        << "Reed-Solomon should correct triple byte error for " << GetParam().test_name;

    auto read_data = read_res.value();
    ASSERT_EQ(read_data.size(), data_size);
    ASSERT_EQ(read_data, write_data)
        << "Data should be correctly recovered for " << GetParam().test_name;

    auto close_res2 = fs->close(fd2);
    ASSERT_TRUE(close_res2.has_value());
}

TEST_P(PpFSReedSolomonTest, ErrorCorrection_ReedSolomon_FourByte)
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
    auto write_data = createTestData(data_size, 0xCD);

    auto write_res = fs->write(fd, write_data);
    ASSERT_TRUE(write_res.has_value());

    auto close_res = fs->close(fd);
    ASSERT_TRUE(close_res.has_value());

    // Corrupt four bytes on disk - RS should correct
    auto sb_read = disk.read(0, sizeof(SuperBlock));
    ASSERT_TRUE(sb_read.has_value());
    SuperBlock sb;
    std::memcpy(&sb, sb_read.value().data(), sizeof(SuperBlock));

    size_t data_region = findDataBlockRegion(disk, sb);
    injectByteError(disk, data_region + 20, 0xFF);
    injectByteError(disk, data_region + 60, 0x00);
    injectByteError(disk, data_region + 120, 0xAA);
    injectByteError(disk, data_region + 180, 0x55);

    // Read back - should correct error
    auto open_res2 = fs->open("/test.txt");
    ASSERT_TRUE(open_res2.has_value());
    file_descriptor_t fd2 = open_res2.value();

    auto read_res = fs->read(fd2, data_size);
    ASSERT_TRUE(read_res.has_value())
        << "Reed-Solomon should correct four byte error for " << GetParam().test_name;

    auto read_data = read_res.value();
    ASSERT_EQ(read_data.size(), data_size);
    ASSERT_EQ(read_data, write_data)
        << "Data should be correctly recovered for " << GetParam().test_name;

    auto close_res2 = fs->close(fd2);
    ASSERT_TRUE(close_res2.has_value());
}

TEST_P(PpFSReedSolomonTest, ErrorCorrection_ReedSolomon_FiveByte)
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
    auto write_data = createTestData(data_size, 0xEF);

    auto write_res = fs->write(fd, write_data);
    ASSERT_TRUE(write_res.has_value());

    auto close_res = fs->close(fd);
    ASSERT_TRUE(close_res.has_value());

    // Corrupt five bytes on disk - RS should correct
    auto sb_read = disk.read(0, sizeof(SuperBlock));
    ASSERT_TRUE(sb_read.has_value());
    SuperBlock sb;
    std::memcpy(&sb, sb_read.value().data(), sizeof(SuperBlock));

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

    auto read_res = fs->read(fd2, data_size);
    ASSERT_TRUE(read_res.has_value())
        << "Reed-Solomon should correct five byte error for " << GetParam().test_name;

    auto read_data = read_res.value();
    ASSERT_EQ(read_data.size(), data_size);
    ASSERT_EQ(read_data, write_data)
        << "Data should be correctly recovered for " << GetParam().test_name;

    auto close_res2 = fs->close(fd2);
    ASSERT_TRUE(close_res2.has_value());
}
