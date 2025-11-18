#include "blockdevice/iblock_device.hpp"
#include "inode_manager/inode.hpp"
#include <optional>

class FileIO {
    IBlockDevice& _block_device;

    std::expected<std::vector<block_index_t>, FsError> readIndirectBlock(block_index_t block_index);

public:
    FileIO(IBlockDevice& block_device);
    /**
     * Reads file with given inode
     */
    std::expected<std::vector<std::byte>, FsError> readFile(
        Inode inode, size_t offset, size_t bytes_to_read);

    /**
     * Writes file with given inode. Does not resize file.
     */
    std::expected<void, FsError> writeFile(
        Inode inode, size_t offset, std::vector<std::byte> bytes_to_write);
};

class BlockIndexIterator {
public:
    BlockIndexIterator(block_index_t index, Inode inode, IBlockDevice& block_device);

    std::optional<block_index_t> next();

private:
    block_index_t _index;
    Inode _inode;
    IBlockDevice& _block_device;
    std::optional<std::vector<block_index_t>> _index_block_1;
    std::optional<std::vector<block_index_t>> _index_block_2;
    std::optional<std::vector<block_index_t>> _index_block_3;
    bool finished = false;

    std::expected<std::vector<block_index_t>, FsError> _readIndexBlock(block_index_t index);
};