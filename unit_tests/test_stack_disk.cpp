#include <gtest/gtest.h>
#include "disk/stack_disk.hpp"

TEST(StackDisk, Compiles)
{
    StackDisk stack_disk;
    auto res = stack_disk.read(0,1);
    SUCCEED();
}

TEST(StackDisk, Reads)
{
    StackDisk stack_disk;
    auto res = stack_disk.read(0,1);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res.value().size(), 1);
}

TEST(StackDisk, Writes)
{
    StackDisk stack_disk;
    std::vector data = {std::byte{0}, std::byte{1}, std::byte{2}, std::byte{3}};
    auto res = stack_disk.write(0, data);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(data.size(), res.value());
}

TEST(StackDisk, OutOfBounds)
{
    StackDisk stack_disk;
    auto res_read = stack_disk.read(stack_disk.size() - 1, 3);
    EXPECT_FALSE(res_read.has_value());
    EXPECT_EQ(res_read.error(), DiskError::OutOfBounds);

    auto res_write = stack_disk.write(stack_disk.size() - 1, {std::byte{1}, std::byte{2}, std::byte{3}});
    EXPECT_FALSE(res_write.has_value());
    EXPECT_EQ(res_write.error(), DiskError::OutOfBounds);
}

TEST(StackDisk, ReadsAndWrites)
{
    StackDisk stack_disk;
    std::vector data = {std::byte{0}, std::byte{1}, std::byte{2}, std::byte{3}};
    std::vector short_data = {std::byte{99}, std::byte{100}};

    // we need some space for the test
    ASSERT_GT(stack_disk.size(), 4);

    auto res = stack_disk.write(0, data);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(data.size(), res.value());

    auto read_res = stack_disk.read(0, data.size());
    ASSERT_TRUE(read_res.has_value());
    for (auto i = 0; i < read_res.value().size(); i++) {
        EXPECT_EQ(data[i], read_res.value()[i]);
    }

    auto write_res = stack_disk.write(1, short_data);
    ASSERT_TRUE(write_res.has_value());
    EXPECT_EQ(write_res.value(), short_data.size());

    auto second_read_res = stack_disk.read(0, data.size());
    ASSERT_TRUE(second_read_res.has_value());

    EXPECT_EQ(std::byte{0}, second_read_res.value()[0]);
    EXPECT_EQ(std::byte{99}, second_read_res.value()[1]);
    EXPECT_EQ(std::byte{100}, second_read_res.value()[2]);
    EXPECT_EQ(std::byte{3}, second_read_res.value()[3]);
}