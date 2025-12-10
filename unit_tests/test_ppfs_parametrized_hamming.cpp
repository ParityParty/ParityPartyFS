#include "super_block_manager/super_block.hpp"
#include "test_ppfs_parametrized_helpers.hpp"
#include <cstring>
#include <gtest/gtest.h>

// Dedicated test class for Hamming ECC tests
class PpFSHammingTest : public PpFSParametrizedTest { };

INSTANTIATE_TEST_SUITE_P(PpFSHamming, PpFSHammingTest,
    ::testing::ValuesIn(generateHammingConfigs()),
    [](const ::testing::TestParamInfo<TestConfig>& info) {
        return sanitizeTestName(info.param.test_name);
    });

TEST_P(PpFSHammingTest, ErrorCorrection_Hamming_SingleBitFlip)
{
    ASSERT_EQ(GetParam().ecc_type, ECCType::Hamming) << "Test only for Hamming ECC";

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

TEST_P(PpFSHammingTest, ErrorDetection_Hamming_DoubleBitFlip)
{
    ASSERT_EQ(GetParam().ecc_type, ECCType::Hamming) << "Test only for Hamming ECC";

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
