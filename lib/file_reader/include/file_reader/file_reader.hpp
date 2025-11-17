#include "blockdevice/iblock_device.hpp"
#include "inode_manager/inode.hpp"

class FileReader {
    IBlockDevice& _block_device;

public:
    /**
     * Reads file with given inode
     */
    std::expected<std::vector<std::byte>, DiskError> ReadFile(Inode inode, size_t offset);

    /**
     * Writes file with given inode. Does not resize file.
     */
    std::expected<void, DiskError> WriteFile(
        Inode inode, size_t offset, std::vector<std::byte> bytes_to_write);
};