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
    std::vector<std::uint8_t> data_vector(data, data + sizeof(inode));
    _block_device.writeBlock(data_vector, _getInodeLocation(node_id));

    _bitmap.setBit(node_id, 0);
    return 0;
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

    auto result = _block_device.readBlock(_getInodeLocation(inode), sizeof(Inode));
    if (!result.has_value()) {
        return std::unexpected(result.error());
    }

    return *(Inode*)(result.value().data());
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
    std::vector<std::uint8_t> data_vector(data, data + sizeof(inode));
    auto write_res = _block_device.writeBlock(data_vector, _getInodeLocation(inode_index));
    if (!write_res.has_value()) {
        return std::unexpected(write_res.error());
    }

    return {};
}
