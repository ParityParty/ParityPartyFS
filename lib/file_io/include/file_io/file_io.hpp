#pragma once
#include "block_manager/iblock_manager.hpp"
#include "blockdevice/iblock_device.hpp"
#include "common/static_vector.hpp"
#include "inode_manager/iinode_manager.hpp"
#include <array>
#include <cstddef>
#include <optional>

class FileIO {
    IBlockDevice& _block_device;
    IBlockManager& _block_manager;
    IInodeManager& _inode_manager;

public:
    FileIO(IBlockDevice& block_device, IBlockManager& block_manager, IInodeManager& inode_manager);
    /**
     * Reads file with given inode. If read exceeds file size, returns FsError::OutOfBounds.
     */
    std::expected<void, FsError> readFile(inode_index_t inode_index, Inode& inode, size_t offset,
        size_t bytes_to_read, static_vector<uint8_t>& buf);

    /**
     * Writes file with given inode. Resizes file if necessary and updates inode table.
     * If writing fails after some blocks were written, those blocks are not freed again.
     * Inode is updated with inode manager to reflect new file size and inode blocks.
     */

    std::expected<size_t, FsError> writeFile(inode_index_t inode_index, Inode& inode, size_t offset,
        const static_vector<uint8_t>& bytes_to_write);

    /**
     * Resizes file to a given size
     */
    [[nodiscard]] std::expected<void, FsError> resizeFile(
        inode_index_t inode_index, Inode& inode, size_t new_size);
};

class BlockIndexIterator {
public:
    BlockIndexIterator(size_t index, Inode& inode, IBlockDevice& block_device,
        IBlockManager& block_manager, bool should_resize);

    /**
     * Returns the next data block index of the file.
     *
     * If should_resize is set to true, updates inode and find new index block if necessary.
     * Does not update inode in inode maneger!!! File size is not updated either!!!
     */
    [[nodiscard]] std::expected<block_index_t, FsError> next();

    /**
     * Returns the next data block index of the file. If this data block is the
     * first one coming from a given indirect block, also returns the indices of
     * the indirect blocks that were newly added.
     *
     * If should_resize is set to true, updates inode and find new index block if necessary.
     * Does not update inode in inode maneger!!! File size is not updated either!!!
     */

    [[nodiscard]] std::expected<block_index_t, FsError>
    nextWithIndirectBlocksAdded(static_vector<block_index_t>& undirected_blocks_addeed);

private:
    size_t _index;
    Inode& _inode;
    IBlockDevice& _block_device;
    IBlockManager& _block_manager;
    std::array<block_index_t, MAX_BLOCK_SIZE / sizeof(block_index_t)> _index_block_1_buffer;
    std::array<block_index_t, MAX_BLOCK_SIZE / sizeof(block_index_t)> _index_block_2_buffer;
    std::array<block_index_t, MAX_BLOCK_SIZE / sizeof(block_index_t)> _index_block_3_buffer;
    static_vector<block_index_t> _index_block_1;
    static_vector<block_index_t> _index_block_2;
    static_vector<block_index_t> _index_block_3;
    bool _finished = false;
    bool _should_resize;
    size_t _occupied_blocks;

    std::expected<void, FsError> _readIndexBlock(block_index_t index, static_vector<block_index_t>& buf);
    std::expected<void, FsError> _writeIndexBlock(
        block_index_t index, const static_vector<block_index_t>& indices);

    [[nodiscard]] std::expected<block_index_t, FsError> _findAndReserveBlock();
};