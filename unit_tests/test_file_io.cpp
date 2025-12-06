#include "block_manager/block_manager.hpp"
#include "blockdevice/raw_block_device.hpp"
#include "disk/stack_disk.hpp"
#include "file_io/file_io.hpp"
#include "inode_manager/inode_manager.hpp"
#include <gtest/gtest.h>

struct FakeInodeManager : public IInodeManager {
    std::expected<inode_index_t, FsError> create(Inode& inode) override { return 0; }
    std::expected<void, FsError> remove(inode_index_t inode) override { return {}; }
    std::expected<Inode, FsError> get(inode_index_t inode) override { return Inode(); }
    std::expected<unsigned int, FsError> numFree() { return 1; }
    std::expected<void, FsError> update(inode_index_t inode_index, const Inode& inode)
    {
        return {};
    }
    std::expected<void, FsError> format() { return {}; };
};

TEST(FileIO, Compiles)
{
    StackDisk disk;
    RawBlockDevice block_device(32, disk);
    BlockManager block_manager(0, 512, block_device);
    InodeManager inode_manager(block_device, *(new SuperBlock()));
    FileIO file_io(block_device, block_manager, inode_manager);

    EXPECT_TRUE(true);
}
TEST(FileIO, WritesAndReadsDirectBlocks)
{
    StackDisk disk;
    RawBlockDevice block_device(128, disk);
    BlockManager block_manager(16, 1024, block_device);
    SuperBlock superblock {
        .total_inodes = 1, .inode_bitmap_address = 0, .inode_table_address = 1, .block_size = 128
    };
    InodeManager inode_manager(block_device, superblock);
    FileIO file_io(block_device, block_manager, inode_manager);

    Inode inode {};
    inode.file_size = 0;
    auto format_res = inode_manager.format();
    ASSERT_TRUE(format_res.has_value())
        << "Failed to format inode table: " << toString(format_res.error());
    auto inode_index = inode_manager.create(inode);
    ASSERT_TRUE(inode_index.has_value())
        << "Failed to create inode: " << toString(inode_index.error());

    std::vector<uint8_t> data(block_device.dataSize() * 12);

    for (size_t i = 0; i < data.size(); ++i)
        data[i] = uint8_t(i % 251);

    auto write_res = file_io.writeFile(inode_index.value(), inode, 0, data);
    ASSERT_TRUE(write_res.has_value()) << "writeFile failed: " << toString(write_res.error());

    ASSERT_NE(inode.direct_blocks[0], 0);
    ASSERT_NE(inode.direct_blocks[1], 0);
    ASSERT_NE(inode.direct_blocks[1], inode.direct_blocks[0]);

    auto read_res = file_io.readFile(inode_index.value(), inode, 0, data.size());
    ASSERT_TRUE(read_res.has_value()) << "readFile failed";

    auto out = read_res.value();

    ASSERT_EQ(out, data);
}

TEST(FileIO, WritesAndReadsUndirectBlocks)
{
    StackDisk disk;
    RawBlockDevice block_device(128, disk);
    BlockManager block_manager(16, 1024, block_device);
    SuperBlock superblock {
        .total_inodes = 1, .inode_bitmap_address = 0, .inode_table_address = 1, .block_size = 128
    };
    InodeManager inode_manager(block_device, superblock);
    FileIO file_io(block_device, block_manager, inode_manager);

    Inode inode {};
    inode.file_size = 0;
    auto format_res = inode_manager.format();
    ASSERT_TRUE(format_res.has_value())
        << "Failed to format inode table: " << toString(format_res.error());
    auto inode_index = inode_manager.create(inode);
    ASSERT_TRUE(inode_index.has_value())
        << "Failed to create inode: " << toString(inode_index.error());

    std::vector<uint8_t> data(block_device.dataSize() * 12
        + block_device.dataSize() * (block_device.dataSize() / sizeof(block_index_t)));

    for (size_t i = 0; i < data.size(); ++i)
        data[i] = uint8_t(i % 253);

    auto write_res = file_io.writeFile(inode_index.value(), inode, 0, data);
    ASSERT_TRUE(write_res.has_value()) << "writeFile failed: " << toString(write_res.error());

    ASSERT_NE(inode.direct_blocks[0], 0);
    ASSERT_NE(inode.direct_blocks[1], 0);
    ASSERT_NE(inode.direct_blocks[1], inode.direct_blocks[0]);

    auto read_res = file_io.readFile(inode_index.value(), inode, 0, data.size());
    ASSERT_TRUE(read_res.has_value()) << "readFile failed";

    auto out = read_res.value();

    ASSERT_EQ(out.size(), data.size());
    ASSERT_EQ(out, data);
}

TEST(FileIO, ReadAndWriteWithOffset)
{
    StackDisk disk;
    RawBlockDevice block_device(128, disk);
    BlockManager block_manager(16, 1024, block_device);
    SuperBlock superblock {
        .total_inodes = 1, .inode_bitmap_address = 0, .inode_table_address = 1, .block_size = 128
    };
    InodeManager inode_manager(block_device, superblock);
    FileIO file_io(block_device, block_manager, inode_manager);

    Inode inode {};
    inode.file_size = 0;
    auto format_res = inode_manager.format();
    ASSERT_TRUE(format_res.has_value())
        << "Failed to format inode table: " << toString(format_res.error());
    auto inode_index = inode_manager.create(inode);
    ASSERT_TRUE(inode_index.has_value())
        << "Failed to create inode: " << toString(inode_index.error());

    size_t size1 = 524;
    size_t size2 = 873;
    std::vector<uint8_t> data1(size1, 200);
    std::vector<uint8_t> data2(size2, 153);
    ASSERT_TRUE(file_io.writeFile(inode_index.value(), inode, 0, data1));
    ASSERT_TRUE(file_io.writeFile(inode_index.value(), inode, size1, data2));
    ASSERT_EQ(inode.file_size, size1 + size2);

    auto read_res = file_io.readFile(inode_index.value(), inode, size1, size2);
    ASSERT_TRUE(read_res.has_value());
    for (int i = 0; i < read_res.value().size(); i++) {
        if (read_res.value()[i] != 153)
            std::cout << i << ": " << read_res.value()[i] << std::endl;
    }
    ASSERT_EQ(read_res.value(), data2);

    read_res = file_io.readFile(inode_index.value(), inode, 0, size1 + size2);

    std::vector<uint8_t> concatenated;
    concatenated.reserve(size1 + size2);
    concatenated.insert(concatenated.end(), data1.begin(), data1.end());
    concatenated.insert(concatenated.end(), data2.begin(), data2.end());
    ASSERT_TRUE(read_res.has_value());
    ASSERT_EQ(read_res.value(), concatenated);
}

TEST(FileIO, WritesAndReadsDoublyUndirectBlocks)
{
    StackDisk disk;
    RawBlockDevice block_device(128, disk);
    BlockManager block_manager(16, 2048, block_device);
    SuperBlock superblock {
        .total_inodes = 1, .inode_bitmap_address = 0, .inode_table_address = 1, .block_size = 128
    };
    InodeManager inode_manager(block_device, superblock);
    FileIO file_io(block_device, block_manager, inode_manager);

    Inode inode {};
    inode.file_size = 0;
    auto format_res = inode_manager.format();
    ASSERT_TRUE(format_res.has_value())
        << "Failed to format inode table: " << toString(format_res.error());
    auto inode_index = inode_manager.create(inode);
    ASSERT_TRUE(inode_index.has_value())
        << "Failed to create inode: " << toString(inode_index.error());

    auto indexes_per_block = block_device.dataSize() / sizeof(block_index_t);
    std::vector<uint8_t> data(
        block_device.dataSize() * (12 + indexes_per_block + indexes_per_block * indexes_per_block));

    for (size_t i = 0; i < data.size(); ++i)
        data[i] = uint8_t(i % 227);

    auto write_res = file_io.writeFile(inode_index.value(), inode, 0, data);
    ASSERT_TRUE(write_res.has_value()) << "writeFile failed: " << toString(write_res.error());

    ASSERT_NE(inode.direct_blocks[0], 0);
    ASSERT_NE(inode.direct_blocks[1], 0);
    ASSERT_NE(inode.direct_blocks[1], inode.direct_blocks[0]);

    auto read_res = file_io.readFile(inode_index.value(), inode, 0, data.size());
    ASSERT_TRUE(read_res.has_value()) << "readFile failed";

    auto out = read_res.value();

    ASSERT_EQ(out.size(), data.size());
    ASSERT_EQ(out, data);
}

TEST(FileIO, WritesAndReadsTreblyUndirectBlocks)
{
    StackDisk disk;
    RawBlockDevice block_device(32, disk);
    BlockManager block_manager(16, 40000, block_device);
    SuperBlock superblock {
        .total_inodes = 1, .inode_bitmap_address = 0, .inode_table_address = 1, .block_size = 128
    };
    FakeInodeManager inode_manager;
    FileIO file_io(block_device, block_manager, inode_manager);

    Inode inode {};
    inode.file_size = 0;

    auto indexes_per_block = block_device.dataSize() / sizeof(block_index_t);
    std::vector<uint8_t> data(block_device.dataSize()
        * (12 + indexes_per_block + indexes_per_block * indexes_per_block
            + indexes_per_block * indexes_per_block * indexes_per_block));
    std::cout << data.size() << std::endl;

    for (size_t i = 0; i < data.size(); ++i)
        data[i] = uint8_t(i % 231);

    auto write_res = file_io.writeFile(0, inode, 0, data);
    ASSERT_TRUE(write_res.has_value()) << "writeFile failed: " << toString(write_res.error());

    ASSERT_NE(inode.direct_blocks[0], 0);
    ASSERT_NE(inode.direct_blocks[1], 0);
    ASSERT_NE(inode.direct_blocks[1], inode.direct_blocks[0]);

    auto read_res = file_io.readFile(0, inode, 0, data.size());
    ASSERT_TRUE(read_res.has_value()) << "readFile failed";

    auto out = read_res.value();

    ASSERT_EQ(out.size(), data.size());
    ASSERT_EQ(out, data);
}

TEST(FileIO, ResizeAndReadFile)
{
    StackDisk disk;
    RawBlockDevice block_device(128, disk);
    BlockManager block_manager(16, 2048, block_device);
    SuperBlock superblock {
        .total_inodes = 1, .inode_bitmap_address = 0, .inode_table_address = 1, .block_size = 128
    };
    InodeManager inode_manager(block_device, superblock);
    FileIO file_io(block_device, block_manager, inode_manager);

    Inode inode {};
    inode.file_size = 0;
    auto format_res = inode_manager.format();
    ASSERT_TRUE(format_res.has_value())
        << "Failed to format inode table: " << toString(format_res.error());
    auto inode_index = inode_manager.create(inode);
    ASSERT_TRUE(inode_index.has_value())
        << "Failed to create inode: " << toString(inode_index.error());

    size_t indexes_per_block = block_device.dataSize() / sizeof(block_index_t);
    size_t num_of_blocks = 12 + indexes_per_block + indexes_per_block * indexes_per_block;

    auto resize_res
        = file_io.resizeFile(inode_index.value(), inode, num_of_blocks * block_device.dataSize());
    ASSERT_TRUE(resize_res.has_value()) << "resizeFile failed: " << toString(resize_res.error());
    ASSERT_EQ(inode.file_size, num_of_blocks * block_device.dataSize());

    ASSERT_NE(inode.direct_blocks[0], 0);
    ASSERT_NE(inode.direct_blocks[1], 0);
    ASSERT_NE(inode.direct_blocks[2], 0);

    auto read_res
        = file_io.readFile(inode_index.value(), inode, 0, num_of_blocks * block_device.dataSize());
    ASSERT_TRUE(read_res.has_value());
    ASSERT_EQ(read_res.value().size(), num_of_blocks * block_device.dataSize());
}

TEST(FileIO, TruncatePartOfBlock)
{
    StackDisk disk;
    RawBlockDevice block_device(128, disk);
    BlockManager block_manager(16, 2048, block_device);
    SuperBlock superblock {
        .total_inodes = 1, .inode_bitmap_address = 0, .inode_table_address = 1, .block_size = 128
    };
    InodeManager inode_manager(block_device, superblock);
    FileIO file_io(block_device, block_manager, inode_manager);

    Inode inode {};
    inode.file_size = 0;
    auto format_res = inode_manager.format();
    ASSERT_TRUE(format_res.has_value())
        << "Failed to format inode table: " << toString(format_res.error());
    auto inode_index = inode_manager.create(inode);
    ASSERT_TRUE(inode_index.has_value())
        << "Failed to create inode: " << toString(inode_index.error());

    size_t indexes_per_block = block_device.dataSize() / sizeof(block_index_t);
    size_t num_of_blocks = 12 + indexes_per_block + indexes_per_block * indexes_per_block;
    size_t to_truncate = block_device.dataSize() / 2;
    size_t file_size = num_of_blocks * block_device.dataSize();

    std::vector<uint8_t> data(file_size);
    for (size_t i = 0; i < data.size(); i++)
        data[i] = uint8_t(i % 251);

    ASSERT_TRUE(file_io.writeFile(inode_index.value(), inode, 0, data).has_value());

    auto resize_res = file_io.resizeFile(inode_index.value(), inode, file_size - to_truncate);
    ASSERT_TRUE(resize_res.has_value()) << "resizeFile failed: " << toString(resize_res.error());

    ASSERT_EQ(inode.file_size, file_size - to_truncate);

    auto read_res = file_io.readFile(inode_index.value(), inode, 0, file_size - to_truncate);
    ASSERT_TRUE(read_res.has_value());
    std::vector<uint8_t> expected_data(data.begin(), data.end() - to_truncate);
    ASSERT_EQ(read_res.value(), expected_data);
}

TEST(FileIO, ResizeHugeFileToZero)
{
    StackDisk disk;
    RawBlockDevice block_device(32, disk);
    BlockManager block_manager(16, 1024, block_device);
    FakeInodeManager inode_manager;
    FileIO file_io(block_device, block_manager, inode_manager);

    Inode inode {};
    inode.file_size = 0;

    auto num_free_res = block_manager.numFree();
    ASSERT_TRUE(num_free_res.has_value()) << "numFree failed";
    auto free_blocks = num_free_res.value();

    size_t indexes_per_block = block_device.dataSize() / sizeof(block_index_t);
    size_t num_of_blocks = 12 + indexes_per_block + indexes_per_block * indexes_per_block
        + indexes_per_block * indexes_per_block * indexes_per_block;

    size_t file_size = num_of_blocks * block_device.dataSize();

    std::vector<uint8_t> data(file_size);
    for (size_t i = 0; i < data.size(); i++)
        data[i] = uint8_t(i % 251);

    auto write_res = file_io.writeFile(0, inode, 0, data);
    ASSERT_TRUE(write_res.has_value()) << "write failed";

    auto resize_res = file_io.resizeFile(0, inode, 0);
    ASSERT_TRUE(resize_res.has_value()) << "resizeFile failed";
    ASSERT_EQ(inode.file_size, 0);

    num_free_res = block_manager.numFree();
    ASSERT_TRUE(num_free_res.has_value()) << "numFree failed";
    ASSERT_EQ(block_manager.numFree().value(), free_blocks);
}