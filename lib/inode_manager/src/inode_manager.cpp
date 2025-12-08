#include "inode_manager/inode_manager.hpp"
#include "static_vector.hpp"

InodeManager::InodeManager(IBlockDevice& block_device, SuperBlock& superblock)
    : _block_device(block_device)
    , _superblock(superblock)
    , _bitmap(Bitmap(block_device, superblock.inode_bitmap_address, superblock.total_inodes))
{
}

DataLocation InodeManager::_getInodeLocation(inode_index_t inode)
{
    DataLocation result;
    auto inodes_in_block = _superblock.block_size / sizeof(Inode);
    result.block_index = _superblock.inode_table_address + inode / inodes_in_block;
    result.offset = sizeof(Inode) * (inode % inodes_in_block);
    return result;
}

std::expected<inode_index_t, FsError> InodeManager::create(Inode& inode)
{
    auto result = _bitmap.getFirstEq(1); // one means free
    if (!result.has_value()) {
        return std::unexpected(result.error());
    }
    auto node_id = result.value();

    auto data = (std::uint8_t*)(&inode);
    static_vector<std::uint8_t, sizeof(Inode)> data_vector(data, data + sizeof(inode));
    auto write_res = _block_device.writeBlock(data_vector, _getInodeLocation(node_id));
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
        return std::unexpected(FsError::AlreadyFree);
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
        return std::unexpected(FsError::NotFound);
    }
    static_vector<uint8_t, sizeof(Inode)> buf(sizeof(Inode));
    auto result = _block_device.readBlock(_getInodeLocation(inode), sizeof(Inode), buf);
    if (!result.has_value()) {
        return std::unexpected(result.error());
    }

    Inode inode_result = *(Inode*)buf.data();

    return inode_result;
}

std::expected<unsigned int, FsError> InodeManager::numFree() { return _bitmap.count(1); }

std::expected<void, FsError> InodeManager::update(inode_index_t inode_index, const Inode& inode)
{
    auto is_free = _bitmap.getBit(inode_index);
    if (!is_free.has_value()) {
        return std::unexpected(is_free.error());
    }
    if (is_free.value()) {
        return std::unexpected(FsError::NotFound);
    }

    auto data = (std::uint8_t*)(&inode);
    static_vector<std::uint8_t, sizeof(Inode)> data_vector(data, data + sizeof(inode));
    auto write_res = _block_device.writeBlock(data_vector, _getInodeLocation(inode_index));
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
        return std::unexpected(FsError::AlreadyTaken);
    }

    Inode root { .file_size = 0, .type = InodeType::Directory };

    auto data = (std::uint8_t*)(&root);
    static_vector<std::uint8_t, sizeof(Inode)> data_vector(data, data + sizeof(root));
    auto write_res = _block_device.writeBlock(data_vector, _getInodeLocation(0));
    if (!write_res.has_value()) {
        return std::unexpected(write_res.error());
    }

    auto setbit_res = _bitmap.setBit(0, 0);
    if (!setbit_res.has_value()) {
        return std::unexpected(setbit_res.error());
    }

    return {};
}
