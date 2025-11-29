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

    virtual std::expected<inode_index_t, FsError> create(Inode inode) override;
    virtual std::expected<void, FsError> remove(inode_index_t inode) override;
    virtual std::expected<Inode, FsError> get(inode_index_t inode) override;
    virtual std::expected<unsigned int, FsError> numFree() override;
    virtual std::expected<void, FsError> update(const Inode& inode) override;
};
