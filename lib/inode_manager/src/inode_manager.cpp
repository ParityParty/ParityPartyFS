#include "ppfs/inode_manager/inode_manager.hpp"
#include "ppfs/common/static_vector.hpp"

InodeManager::InodeManager(IBlockDevice& block_device, SuperBlock& superblock)
    : _block_device(block_device)
    , _superblock(superblock)
    , _bitmap(Bitmap(block_device, superblock.inode_bitmap_address, superblock.total_inodes))
{
}

DataLocation InodeManager::_getInodeLocation(inode_index_t inode)
{
    DataLocation result;
    result.block_index
        = _superblock.inode_table_address + inode * sizeof(Inode) / _block_device.dataSize();
    result.offset = inode * sizeof(Inode) % _block_device.dataSize();
    return result;
}

std::expected<void, FsError> InodeManager::_writeInode(inode_index_t index, const Inode& inode)
{
    // TODO: When we switch to static memory, optimise this
    auto data = (std::uint8_t*)(&inode);
    int bytes_written = 0;

    auto start = _getInodeLocation(index);
    if (_block_device.dataSize() - start.offset < sizeof(Inode)) {
        // Data won't fit in one block
        // We write the unaligned part here before writing the rest
        static_vector<uint8_t> data_vector(data, sizeof(Inode), sizeof(Inode));
        auto write_res = _block_device.writeBlock(data_vector, start);
        if (!write_res.has_value()) {
            return std::unexpected(write_res.error());
        }
        bytes_written += write_res.value();
        start.offset = 0;
        start.block_index++;
        data += write_res.value();
    }

    while (bytes_written < sizeof(Inode)) {
        int bytes_left = sizeof(Inode) - bytes_written;
        static_vector<std::uint8_t> data_vector(data, bytes_left, bytes_left);
        auto write_res = _block_device.writeBlock(data_vector, start);
        if (!write_res.has_value()) {
            return std::unexpected(write_res.error());
        }
        start.block_index++;
        bytes_written += write_res.value();
        data += write_res.value();
    }

    return {};
}

std::expected<Inode, FsError> InodeManager::_readInode(inode_index_t index)
{
    Inode inode;
    size_t bytes_read = 0;

    auto start = _getInodeLocation(index);
    if (_block_device.dataSize() - start.offset < sizeof(Inode)) {
        // Data doesn't fit in one block
        // We read the unaligned part here before reading the rest
        static_vector<uint8_t> data_vector(
            reinterpret_cast<uint8_t*>(&inode), sizeof(Inode), sizeof(Inode));
        auto read_res = _block_device.readBlock(start, sizeof(Inode), data_vector);
        if (!read_res.has_value()) {
            return std::unexpected(read_res.error());
        }
        start.offset = 0;
        start.block_index++;
        bytes_read += data_vector.size();
    }

    while (bytes_read < sizeof(Inode)) {
        int bytes_left = sizeof(Inode) - bytes_read;

        static_vector<uint8_t> data_vector(
            reinterpret_cast<uint8_t*>(&inode) + bytes_read, bytes_left, 0);
        auto read_res = _block_device.readBlock(start, bytes_left, data_vector);
        if (!read_res.has_value()) {
            return std::unexpected(read_res.error());
        }
        bytes_read += data_vector.size();
        start.block_index++;
    }
    return inode;
}

std::expected<inode_index_t, FsError> InodeManager::create(Inode& inode)
{
    auto result = _bitmap.getFirstEq(1); // one means free
    if (!result.has_value()) {
        if (result.error() == FsError::Bitmap_NotFound) {
            return std::unexpected(FsError::InodeManager_NoMoreFreeInodes);
        }
        return std::unexpected(result.error());
    }
    auto node_id = result.value();

    auto write_res = _writeInode(node_id, inode);

    if (!write_res.has_value()) {
        return std::unexpected(write_res.error());
    }

    auto setbit_res = _bitmap.setBit(node_id, 0);
    if (!setbit_res.has_value()) {
        return std::unexpected(setbit_res.error());
    }
    return node_id;
}

std::expected<void, FsError> InodeManager::remove(inode_index_t inode)
{
    auto is_free = _bitmap.getBit(inode);
    if (!is_free.has_value()) {
        return std::unexpected(is_free.error());
    }
    if (is_free.value()) {
        return std::unexpected(FsError::InodeManager_AlreadyFree);
    }
    return _bitmap.setBit(inode, 1);
}

std::expected<Inode, FsError> InodeManager::get(inode_index_t inode)
{
    auto is_free = _bitmap.getBit(inode);
    if (!is_free.has_value()) {
        return std::unexpected(is_free.error());
    }
    if (is_free.value()) {
        return std::unexpected(FsError::InodeManager_NotFound);
    }

    return _readInode(inode);
}

std::expected<unsigned int, FsError> InodeManager::numFree() { return _bitmap.count(1); }

std::expected<void, FsError> InodeManager::update(inode_index_t inode_index, const Inode& inode)
{
    auto is_free = _bitmap.getBit(inode_index);
    if (!is_free.has_value()) {
        return std::unexpected(is_free.error());
    }
    if (is_free.value()) {
        return std::unexpected(FsError::InodeManager_NotFound);
    }

    auto write_res = _writeInode(inode_index, inode);

    if (!write_res.has_value()) {
        return std::unexpected(write_res.error());
    }

    return {};
}

std::expected<void, FsError> InodeManager::format()
{
    auto set_res = _bitmap.setAll(1);
    if (!set_res.has_value()) {
        return std::unexpected(set_res.error());
    }
    return _createRootInode();
}

std::expected<void, FsError> InodeManager::_createRootInode()
{
    auto is_free = _bitmap.getBit(0);
    if (!is_free.has_value()) {
        return std::unexpected(is_free.error());
    }
    if (!is_free.value()) {
        return std::unexpected(FsError::InodeManager_AlreadyTaken);
    }

    Inode root { .file_size = 0, .type = InodeType::Directory };

    auto write_res = _writeInode(0, root);

    if (!write_res.has_value()) {
        return std::unexpected(write_res.error());
    }

    auto setbit_res = _bitmap.setBit(0, 0);
    if (!setbit_res.has_value()) {
        return std::unexpected(setbit_res.error());
    }

    return {};
}
