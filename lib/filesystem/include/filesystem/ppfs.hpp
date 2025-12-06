#pragma once
#include "filesystem/ifilesystem.hpp"
#include <memory>
#include <optional>

#include "block_manager/block_manager.hpp"
#include "blockdevice/iblock_device.hpp"
#include "directory_manager/directory_manager.hpp"
#include "disk/idisk.hpp"
#include "inode_manager/inode_manager.hpp"
#include "super_block_manager/super_block_manager.hpp"

class PpFS : public IFilesystem {
    IDisk& _disk;
    std::unique_ptr<IBlockDevice> _blockDevice;
    std::unique_ptr<ISuperBlockManager> _superBlockManager;
    std::unique_ptr<IInodeManager> _inodeManager;
    std::unique_ptr<IBlockManager> _blockManager;
    std::unique_ptr<IDirectoryManager> _directoryManager;

    inode_index_t _root = 0;

public:
    PpFS(IDisk& disk);

    virtual std::expected<void, FsError> init() override;
    virtual std::expected<void, FsError> format(FsConfig options) override;
    virtual std::expected<void, FsError> create(std::string path) override;
    virtual std::expected<inode_index_t, FsError> open(std::string path) override;
    virtual std::expected<void, FsError> close(inode_index_t inode) override;
    virtual std::expected<void, FsError> remove(std::string path) override;
    virtual std::expected<std::vector<std::uint8_t>, FsError> read(
        inode_index_t inode, size_t offset, size_t size) override;
    virtual std::expected<void, FsError> write(
        inode_index_t inode, std::vector<std::uint8_t> buffer, size_t offset) override;
    virtual std::expected<void, FsError> createDirectory(std::string path) override;
    virtual std::expected<std::vector<std::string>, FsError> readDirectory(
        std::string path) override;
};
