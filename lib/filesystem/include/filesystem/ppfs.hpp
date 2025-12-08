#pragma once
#include "filesystem/ifilesystem.hpp"
#include <memory>
#include <optional>

#include "block_manager/block_manager.hpp"
#include "blockdevice/iblock_device.hpp"
#include "directory_manager/directory_manager.hpp"
#include "disk/idisk.hpp"
#include "file_io/file_io.hpp"
#include "inode_manager/inode_manager.hpp"
#include "super_block_manager/super_block_manager.hpp"

class PpFS : public IFilesystem {
    IDisk& _disk;
    std::unique_ptr<IBlockDevice> _blockDevice;
    std::unique_ptr<ISuperBlockManager> _superBlockManager;
    std::unique_ptr<IInodeManager> _inodeManager;
    std::unique_ptr<IBlockManager> _blockManager;
    std::unique_ptr<IDirectoryManager> _directoryManager;
    std::unique_ptr<FileIO> _fileIO;

    inode_index_t _root = 0;
    SuperBlock _superBlock;

    std::expected<inode_index_t, FsError> _getParentInodeFromPath(std::string_view path);
    std::expected<inode_index_t, FsError> _getInodeFromPath(std::string_view path);
    bool _isPathValid(std::string_view path);
    std::expected<void, FsError> _isUniqueInDirectory(
        inode_index_t dir_inode, std::string_view name);

public:
    PpFS(IDisk& disk);

    virtual std::expected<void, FsError> init() override;
    virtual std::expected<void, FsError> format(FsConfig options) override;
    virtual std::expected<void, FsError> create(std::string_view path) override;
    virtual std::expected<inode_index_t, FsError> open(std::string_view path) override;
    virtual std::expected<void, FsError> close(inode_index_t inode) override;
    virtual std::expected<void, FsError> remove(std::string_view path) override;
    virtual std::expected<std::vector<std::uint8_t>, FsError> read(
        inode_index_t inode, size_t offset, size_t size) override;
    virtual std::expected<void, FsError> write(
        inode_index_t inode, std::vector<std::uint8_t> buffer, size_t offset) override;
    virtual std::expected<void, FsError> createDirectory(std::string_view path) override;
    virtual std::expected<std::vector<std::string>, FsError> readDirectory(
        std::string_view path) override;
    virtual bool isInitialized() const override;
};
