#include "test_ppfs_parametrized_helpers.hpp"
#include "super_block_manager/super_block.hpp"
#include <gtest/gtest.h>
#include <cstring>

INSTANTIATE_TEST_SUITE_P(
    PpFSInit,
    PpFSParametrizedTest,
    ::testing::ValuesIn(generateTestConfigs()),
    [](const ::testing::TestParamInfo<TestConfig>& info) {
        return sanitizeTestName(info.param.test_name);
    });

TEST_P(PpFSParametrizedTest, Init_Succeeds_AfterFormat)
{
    // Format the filesystem
    auto format_res = fs->format(config);
    ASSERT_TRUE(format_res.has_value())
        << "Format failed for " << GetParam().test_name << ": " << toString(format_res.error());
    
    
    PpFS new_fs(disk);
    auto init_res = new_fs.init();
    ASSERT_TRUE(init_res.has_value())
        << "Init failed for " << GetParam().test_name << ": " << toString(init_res.error());
    
    ASSERT_TRUE(new_fs.isInitialized())
        << "Filesystem should be initialized after init for " << GetParam().test_name;
}


TEST_P(PpFSParametrizedTest, Init_Fails_NoFormat)
{
    // Try to initialize without formatting
    auto init_res = fs->init();
    ASSERT_FALSE(init_res.has_value())
        << "Init should fail on unformatted disk for " << GetParam().test_name;
    ASSERT_EQ(init_res.error(), FsError::PpFS_DiskNotFormatted)
        << "Init should return DiskNotFormatted error for " << GetParam().test_name;
    
    ASSERT_FALSE(fs->isInitialized())
        << "Filesystem should not be initialized after failed init for " << GetParam().test_name;
}

