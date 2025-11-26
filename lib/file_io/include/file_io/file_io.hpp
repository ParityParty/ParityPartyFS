#include "block_manager/iblock_manager.hpp"
#include "blockdevice/iblock_device.hpp"
#include "inode_manager/iinode_manager.hpp"
#include <optional>

class FileIO {
    IBlockDevice& _block_device;
    IBlockManager& _block_manager;
    IInodeManager& _inode_manager;

    std::expected<void, FsError> _freeIndirectAndDirectBlocks(
        block_index_t first_index_to_free, block_index_t last_index_to_free, int levels);

public:
    FileIO(IBlockDevice& block_device, IBlockManager& block_manager, IInodeManager& inode_manager);
    /**
     * Reads file with given inode. If read exceeds file size, returns FsError::OutOfBounds.
     */
    std::expected<std::vector<uint8_t>, FsError> readFile(
        Inode& inode, size_t offset, size_t bytes_to_read);

    /**
     * Writes file with given inode. Resizes file if necessary and updates inode table.
     * If writing fails after some blocks were written, those blocks are not freed again.
     * Inode is updated with inode manager to reflect new file size and inode blocks.
     */
    std::expected<size_t, FsError> writeFile(
        Inode& inode, size_t offset, std::vector<uint8_t> bytes_to_write);

    std::expected<void, FsError> resizeFile(Inode& inode, size_t new_size);
};

class BlockIndexIterator {
public:
    BlockIndexIterator(size_t index, Inode& inode, IBlockDevice& block_device,
        IBlockManager& block_manager, bool should_resize);

    /**
     * If should_resize is set to true, updates inode and find new index block if necessary.
     * Does not update inode in inode maneger!!! File size is not updated either!!!
     */
    std::expected<block_index_t, FsError> next();

    std::expected<std::tuple<block_index_t, std::vector<block_index_t>>, FsError>
    nextWithIndirectBlocksAdded();

private:
    size_t _index;
    Inode& _inode;
    IBlockDevice& _block_device;
    IBlockManager& _block_manager;
    std::vector<block_index_t> _index_block_1;
    std::vector<block_index_t> _index_block_2;
    std::vector<block_index_t> _index_block_3;
    bool _finished = false;
    bool _should_resize;
    size_t _occupied_blocks;

    std::expected<std::vector<block_index_t>, FsError> _readIndexBlock(block_index_t index);
    std::expected<void, FsError> _writeIndexBlock(
        block_index_t index, const std::vector<block_index_t>& indices);

    std::expected<block_index_t, FsError> _findAndReserveBlock();
};