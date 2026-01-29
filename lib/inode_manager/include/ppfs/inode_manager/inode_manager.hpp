#pragma once
#include "ppfs/bitmap/bitmap.hpp"
#include "ppfs/inode_manager/iinode_manager.hpp"
#include "ppfs/super_block_manager/super_block.hpp"

/**
 * Manages inode allocation, deallocation, and storage.
 */
class InodeManager : public IInodeManager {
    IBlockDevice& _block_device;
    SuperBlock& _superblock;
    Bitmap _bitmap;

    DataLocation _getInodeLocation(inode_index_t inode);
    [[nodiscard]] std::expected<void, FsError> _writeInode(inode_index_t index, const Inode& inode);
    [[nodiscard]] std::expected<Inode, FsError> _readInode(inode_index_t index);
    [[nodiscard]] std::expected<void, FsError> _createRootInode();

public:
    InodeManager(IBlockDevice& block_device, SuperBlock& superblock);

    [[nodiscard]] virtual std::expected<inode_index_t, FsError> create(Inode& inode) override;
    [[nodiscard]] virtual std::expected<void, FsError> remove(inode_index_t inode) override;
    [[nodiscard]] virtual std::expected<Inode, FsError> get(inode_index_t inode) override;
    [[nodiscard]] virtual std::expected<unsigned int, FsError> numFree() override;
    [[nodiscard]] virtual std::expected<void, FsError> update(
        inode_index_t inode_index, const Inode& inode) override;
    [[nodiscard]] virtual std::expected<void, FsError> format() override;
};
