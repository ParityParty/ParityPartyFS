#include "super_block_manager/super_block.hpp"
#include "test_ppfs_parametrized_helpers.hpp"
#include <cstring>
#include <gtest/gtest.h>

// Dedicated test class for CRC ECC tests
class PpFSParametrizedCrcTest : public PpFSParametrizedTest { };

INSTANTIATE_TEST_SUITE_P(PpFSCrc, PpFSParametrizedCrcTest,
    ::testing::ValuesIn(generateCrcConfigs()),
    [](const ::testing::TestParamInfo<TestConfig>& info) {
        return sanitizeTestName(info.param.test_name);
    });

TEST_P(PpFSParametrizedCrcTest, ErrorDetection_CRC_SingleBitFlip)
{
    ASSERT_EQ(GetParam().ecc_type, ECCType::Crc) << "Test only for CRC ECC type";

    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());

    auto create_res = fs->create("/test.txt");
    ASSERT_TRUE(create_res.has_value());

    auto open_res = fs->open("/test.txt");
    ASSERT_TRUE(open_res.has_value());
    file_descriptor_t fd = open_res.value();

    // Write data
    size_t data_size = GetParam().block_size;
    auto write_data_vec = createTestData(data_size, 0xAA);
    std::array<uint8_t, 4096> write_buf;
    static_vector<uint8_t> write_data(write_buf.data(), write_buf.size(), write_data_vec.size());
    std::memcpy(write_data.data(), write_data_vec.data(), write_data_vec.size());

    auto write_res = fs->write(fd, write_data);
    ASSERT_TRUE(write_res.has_value());

    auto close_res = fs->close(fd);
    ASSERT_TRUE(close_res.has_value());

    // Corrupt data on disk - CRC detects but doesn't correct
    std::array<uint8_t, 512> sb_buf;
    static_vector<uint8_t> sb_data(sb_buf.data(), sb_buf.size());
    auto sb_read = disk.read(0, sizeof(SuperBlock), sb_data);
    ASSERT_TRUE(sb_read.has_value());
    SuperBlock sb;
    std::memcpy(&sb, sb_data.data(), sizeof(SuperBlock));

    size_t data_region = findDataBlockRegion(disk, sb);

    // One block for directory entries of root dir, then there should be the datablock
    injectBitFlip(disk, data_region + config.block_size + 1, 0x01);

    // Read back - should detect error
    auto open_res2 = fs->open("/test.txt");
    ASSERT_TRUE(open_res2.has_value());
    file_descriptor_t fd2 = open_res2.value();

    std::array<uint8_t, 4096> read_buf;
    static_vector<uint8_t> read_data(read_buf.data(), read_buf.size());
    auto read_res = fs->read(fd2, data_size, read_data);
    ASSERT_FALSE(read_res.has_value())
        << "CRC should detect single bit flip for " << GetParam().test_name;
    ASSERT_EQ(read_res.error(), FsError::BlockDevice_CorrectionError)
        << "Should return CorrectionError for " << GetParam().test_name;

    auto close_res2 = fs->close(fd2);
    ASSERT_TRUE(close_res2.has_value());
}

TEST_P(PpFSParametrizedCrcTest, ErrorDetection_CRC_MultipleBitFlips)
{
    ASSERT_EQ(GetParam().ecc_type, ECCType::Crc) << "Test only for CRC ECC type";

    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());

    auto create_res = fs->create("/test.txt");
    ASSERT_TRUE(create_res.has_value());

    auto open_res = fs->open("/test.txt");
    ASSERT_TRUE(open_res.has_value());
    file_descriptor_t fd = open_res.value();

    // Write data
    size_t data_size = GetParam().block_size;
    auto write_data_vec = createTestData(data_size, 0x42);
    std::array<uint8_t, 4096> write_buf;
    static_vector<uint8_t> write_data(write_buf.data(), write_buf.size(), write_data_vec.size());
    std::memcpy(write_data.data(), write_data_vec.data(), write_data_vec.size());

    auto write_res = fs->write(fd, write_data);
    ASSERT_TRUE(write_res.has_value());

    auto close_res = fs->close(fd);
    ASSERT_TRUE(close_res.has_value());

    // Corrupt multiple bits on disk
    std::array<uint8_t, 512> sb_buf;
    static_vector<uint8_t> sb_data(sb_buf.data(), sb_buf.size());
    auto sb_read = disk.read(0, sizeof(SuperBlock), sb_data);
    ASSERT_TRUE(sb_read.has_value());
    SuperBlock sb;
    std::memcpy(&sb, sb_data.data(), sizeof(SuperBlock));

    size_t data_region = findDataBlockRegion(disk, sb);
    injectBitFlip(disk, data_region + GetParam().block_size + 30, 0x02);
    injectBitFlip(disk, data_region + GetParam().block_size + 60, 0x04);
    injectBitFlip(disk, data_region + GetParam().block_size + 90, 0x08);

    // Read back - should detect error
    auto open_res2 = fs->open("/test.txt");
    ASSERT_TRUE(open_res2.has_value());
    file_descriptor_t fd2 = open_res2.value();

    std::array<uint8_t, 4096> read_buf;
    static_vector<uint8_t> read_data(read_buf.data(), read_buf.size());
    auto read_res = fs->read(fd2, data_size, read_data);
    ASSERT_FALSE(read_res.has_value())
        << "CRC should detect multiple bit flips for " << GetParam().test_name;
    ASSERT_EQ(read_res.error(), FsError::BlockDevice_CorrectionError)
        << "Should return CorrectionError for " << GetParam().test_name;

    auto close_res2 = fs->close(fd2);
    ASSERT_TRUE(close_res2.has_value());
}
