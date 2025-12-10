#include "super_block_manager/super_block.hpp"
#include "test_ppfs_parametrized_helpers.hpp"
#include <cstring>
#include <gtest/gtest.h>

// Dedicated test class for None ECC tests
class PpFSNoneTest : public PpFSParametrizedTest { };

INSTANTIATE_TEST_SUITE_P(PpFSNone, PpFSNoneTest, ::testing::ValuesIn(generateNonEccConfigs()),
    [](const ::testing::TestParamInfo<TestConfig>& info) {
        return sanitizeTestName(info.param.test_name);
    });

TEST_P(PpFSNoneTest, ErrorCorrection_None_NoCorrection)
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
