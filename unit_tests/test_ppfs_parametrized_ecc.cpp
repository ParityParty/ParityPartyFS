#include "super_block_manager/super_block.hpp"
#include "test_ppfs_parametrized_helpers.hpp"
#include <cstring>
#include <gtest/gtest.h>

INSTANTIATE_TEST_SUITE_P(PpFSEcc, PpFSParametrizedTest, ::testing::ValuesIn(generateTestConfigs()),
    [](const ::testing::TestParamInfo<TestConfig>& info) {
        return sanitizeTestName(info.param.test_name);
    });

TEST_P(PpFSParametrizedTest, ErrorCorrection_None_NoCorrection)
{
    if (GetParam().ecc_type != ECCType::None) {
        GTEST_SKIP() << "Test only for None ECC type";
    }

    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());

    auto create_res = fs->create("/test.txt");
    ASSERT_TRUE(create_res.has_value());

    auto open_res = fs->open("/test.txt");
    ASSERT_TRUE(open_res.has_value());
    file_descriptor_t fd = open_res.value();

    // Write data
    size_t data_size = GetParam().block_size;
    auto write_data = createTestData(data_size, 0xAA);

    auto write_res = fs->write(fd, write_data);
    ASSERT_TRUE(write_res.has_value());

    auto close_res = fs->close(fd);
    ASSERT_TRUE(close_res.has_value());

    // Corrupt data on disk (None ECC doesn't detect/correct)
    // Read superblock to find data region
    auto sb_read = disk.read(0, sizeof(SuperBlock));
    ASSERT_TRUE(sb_read.has_value());
    SuperBlock sb;
    std::memcpy(&sb, sb_read.value().data(), sizeof(SuperBlock));

    size_t data_region = findDataBlockRegion(disk, sb);
    injectBitFlip(disk, data_region + 100, 0x04);

    // Read back - should succeed but data may be corrupted
    auto open_res2 = fs->open("/test.txt");
    ASSERT_TRUE(open_res2.has_value());
    file_descriptor_t fd2 = open_res2.value();

    auto read_res = fs->read(fd2, data_size);
    // None ECC doesn't detect errors, so read may succeed with corrupted data
    // This is expected behavior

    auto close_res2 = fs->close(fd2);
    ASSERT_TRUE(close_res2.has_value());
}

TEST_P(PpFSParametrizedTest, ErrorDetection_Parity_SingleBitFlip)
{
    if (GetParam().ecc_type != ECCType::Parity) {
        GTEST_SKIP() << "Test only for Parity ECC type";
    }

    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());

    auto create_res = fs->create("/test.txt");
    ASSERT_TRUE(create_res.has_value());

    auto open_res = fs->open("/test.txt");
    ASSERT_TRUE(open_res.has_value());
    file_descriptor_t fd = open_res.value();

    // Write data
    size_t data_size = GetParam().block_size;
    auto write_data = createTestData(data_size, 0x55);

    auto write_res = fs->write(fd, write_data);
    ASSERT_TRUE(write_res.has_value());

    auto close_res = fs->close(fd);
    ASSERT_TRUE(close_res.has_value());

    // Corrupt data on disk - Parity detects but doesn't correct
    auto sb_read = disk.read(0, sizeof(SuperBlock));
    ASSERT_TRUE(sb_read.has_value());
    SuperBlock sb;
    std::memcpy(&sb, sb_read.value().data(), sizeof(SuperBlock));

    size_t data_region = findDataBlockRegion(disk, sb);
    injectBitFlip(disk, data_region + 50, 0x08);

    // Read back - should detect error
    auto open_res2 = fs->open("/test.txt");
    ASSERT_TRUE(open_res2.has_value());
    file_descriptor_t fd2 = open_res2.value();

    auto read_res = fs->read(fd2, data_size);
    ASSERT_FALSE(read_res.has_value())
        << "Parity should detect single bit flip for " << GetParam().test_name;
    ASSERT_EQ(read_res.error(), FsError::BlockDevice_CorrectionError)
        << "Should return CorrectionError for " << GetParam().test_name;

    auto close_res2 = fs->close(fd2);
    ASSERT_TRUE(close_res2.has_value());
}

TEST_P(PpFSParametrizedTest, ErrorDetection_CRC_SingleBitFlip)
{
    if (GetParam().ecc_type != ECCType::Crc) {
        GTEST_SKIP() << "Test only for CRC ECC type";
    }

    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());

    auto create_res = fs->create("/test.txt");
    ASSERT_TRUE(create_res.has_value());

    auto open_res = fs->open("/test.txt");
    ASSERT_TRUE(open_res.has_value());
    file_descriptor_t fd = open_res.value();

    // Write data
    size_t data_size = GetParam().block_size;
    auto write_data = createTestData(data_size, 0xAA);

    auto write_res = fs->write(fd, write_data);
    ASSERT_TRUE(write_res.has_value());

    auto close_res = fs->close(fd);
    ASSERT_TRUE(close_res.has_value());

    // Corrupt data on disk - CRC detects but doesn't correct
    auto sb_read = disk.read(0, sizeof(SuperBlock));
    ASSERT_TRUE(sb_read.has_value());
    SuperBlock sb;
    std::memcpy(&sb, sb_read.value().data(), sizeof(SuperBlock));

    size_t data_region = findDataBlockRegion(disk, sb);

    // One block for directory entries of root dir, then there should be the datablock
    injectBitFlip(disk, data_region + config.block_size + 1, 0x01);

    // Read back - should detect error
    auto open_res2 = fs->open("/test.txt");
    ASSERT_TRUE(open_res2.has_value());
    file_descriptor_t fd2 = open_res2.value();

    auto read_res = fs->read(fd2, data_size);
    ASSERT_FALSE(read_res.has_value())
        << "CRC should detect single bit flip for " << GetParam().test_name;
    ASSERT_EQ(read_res.error(), FsError::BlockDevice_CorrectionError)
        << "Should return CorrectionError for " << GetParam().test_name;

    auto close_res2 = fs->close(fd2);
    ASSERT_TRUE(close_res2.has_value());
}

TEST_P(PpFSParametrizedTest, ErrorDetection_CRC_MultipleBitFlips)
{
    if (GetParam().ecc_type != ECCType::Crc) {
        GTEST_SKIP() << "Test only for CRC ECC type";
    }

    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());

    auto create_res = fs->create("/test.txt");
    ASSERT_TRUE(create_res.has_value());

    auto open_res = fs->open("/test.txt");
    ASSERT_TRUE(open_res.has_value());
    file_descriptor_t fd = open_res.value();

    // Write data
    size_t data_size = GetParam().block_size;
    auto write_data = createTestData(data_size, 0x42);

    auto write_res = fs->write(fd, write_data);
    ASSERT_TRUE(write_res.has_value());

    auto close_res = fs->close(fd);
    ASSERT_TRUE(close_res.has_value());

    // Corrupt multiple bits on disk
    auto sb_read = disk.read(0, sizeof(SuperBlock));
    ASSERT_TRUE(sb_read.has_value());
    SuperBlock sb;
    std::memcpy(&sb, sb_read.value().data(), sizeof(SuperBlock));

    size_t data_region = findDataBlockRegion(disk, sb);
    injectBitFlip(disk, data_region + GetParam().block_size + 30, 0x02);
    injectBitFlip(disk, data_region + GetParam().block_size + 60, 0x04);
    injectBitFlip(disk, data_region + GetParam().block_size + 90, 0x08);

    // Read back - should detect error
    auto open_res2 = fs->open("/test.txt");
    ASSERT_TRUE(open_res2.has_value());
    file_descriptor_t fd2 = open_res2.value();

    auto read_res = fs->read(fd2, data_size);
    ASSERT_FALSE(read_res.has_value())
        << "CRC should detect multiple bit flips for " << GetParam().test_name;
    ASSERT_EQ(read_res.error(), FsError::BlockDevice_CorrectionError)
        << "Should return CorrectionError for " << GetParam().test_name;

    auto close_res2 = fs->close(fd2);
    ASSERT_TRUE(close_res2.has_value());
}

TEST_P(PpFSParametrizedTest, ErrorCorrection_Hamming_SingleBitFlip)
{
    if (GetParam().ecc_type != ECCType::Hamming) {
        GTEST_SKIP() << "Test only for Hamming ECC type";
    }

    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());

    auto create_res = fs->create("/test.txt");
    ASSERT_TRUE(create_res.has_value());

    auto open_res = fs->open("/test.txt");
    ASSERT_TRUE(open_res.has_value());
    file_descriptor_t fd = open_res.value();

    // Write data
    size_t data_size = GetParam().block_size / 2; // Hamming has overhead
    auto write_data = createTestData(data_size, 0xAA);

    auto write_res = fs->write(fd, write_data);
    ASSERT_TRUE(write_res.has_value());

    auto close_res = fs->close(fd);
    ASSERT_TRUE(close_res.has_value());

    // Corrupt single bit on disk - Hamming should correct
    auto sb_read = disk.read(0, sizeof(SuperBlock));
    ASSERT_TRUE(sb_read.has_value());
    SuperBlock sb;
    std::memcpy(&sb, sb_read.value().data(), sizeof(SuperBlock));

    size_t data_region = findDataBlockRegion(disk, sb);
    injectBitFlip(disk, data_region + 40, 0x10);

    // Read back - should correct error
    auto open_res2 = fs->open("/test.txt");
    ASSERT_TRUE(open_res2.has_value());
    file_descriptor_t fd2 = open_res2.value();

    auto read_res = fs->read(fd2, data_size);
    ASSERT_TRUE(read_res.has_value())
        << "Hamming should correct single bit flip for " << GetParam().test_name;

    auto read_data = read_res.value();
    ASSERT_EQ(read_data.size(), data_size);
    ASSERT_EQ(read_data, write_data)
        << "Data should be correctly recovered for " << GetParam().test_name;

    auto close_res2 = fs->close(fd2);
    ASSERT_TRUE(close_res2.has_value());
}

TEST_P(PpFSParametrizedTest, ErrorDetection_Hamming_DoubleBitFlip)
{
    if (GetParam().ecc_type != ECCType::Hamming) {
        GTEST_SKIP() << "Test only for Hamming ECC type";
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
    auto write_data = createTestData(data_size, 0x55);

    auto write_res = fs->write(fd, write_data);
    ASSERT_TRUE(write_res.has_value());

    auto close_res = fs->close(fd);
    ASSERT_TRUE(close_res.has_value());

    // Corrupt two bits on disk - Hamming should fail
    auto sb_read = disk.read(0, sizeof(SuperBlock));
    ASSERT_TRUE(sb_read.has_value());
    SuperBlock sb;
    std::memcpy(&sb, sb_read.value().data(), sizeof(SuperBlock));

    size_t data_region = findDataBlockRegion(disk, sb);
    injectBitFlip(disk, data_region + GetParam().block_size + 20, 0x20);
    injectBitFlip(disk, data_region + GetParam().block_size + 50, 0x40);

    // Read back - should detect uncorrectable error
    auto open_res2 = fs->open("/test.txt");
    ASSERT_TRUE(open_res2.has_value());
    file_descriptor_t fd2 = open_res2.value();

    auto read_res = fs->read(fd2, data_size);
    ASSERT_FALSE(read_res.has_value())
        << "Hamming should fail on double bit flip for " << GetParam().test_name;
    ASSERT_EQ(read_res.error(), FsError::BlockDevice_CorrectionError)
        << "Should return CorrectionError for " << GetParam().test_name;

    auto close_res2 = fs->close(fd2);
    ASSERT_TRUE(close_res2.has_value());
}

TEST_P(PpFSParametrizedTest, ErrorCorrection_ReedSolomon_SingleByte)
{
    if (GetParam().ecc_type != ECCType::ReedSolomon || GetParam().rs_correctable_bytes < 1) {
        GTEST_SKIP() << "Test only for Reed-Solomon with rs_correctable_bytes >= 1";
    }

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

TEST_P(PpFSParametrizedTest, ErrorCorrection_ReedSolomon_DoubleByte)
{
    if (GetParam().ecc_type != ECCType::ReedSolomon || GetParam().rs_correctable_bytes < 2) {
        GTEST_SKIP() << "Test only for Reed-Solomon with rs_correctable_bytes >= 2";
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

TEST_P(PpFSParametrizedTest, ErrorCorrection_ReedSolomon_TripleByte)
{
    if (GetParam().ecc_type != ECCType::ReedSolomon || GetParam().rs_correctable_bytes < 3) {
        GTEST_SKIP() << "Test only for Reed-Solomon with rs_correctable_bytes >= 3";
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

TEST_P(PpFSParametrizedTest, ErrorCorrection_ReedSolomon_FourByte)
{
    if (GetParam().ecc_type != ECCType::ReedSolomon || GetParam().rs_correctable_bytes < 4) {
        GTEST_SKIP() << "Test only for Reed-Solomon with rs_correctable_bytes >= 4";
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

TEST_P(PpFSParametrizedTest, ErrorCorrection_ReedSolomon_FiveByte)
{
    if (GetParam().ecc_type != ECCType::ReedSolomon || GetParam().rs_correctable_bytes < 5) {
        GTEST_SKIP() << "Test only for Reed-Solomon with rs_correctable_bytes >= 5";
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
