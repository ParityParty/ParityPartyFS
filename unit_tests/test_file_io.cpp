#include "ppfs/block_manager/block_manager.hpp"
#include "ppfs/blockdevice/raw_block_device.hpp"
#include "ppfs/common/static_vector.hpp"
#include "ppfs/disk/stack_disk.hpp"
#include "ppfs/file_io/file_io.hpp"
#include "ppfs/inode_manager/inode_manager.hpp"
#include <array>
#include <gtest/gtest.h>
#include <vector>

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
    BlockManager block_manager(SuperBlock {}, block_device);
    InodeManager inode_manager(block_device, *(new SuperBlock()));
    FileIO file_io(block_device, block_manager, inode_manager);

    EXPECT_TRUE(true);
}
TEST(FileIO, WritesAndReadsDirectBlocks)
{
    StackDisk disk;
    RawBlockDevice block_device(128, disk);
    SuperBlock superblock {
        .total_inodes = 10,
        .block_bitmap_address = 16,
        .inode_bitmap_address = 0,
        .inode_table_address = 1,
        .first_data_blocks_address = 18,
        .last_data_block_address = 1024,
        .block_size = 128,
    };
    BlockManager block_manager(superblock, block_device);
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

    size_t data_size = block_device.dataSize() * 12;
    std::array<uint8_t, 128 * 12> data_buffer;
    static_vector<uint8_t> data(data_buffer.data(), data_buffer.size(), data_size);

    for (size_t i = 0; i < data.size(); ++i)
        data[i] = uint8_t(i % 251);

    auto write_res = file_io.writeFile(inode_index.value(), inode, 0, data);
    ASSERT_TRUE(write_res.has_value()) << "writeFile failed: " << toString(write_res.error());

    ASSERT_NE(inode.direct_blocks[0], 0);
    ASSERT_NE(inode.direct_blocks[1], 0);
    ASSERT_NE(inode.direct_blocks[1], inode.direct_blocks[0]);

    std::array<uint8_t, 128 * 12> read_buffer;
    static_vector<uint8_t> read_data(read_buffer.data(), read_buffer.size());
    auto read_res = file_io.readFile(inode_index.value(), inode, 0, data_size, read_data);
    ASSERT_TRUE(read_res.has_value()) << "readFile failed";

    ASSERT_EQ(read_data.size(), data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        ASSERT_EQ(read_data[i], data[i]) << "Mismatch at index " << i;
    }
}

TEST(FileIO, WritesAndReadsUndirectBlocks)
{
    StackDisk disk;
    RawBlockDevice block_device(128, disk);
    SuperBlock superblock {
        .total_inodes = 10,
        .block_bitmap_address = 16,
        .inode_bitmap_address = 0,
        .inode_table_address = 1,
        .first_data_blocks_address = 18,
        .last_data_block_address = 1024,
        .block_size = 128,
    };
    BlockManager block_manager(superblock, block_device);
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

    size_t data_size = block_device.dataSize() * 12
        + block_device.dataSize() * (block_device.dataSize() / sizeof(block_index_t));
    std::array<uint8_t, 128 * 12 + 128 * 32>
        data_buffer; // 128 * 32 = 4096, enough for indexes_per_block
    static_vector<uint8_t> data(data_buffer.data(), data_buffer.size(), data_size);

    for (size_t i = 0; i < data.size(); ++i)
        data[i] = uint8_t(i % 253);

    auto write_res = file_io.writeFile(inode_index.value(), inode, 0, data);
    ASSERT_TRUE(write_res.has_value()) << "writeFile failed: " << toString(write_res.error());

    ASSERT_NE(inode.direct_blocks[0], 0);
    ASSERT_NE(inode.direct_blocks[1], 0);
    ASSERT_NE(inode.direct_blocks[1], inode.direct_blocks[0]);

    std::array<uint8_t, 128 * 12 + 128 * 32> read_buffer;
    static_vector<uint8_t> read_data(read_buffer.data(), read_buffer.size());
    auto read_res = file_io.readFile(inode_index.value(), inode, 0, data_size, read_data);
    ASSERT_TRUE(read_res.has_value()) << "readFile failed";

    ASSERT_EQ(read_data.size(), data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        ASSERT_EQ(read_data[i], data[i]) << "Mismatch at index " << i;
    }
}

TEST(FileIO, ReadAndWriteWithOffset)
{
    StackDisk disk;
    RawBlockDevice block_device(128, disk);
    SuperBlock superblock {
        .total_inodes = 10,
        .block_bitmap_address = 16,
        .inode_bitmap_address = 0,
        .inode_table_address = 1,
        .first_data_blocks_address = 18,
        .last_data_block_address = 1024,
        .block_size = 128,
    };
    BlockManager block_manager(superblock, block_device);
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
    std::array<uint8_t, 524> data1_buffer;
    std::fill(data1_buffer.begin(), data1_buffer.end(), 200);
    static_vector<uint8_t> data1(data1_buffer.data(), data1_buffer.size(), size1);

    std::array<uint8_t, 873> data2_buffer;
    std::fill(data2_buffer.begin(), data2_buffer.end(), 153);
    static_vector<uint8_t> data2(data2_buffer.data(), data2_buffer.size(), size2);

    ASSERT_TRUE(file_io.writeFile(inode_index.value(), inode, 0, data1));
    ASSERT_TRUE(file_io.writeFile(inode_index.value(), inode, size1, data2));
    ASSERT_EQ(inode.file_size, size1 + size2);

    std::array<uint8_t, 873> read_buffer1;
    static_vector<uint8_t> read_data1(read_buffer1.data(), read_buffer1.size());
    auto read_res = file_io.readFile(inode_index.value(), inode, size1, size2, read_data1);
    ASSERT_TRUE(read_res.has_value());
    for (size_t i = 0; i < read_data1.size(); i++) {
        if (read_data1[i] != 153)
            std::cout << i << ": " << read_data1[i] << std::endl;
    }
    ASSERT_EQ(read_data1.size(), data2.size());
    for (size_t i = 0; i < data2.size(); ++i) {
        ASSERT_EQ(read_data1[i], data2[i]) << "Mismatch at index " << i;
    }

    std::array<uint8_t, 1397> read_buffer2;
    static_vector<uint8_t> read_data2(read_buffer2.data(), read_buffer2.size());
    read_res = file_io.readFile(inode_index.value(), inode, 0, size1 + size2, read_data2);

    ASSERT_TRUE(read_res.has_value());
    ASSERT_EQ(read_data2.size(), size1 + size2);
    for (size_t i = 0; i < size1; ++i) {
        ASSERT_EQ(read_data2[i], 200) << "Mismatch at index " << i;
    }
    for (size_t i = 0; i < size2; ++i) {
        ASSERT_EQ(read_data2[size1 + i], 153) << "Mismatch at index " << (size1 + i);
    }
}

TEST(FileIO, WritesAndReadsDoublyUndirectBlocks)
{
    StackDisk disk;
    RawBlockDevice block_device(128, disk);
    SuperBlock superblock {
        .total_inodes = 10,
        .block_bitmap_address = 16,
        .inode_bitmap_address = 0,
        .inode_table_address = 1,
        .first_data_blocks_address = 30,
        .last_data_block_address = 2024,
        .block_size = 128,
    };
    BlockManager block_manager(superblock, block_device);
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
    size_t data_size = block_device.dataSize()
        * (12 + indexes_per_block + indexes_per_block * indexes_per_block);
    std::array<uint8_t, 128 * (12 + 32 + 32 * 32)> data_buffer; // Large enough buffer
    static_vector<uint8_t> data(data_buffer.data(), data_buffer.size(), data_size);

    for (size_t i = 0; i < data.size(); ++i)
        data[i] = uint8_t(i % 227);

    auto write_res = file_io.writeFile(inode_index.value(), inode, 0, data);
    ASSERT_TRUE(write_res.has_value()) << "writeFile failed: " << toString(write_res.error());

    ASSERT_NE(inode.direct_blocks[0], 0);
    ASSERT_NE(inode.direct_blocks[1], 0);
    ASSERT_NE(inode.direct_blocks[1], inode.direct_blocks[0]);

    std::array<uint8_t, 128 * (12 + 32 + 32 * 32)> read_buffer;
    static_vector<uint8_t> read_data(read_buffer.data(), read_buffer.size());
    auto read_res = file_io.readFile(inode_index.value(), inode, 0, data_size, read_data);
    ASSERT_TRUE(read_res.has_value()) << "readFile failed";

    ASSERT_EQ(read_data.size(), data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        ASSERT_EQ(read_data[i], data[i]) << "Mismatch at index " << i;
    }
}

TEST(FileIO, WritesAndReadsTreblyUndirectBlocks)
{
    StackDisk disk;
    RawBlockDevice block_device(32, disk);
    SuperBlock superblock {
        .total_inodes = 10,
        .block_bitmap_address = 16,
        .inode_bitmap_address = 0,
        .inode_table_address = 1,
        .first_data_blocks_address = 100,
        .last_data_block_address = 40000,
        .block_size = 128,
    };
    BlockManager block_manager(superblock, block_device);
    FakeInodeManager inode_manager;
    FileIO file_io(block_device, block_manager, inode_manager);

    Inode inode {};
    inode.file_size = 0;

    auto indexes_per_block = block_device.dataSize() / sizeof(block_index_t);
    size_t data_size = block_device.dataSize()
        * (12 + indexes_per_block + indexes_per_block * indexes_per_block
            + indexes_per_block * indexes_per_block * indexes_per_block);
    std::array<uint8_t, 32 * (12 + 8 + 8 * 8 + 8 * 8 * 8)> data_buffer; // Large enough buffer
    static_vector<uint8_t> data(data_buffer.data(), data_buffer.size(), data_size);

    for (size_t i = 0; i < data.size(); ++i)
        data[i] = uint8_t(i / block_device.dataSize());

    auto write_res = file_io.writeFile(0, inode, 0, data);
    ASSERT_TRUE(write_res.has_value()) << "writeFile failed: " << toString(write_res.error());

    ASSERT_NE(inode.direct_blocks[0], 0);
    ASSERT_NE(inode.direct_blocks[1], 0);
    ASSERT_NE(inode.direct_blocks[1], inode.direct_blocks[0]);

    std::array<uint8_t, 32 * (12 + 8 + 8 * 8 + 8 * 8 * 8)> read_buffer;
    static_vector<uint8_t> read_data(read_buffer.data(), read_buffer.size());
    auto read_res = file_io.readFile(0, inode, 0, data_size, read_data);
    ASSERT_TRUE(read_res.has_value()) << "readFile failed";

    ASSERT_EQ(read_data.size(), data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        ASSERT_EQ(read_data[i], data[i]) << "Mismatch at index " << i;
    }
}

TEST(FileIO, ResizeAndReadFile)
{
    StackDisk disk;
    RawBlockDevice block_device(128, disk);

    SuperBlock superblock {
        .total_inodes = 10,
        .block_bitmap_address = 16,
        .inode_bitmap_address = 0,
        .inode_table_address = 1,
        .first_data_blocks_address = 18,
        .last_data_block_address = 2048,
        .block_size = 128,
    };
    BlockManager block_manager(superblock, block_device);
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

    size_t read_size = num_of_blocks * block_device.dataSize();
    std::array<uint8_t, 128 * (12 + 32 + 32 * 32)> read_buffer;
    static_vector<uint8_t> read_data(read_buffer.data(), read_buffer.size());
    auto read_res = file_io.readFile(inode_index.value(), inode, 0, read_size, read_data);
    ASSERT_TRUE(read_res.has_value());
    ASSERT_EQ(read_data.size(), read_size);
}

TEST(FileIO, TruncatePartOfBlock)
{
    StackDisk disk;
    RawBlockDevice block_device(128, disk);
    SuperBlock superblock {
        .total_inodes = 10,
        .block_bitmap_address = 16,
        .inode_bitmap_address = 0,
        .inode_table_address = 1,
        .first_data_blocks_address = 18,
        .last_data_block_address = 2048,
        .block_size = 128,
    };
    BlockManager block_manager(superblock, block_device);
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

    std::array<uint8_t, 128 * (12 + 32 + 32 * 32)> data_buffer;
    static_vector<uint8_t> data(data_buffer.data(), data_buffer.size(), file_size);
    for (size_t i = 0; i < data.size(); i++)
        data[i] = uint8_t(i % 251);

    ASSERT_TRUE(file_io.writeFile(inode_index.value(), inode, 0, data).has_value());

    auto resize_res = file_io.resizeFile(inode_index.value(), inode, file_size - to_truncate);
    ASSERT_TRUE(resize_res.has_value()) << "resizeFile failed: " << toString(resize_res.error());

    ASSERT_EQ(inode.file_size, file_size - to_truncate);

    size_t read_size = file_size - to_truncate;
    std::array<uint8_t, 128 * (12 + 32 + 32 * 32)> read_buffer;
    static_vector<uint8_t> read_data(read_buffer.data(), read_buffer.size());
    auto read_res = file_io.readFile(inode_index.value(), inode, 0, read_size, read_data);
    ASSERT_TRUE(read_res.has_value());
    ASSERT_EQ(read_data.size(), read_size);
    for (size_t i = 0; i < read_size; ++i) {
        ASSERT_EQ(read_data[i], data[i]) << "Mismatch at index " << i;
    }
}

TEST(FileIO, ResizeHugeFileToZero)
{
    StackDisk disk;
    RawBlockDevice block_device(32, disk);
    SuperBlock superblock {
        .total_inodes = 10,
        .block_bitmap_address = 16,
        .first_data_blocks_address = 18,
        .last_data_block_address = 1024,

    };
    BlockManager block_manager(superblock, block_device);
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

    std::array<uint8_t, 32 * (12 + 8 + 8 * 8 + 8 * 8 * 8)> data_buffer;
    static_vector<uint8_t> data(data_buffer.data(), data_buffer.size(), file_size);
    for (size_t i = 0; i < data.size(); i++)
        data[i] = uint8_t(i % 251);

    auto write_res = file_io.writeFile(0, inode, 0, data);
    ASSERT_TRUE(write_res.has_value()) << "write failed" << toString(write_res.error());

    auto resize_res = file_io.resizeFile(0, inode, 0);
    ASSERT_TRUE(resize_res.has_value()) << "resizeFile failed";
    ASSERT_EQ(inode.file_size, 0);

    num_free_res = block_manager.numFree();
    ASSERT_TRUE(num_free_res.has_value()) << "numFree failed";
    ASSERT_EQ(block_manager.numFree().value(), free_blocks);
}
