#include "array"
#include "ppfs/common/static_vector.hpp"
#include "ppfs/disk/file_disk.hpp"

#include <cstdio>
#include <gtest/gtest.h>
#include <string>
#include <unistd.h>

static std::string make_temp_file()
{
    char tmpl[] = "/tmp/filedisk-test-XXXXXX";
    int fd = mkstemp(tmpl);
    EXPECT_NE(fd, -1);
    close(fd);
    return std::string(tmpl);
}

TEST(FileDisk, Compiles)
{
    FileDisk disk;
    SUCCEED();
}

TEST(FileDisk, CreateAndSize)
{
    auto path = make_temp_file();
    FileDisk disk;

    constexpr size_t DISK_SIZE = 1024;

    auto res = disk.create(path, DISK_SIZE);
    ASSERT_TRUE(res.has_value());

    EXPECT_EQ(disk.size(), DISK_SIZE);

    unlink(path.c_str());
}

TEST(FileDisk, WritesAndReads)
{
    auto path = make_temp_file();
    FileDisk disk;

    disk.create(path, 16);

    std::array<uint8_t, 4> src = { 1, 2, 3, 4 };
    static_vector<uint8_t> write_data(src.data(), src.size(), src.size());

    auto write_res = disk.write(0, write_data);
    ASSERT_TRUE(write_res.has_value());
    EXPECT_EQ(write_res.value(), write_data.size());

    std::array<uint8_t, 4> dst;
    static_vector read_data(dst.data(), dst.size());

    auto read_res = disk.read(0, dst.size(), read_data);
    ASSERT_TRUE(read_res.has_value());

    EXPECT_EQ(read_data.size(), write_data.size());
    for (size_t i = 0; i < dst.size(); ++i) {
        EXPECT_EQ(dst[i], src[i]);
    }

    unlink(path.c_str());
}

TEST(FileDisk, OutOfBounds)
{
    auto path = make_temp_file();
    FileDisk disk;

    disk.create(path, 8);

    std::array<uint8_t, 4> buf;
    static_vector data(buf.data(), buf.size());

    auto read_res = disk.read(6, 4, data);
    EXPECT_FALSE(read_res.has_value());
    EXPECT_EQ(read_res.error(), FsError::Disk_OutOfBounds);

    std::array<uint8_t, 4> src = { 1, 2, 3, 4 };
    static_vector write_data(src.data(), src.size(), src.size());

    auto write_res = disk.write(6, write_data);
    EXPECT_FALSE(write_res.has_value());
    EXPECT_EQ(write_res.error(), FsError::Disk_OutOfBounds);

    unlink(path.c_str());
}

TEST(FileDisk, DataPersistsAfterReopen)
{
    auto path = make_temp_file();

    {
        FileDisk disk;
        disk.create(path, 16);

        std::array<uint8_t, 3> src = { 9, 8, 7 };
        static_vector<uint8_t> write_data(src.data(), src.size(), src.size());
        ASSERT_TRUE(disk.write(4, write_data).has_value());
    }

    {
        FileDisk disk;
        ASSERT_TRUE(disk.open(path).has_value());

        std::array<uint8_t, 3> dst;
        static_vector read_data(dst.data(), dst.size());

        ASSERT_TRUE(disk.read(4, dst.size(), read_data).has_value());
        EXPECT_EQ(dst[0], 9);
        EXPECT_EQ(dst[1], 8);
        EXPECT_EQ(dst[2], 7);
    }

    unlink(path.c_str());
}
