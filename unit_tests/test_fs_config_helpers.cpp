#include "ppfs/filesystem/fs_config_helpers.hpp"

#include <fcntl.h>
#include <gtest/gtest.h>
#include <string>
#include <unistd.h>

static std::string write_temp_config(const std::string& content)
{
    char tmpl[] = "/tmp/ppfs_cfg_XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd == -1)
        throw std::runtime_error("mkstemp failed");

    ssize_t written = write(fd, content.data(), content.size());
    if (written != static_cast<ssize_t>(content.size())) {
        close(fd);
        unlink(tmpl);
        throw std::runtime_error("write failed");
    }

    close(fd);
    return std::string(tmpl);
}

TEST(ConfigLoader, ValidMinimalConfig)
{
    auto path = write_temp_config(R"(
        total_size = 1048576
        average_file_size = 4096
        block_size = 512
        ecc_type = none
        use_journal = false
    )");

    auto res = load_fs_config(path);

    ASSERT_TRUE(res.has_value()) << "Error: " << toString(res.error());
    EXPECT_EQ(res->total_size, 1048576);
    EXPECT_EQ(res->average_file_size, 4096);
    EXPECT_EQ(res->block_size, 512);
    EXPECT_EQ(res->ecc_type, ECCType::None);
    EXPECT_FALSE(res->use_journal);
}

TEST(ConfigLoader, MissingRequiredField)
{
    auto path = write_temp_config(R"(
        total_size = 1048576
        block_size = 512
        ecc_type = none
    )");

    auto res = load_fs_config(path);

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), FsError::Config_MissingField);
}

TEST(ConfigLoader, UnknownKey)
{
    auto path = write_temp_config(R"(
        total_size = 1048576
        average_file_size = 4096
        block_size = 512
        ecc_type = none
        potato = 123
    )");

    auto res = load_fs_config(path);

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), FsError::Config_UnknownKey);
}

TEST(ConfigLoader, InvalidNumericValue)
{
    auto path = write_temp_config(R"(
        total_size = abc
        average_file_size = 4096
        block_size = 512
        ecc_type = none
    )");

    auto res = load_fs_config(path);

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), FsError::Config_InvalidValue);
}

TEST(ConfigLoader, CrcWithHexPolynomial)
{
    auto path = write_temp_config(R"(
        total_size = 1048576
        average_file_size = 4096
        block_size = 512
        ecc_type = crc
        crc_polynomial = 0x9960034c
        use_journal = true
    )");

    auto res = load_fs_config(path);

    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->ecc_type, ECCType::Crc);
}

TEST(ConfigLoader, CrcMissingPolynomial)
{
    auto path = write_temp_config(R"(
        total_size = 1048576
        average_file_size = 4096
        block_size = 512
        ecc_type = crc
    )");

    auto res = load_fs_config(path);

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), FsError::Config_MissingField);
}

TEST(ConfigLoader, ReedSolomonMissingField)
{
    auto path = write_temp_config(R"(
        total_size = 1048576
        average_file_size = 4096
        block_size = 512
        ecc_type = reed_solomon
    )");

    auto res = load_fs_config(path);

    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), FsError::Config_MissingField)
        << "Expected missing field error for rs_correctable_bytes, got " << toString(res.error());
}

TEST(ConfigLoader, HandlesCommentsAndEmptyLines)
{
    auto path = write_temp_config(R"(
        # Global comment
        // Another comment style

        total_size = 1048576        # size in bytes
        average_file_size = 4096   // avg file size

        block_size = 512

        # ECC configuration
        ecc_type = crc
        crc_polynomial = 0x9960034c   # hex polynomial

        use_journal = false

        // trailing empty lines below


    )");

    auto res = load_fs_config(path);

    unlink(path.c_str());

    ASSERT_TRUE(res.has_value());

    const FsConfig& cfg = *res;

    EXPECT_EQ(cfg.total_size, 1048576u);
    EXPECT_EQ(cfg.average_file_size, 4096u);
    EXPECT_EQ(cfg.block_size, 512u);
    EXPECT_EQ(cfg.ecc_type, ECCType::Crc);
    EXPECT_FALSE(cfg.use_journal);
}
