#include "inode_manager/inode_manager.hpp"

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
        = _superblock.inode_table_address + inode * sizeof(Inode) / _superblock.block_size;
    result.offset = inode * sizeof(Inode) % _superblock.block_size;
    return result;
}

std::expected<void, FsError> InodeManager::_writeInode(inode_index_t index, const Inode& inode)
{
    // TODO: When we switch to static memory, optimise this
    auto data = (std::uint8_t*)(&inode);
    int bytes_written = 0;

    auto start = _getInodeLocation(index);
    if (_superblock.block_size - start.offset < sizeof(Inode)) {
        // Data won't fit in one block
        // We write the unaligned part here before writing the rest
        std::vector<std::uint8_t> data_vector(data, data + _superblock.block_size - start.offset);
        auto write_res = _block_device.writeBlock(data_vector, start);
        if (!write_res.has_value()) {
            return std::unexpected(write_res.error());
        }
        bytes_written = _superblock.block_size - start.offset;
        start.offset = 0;
        start.block_index++;
        data += bytes_written;
    }

    while (bytes_written < sizeof(Inode)) {
        int bytes_left = sizeof(Inode) - bytes_written;
        int bytes_to_write
            = bytes_left > _superblock.block_size ? _superblock.block_size : bytes_left;
        std::vector<std::uint8_t> data_vector(data, data + bytes_to_write);
        auto write_res = _block_device.writeBlock(data_vector, start);
        if (!write_res.has_value()) {
            return std::unexpected(write_res.error());
        }
        start.block_index++;
        bytes_written += bytes_to_write;
        data += bytes_to_write;
    }

    return { };
}

std::expected<Inode, FsError> InodeManager::_readInode(inode_index_t index)
{
    std::vector<std::uint8_t> data_vector;
    data_vector.reserve(sizeof(Inode));

    auto start = _getInodeLocation(index);
    if (_superblock.block_size - start.offset < sizeof(Inode)) {
        // Data doesn't fit in one block
        // We read the unaligned part here before reading the rest
        auto read_res = _block_device.readBlock(start, _superblock.block_size - start.offset);
        if (!read_res.has_value()) {
            return std::unexpected(read_res.error());
        }
        start.offset = 0;
        start.block_index++;
        data_vector.insert(data_vector.end(), read_res.value().begin(), read_res.value().end());
    }

    while (data_vector.size() < sizeof(Inode)) {
        int bytes_left = sizeof(Inode) - data_vector.size();
        int bytes_to_read
            = bytes_left > _superblock.block_size ? _superblock.block_size : bytes_left;

        auto read_res = _block_device.readBlock(start, bytes_to_read);
        if (!read_res.has_value()) {
            return std::unexpected(read_res.error());
        }
        data_vector.insert(data_vector.end(), read_res.value().begin(), read_res.value().end());
        start.block_index++;
    }
    return *reinterpret_cast<Inode*>(data_vector.data());
}

std::expected<inode_index_t, FsError> InodeManager::create(Inode& inode)
{
    auto result = _bitmap.getFirstEq(1); // one means free
    if (!result.has_value()) {
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

    return { };
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

    return { };
}
