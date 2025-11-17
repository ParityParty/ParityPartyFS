#include "inode_manager/inode_manager.hpp"

InodeManager::InodeManager(IBlockDevice& block_device, SuperBlock& superblock)
    : _block_device(block_device)
    , _superblock(superblock)
    , _bitmap(Bitmap(block_device, superblock.inode_bitmap_address, superblock.total_inodes))
{
}

std::expected<inode_index_t, DiskError> InodeManager::create(Inode inode) { }

std::expected<void, DiskError> InodeManager::remove(inode_index_t inode) { }
std::expected<Inode, DiskError> InodeManager::get(inode_index_t inode) { }
std::expected<unsigned int, DiskError> InodeManager::numFree() { }
