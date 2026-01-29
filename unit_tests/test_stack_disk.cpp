#include "array"
#include "ppfs/common/static_vector.hpp"
#include "ppfs/disk/stack_disk.hpp"
#include <gtest/gtest.h>

TEST(StackDisk, Compiles)
{
    StackDisk stack_disk;
    SUCCEED();
}

TEST(StackDisk, Reads)
{
    StackDisk stack_disk;

    std::array<uint8_t, 1> buffer;
    static_vector data(buffer.data(), 1);

    auto res = stack_disk.read(0, 1, data);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(data.size(), 1);
}

TEST(StackDisk, Writes)
{
    StackDisk stack_disk;
    std::array buffer
        = { std::uint8_t { 0 }, std::uint8_t { 1 }, std::uint8_t { 2 }, std::uint8_t { 3 } };
    static_vector<uint8_t> data(buffer.data(), buffer.size(), buffer.size());
    auto res = stack_disk.write(0, data);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(data.size(), res.value());
}

TEST(StackDisk, OutOfBounds)
{
    StackDisk stack_disk;
    std::array<uint8_t, 3> buffer1;
    static_vector data1(buffer1.data(), 3);

    auto res_read = stack_disk.read(stack_disk.size() - 1, 3, data1);
    EXPECT_FALSE(res_read.has_value());
    EXPECT_EQ(res_read.error(), FsError::Disk_OutOfBounds);

    std::array buffer2 = { std::uint8_t { 1 }, std::uint8_t { 2 }, std::uint8_t { 3 } };
    static_vector data2(buffer2.data(), 3, 3);

    auto res_write = stack_disk.write(stack_disk.size() - 1, data2);
    EXPECT_FALSE(res_write.has_value());
    EXPECT_EQ(res_write.error(), FsError::Disk_OutOfBounds) << toString(res_write.error());
}

TEST(StackDisk, ReadsAndWrites)
{
    StackDisk stack_disk;
    std::array<uint8_t, 4> buf1 = { 0, 1, 2, 3 };
    static_vector<uint8_t> data(buf1.data(), buf1.size(), buf1.size());

    std::array<uint8_t, 2> buf2 = { 99, 100 };
    static_vector<uint8_t> short_data(buf2.data(), buf2.size(), buf2.size());

    // we need some space for the test
    ASSERT_GT(stack_disk.size(), 4);

    auto write_res = stack_disk.write(0, data);
    ASSERT_TRUE(write_res.has_value());
    EXPECT_EQ(data.size(), write_res.value());

    std::array<uint8_t, 4> buf3;
    static_vector read_data(buf3.data(), buf3.size());
    auto read_res = stack_disk.read(0, data.size(), read_data);
    ASSERT_TRUE(read_res.has_value());
    ASSERT_EQ(read_data.size(), data.size());
    for (auto i = 0; i < data.size(); i++) {
        EXPECT_EQ(data[i], read_data[i]);
    }

    write_res = stack_disk.write(1, short_data);
    ASSERT_TRUE(write_res.has_value());
    EXPECT_EQ(write_res.value(), short_data.size());

    read_res = stack_disk.read(0, data.size(), read_data);
    ASSERT_TRUE(read_res.has_value());
    ASSERT_EQ(read_data.size(), data.size());

    EXPECT_EQ(std::uint8_t { 0 }, read_data[0]);
    EXPECT_EQ(std::uint8_t { 99 }, read_data[1]);
    EXPECT_EQ(std::uint8_t { 100 }, read_data[2]);
    EXPECT_EQ(std::uint8_t { 3 }, read_data[3]);
}