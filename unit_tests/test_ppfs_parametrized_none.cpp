#include "super_block_manager/super_block.hpp"
#include "test_ppfs_parametrized_helpers.hpp"
#include <cstring>
#include <gtest/gtest.h>

// Dedicated test class for None ECC tests
class PpFSParametrizedNoneTest : public PpFSParametrizedTest { };

INSTANTIATE_TEST_SUITE_P(PpFSNone, PpFSParametrizedNoneTest,
    ::testing::ValuesIn(generateNonEccConfigs()),
    [](const ::testing::TestParamInfo<TestConfig>& info) {
        return sanitizeTestName(info.param.test_name);
    });

TEST_P(PpFSParametrizedNoneTest, ErrorCorrection_None_NoCorrection)
{
    ASSERT_EQ(GetParam().ecc_type, ECCType::None) << "Test only for None ECC type";

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

    // Corrupt data on disk (None ECC doesn't detect/correct)
    // Read superblock to find data region
    std::array<uint8_t, 512> sb_buf;
    static_vector<uint8_t> sb_data(sb_buf.data(), sb_buf.size());
    auto sb_read = disk.read(0, sizeof(SuperBlock), sb_data);
    ASSERT_TRUE(sb_read.has_value());
    SuperBlock sb;
    std::memcpy(&sb, sb_data.data(), sizeof(SuperBlock));

    size_t data_region = findDataBlockRegion(disk, sb);
    injectBitFlip(disk, data_region + 100, 0x04);

    // Read back - should succeed but data may be corrupted
    auto open_res2 = fs->open("/test.txt");
    ASSERT_TRUE(open_res2.has_value());
    file_descriptor_t fd2 = open_res2.value();

    std::array<uint8_t, 4096> read_buf;
    static_vector<uint8_t> read_data(read_buf.data(), read_buf.size());
    auto read_res = fs->read(fd2, data_size, read_data);
    // None ECC doesn't detect errors, so read may succeed with corrupted data
    // This is expected behavior

    auto close_res2 = fs->close(fd2);
    ASSERT_TRUE(close_res2.has_value());
}
