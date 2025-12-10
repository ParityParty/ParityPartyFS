#include "test_ppfs_parametrized_helpers.hpp"
#include "super_block_manager/super_block.hpp"
#include <gtest/gtest.h>
#include <cstring>

INSTANTIATE_TEST_SUITE_P(
    PpFSFormat,
    PpFSParametrizedTest,
    ::testing::ValuesIn(generateTestConfigs()),
    [](const ::testing::TestParamInfo<TestConfig>& info) {
        return sanitizeTestName(info.param.test_name);
    });

TEST_P(PpFSParametrizedTest, Format_Succeeds)
{
    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value())
        << "Format failed for " << GetParam().test_name << ": " << toString(format_res.error());
}

TEST_P(PpFSParametrizedTest, Format_IsInitialized)
{
    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());
    
    ASSERT_TRUE(fs->isInitialized())
        << "Filesystem should be initialized after format for " << GetParam().test_name;
}

TEST_P(PpFSParametrizedTest, Format_ProducesValidSuperblock)
{
    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value());
    
    // Verify superblock signature
    auto read_res = disk.read(0, sizeof(SuperBlock));
    ASSERT_TRUE(read_res.has_value());
    
    const auto& data = read_res.value();
    SuperBlock sb;
    std::memcpy(&sb, data.data(), sizeof(SuperBlock));
    
    ASSERT_EQ(std::memcmp(sb.signature, "PPFS", 4), 0)
        << "Superblock signature should be 'PPFS' for " << GetParam().test_name;
    
    // Verify configuration matches
    ASSERT_EQ(sb.block_size, config.block_size)
        << "Block size mismatch for " << GetParam().test_name;
    ASSERT_EQ(sb.ecc_type, config.ecc_type)
        << "ECC type mismatch for " << GetParam().test_name;
    
    if (config.ecc_type == ECCType::ReedSolomon) {
        ASSERT_EQ(sb.rs_correctable_bytes, config.rs_correctable_bytes)
            << "RS correctable bytes mismatch for " << GetParam().test_name;
    }
    
    // Verify superblock addresses are valid
    ASSERT_GT(sb.total_blocks, 0)
        << "Total blocks should be > 0 for " << GetParam().test_name;
    ASSERT_GT(sb.total_inodes, 0)
        << "Total inodes should be > 0 for " << GetParam().test_name;
    ASSERT_LT(sb.first_data_blocks_address, sb.last_data_block_address)
        << "First data block address should be < last for " << GetParam().test_name;
}

