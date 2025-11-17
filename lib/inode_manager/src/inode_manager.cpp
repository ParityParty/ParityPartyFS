#include "inode_manager/inode_manager.hpp"

InodeManager::InodeManager(IBlockDevice& block_device, SuperBlock& superblock)
    : _block_device(block_device)
    , _superblock(superblock)
    , _bitmap(Bitmap(block_device, superblock.inode_bitmap_address, superblock.total_inodes))
{
}

std::expected<inode_index_t, FsError> InodeManager::create(Inode inode) { return 0; }

std::expected<void, FsError> InodeManager::remove(inode_index_t inode) { return {}; }
std::expected<Inode, FsError> InodeManager::get(inode_index_t inode) { return Inode {}; }
std::expected<unsigned int, FsError> InodeManager::numFree() { return 0; }
