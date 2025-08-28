#include <gtest/gtest.h>
#include "ppfs/data_structures.hpp"
#include "ppfs/ppfs.hpp"
#include "disk/stack_disk.hpp"

TEST(SuperBlock, serialization){
    SuperBlock sb1 = SuperBlock(1,12345,99999999,1024);
    auto bytes = sb1.toBytes();
    auto sb2 = SuperBlock::fromBytes(bytes);

    EXPECT_EQ(sb1.num_blocks, sb2.num_blocks);
    EXPECT_EQ(sb1.block_size, sb2.block_size);
    EXPECT_EQ(sb1.fat_block_start, sb2.fat_block_start);
    EXPECT_EQ(sb1.fat_size, sb2.fat_size);
}

TEST(FAT, serialization)
{
    std::vector fat = {-1, -1, -1, 0,0,0,7,-1, 9, 10, -1, 0};
    auto dirty_entries = std::vector(fat.size(), false);
    const auto fat1 = FileAllocationTable(std::move(fat), std::move(dirty_entries));
    const auto bytes = fat1.toBytes();
    const auto fat2 = FileAllocationTable::fromBytes(bytes);

    EXPECT_EQ(fat1, fat2);
}

TEST(FAT, UpdateFat)
{
    StackDisk disk;
    std::vector fat = {-1, -1, -1, 0,0,0,7,-1, 9, 10, -1, 0};
    auto dirty_entries = std::vector(fat.size(), false);
    auto fat1 = FileAllocationTable(std::move(fat), std::move(dirty_entries));

    const auto bytes = fat1.toBytes();
    const auto ret = disk.write(0, bytes);
    ASSERT_TRUE(ret.has_value()) << "Failed to write to disk";

    fat1.setValue(0,22);
    fat1.setValue(1,2222);
    fat1.setValue(2,222222);

    const auto ret_update = fat1.updateFat(disk, 0);
    ASSERT_TRUE(ret_update.has_value()) << "Failed to update fat";

    const auto ret_after_update = disk.read(0, bytes.size());
    ASSERT_TRUE(ret_after_update.has_value()) << "Error reading from disk";

    const auto fat2 = FileAllocationTable::fromBytes(ret_after_update.value());

    EXPECT_EQ(fat1, fat2);
}
