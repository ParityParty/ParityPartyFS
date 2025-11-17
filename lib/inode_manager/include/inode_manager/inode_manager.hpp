#pragma once
#include "bitmap/bitmap.hpp"
#include "inode_manager/iinode_manager.hpp"
#include "super_block_manager/super_block.hpp"

class InodeManager : public IInodeManager {
    IBlockDevice& _block_device;
    SuperBlock& _superblock;

    Bitmap _bitmap;

public:
    InodeManager(IBlockDevice& block_device, SuperBlock& superblock);

    virtual std::expected<inode_index_t, DiskError> create(Inode inode) override;
    virtual std::expected<void, DiskError> remove(inode_index_t inode) override;
    virtual std::expected<Inode, DiskError> get(inode_index_t inode) override;
    virtual std::expected<unsigned int, DiskError> numFree() override;
    virtual std::expected<unsigned int, DiskError> numTotal() override;
};
