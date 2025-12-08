#pragma once
#include "common/ppfs_mutex.hpp"
#include "filesystem/ifilesystem.hpp"
#include "filesystem/open_files_table.hpp"
#include <memory>
#include <optional>

#include "block_manager/block_manager.hpp"
#include "blockdevice/iblock_device.hpp"
#include "directory_manager/directory_manager.hpp"
#include "disk/idisk.hpp"
#include "file_io/file_io.hpp"
#include "inode_manager/inode_manager.hpp"
#include "super_block_manager/super_block_manager.hpp"

static constexpr size_t MAX_OPEN_FILES = 32;

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

    PpFSMutex _mutex;
    OpenFilesTable<MAX_OPEN_FILES> _openFilesTable;

    std::expected<inode_index_t, FsError> _getParentInodeFromPath(std::string_view path);
    std::expected<inode_index_t, FsError> _getInodeFromPath(std::string_view path);
    std::expected<inode_index_t, FsError> _getInodeFromParent(
        inode_index_t parent_inode, std::string_view path);
    bool _isPathValid(std::string_view path);
    std::expected<void, FsError> _isUniqueInDirectory(
        inode_index_t dir_inode, std::string_view name);
    std::expected<void, FsError> _checkIfInUseRecursive(inode_index_t inode);
    std::expected<void, FsError> _removeRecursive(inode_index_t parent, inode_index_t inode);

public:
    PpFS(IDisk& disk);

    virtual std::expected<void, FsError> init() override;
    virtual std::expected<void, FsError> format(FsConfig options) override;
    virtual std::expected<void, FsError> create(std::string_view path) override;
    virtual std::expected<file_descriptor_t, FsError> open(
        std::string_view path, OpenMode mode = OpenMode::Normal) override;
    virtual std::expected<void, FsError> close(file_descriptor_t fd) override;
    virtual std::expected<void, FsError> remove(
        std::string_view path, bool recursive = false) override;
    virtual std::expected<std::vector<std::uint8_t>, FsError> read(
        file_descriptor_t fd, std::size_t bytes_to_read) override;
    virtual std::expected<void, FsError> write(
        file_descriptor_t fd, std::vector<std::uint8_t> buffer) override;
    virtual std::expected<void, FsError> seek(file_descriptor_t fd, size_t position) override;
    virtual std::expected<void, FsError> createDirectory(std::string_view path) override;
    virtual std::expected<std::vector<std::string>, FsError> readDirectory(
        std::string_view path) override;
    virtual bool isInitialized() const override;
    virtual std::expected<std::size_t, FsError> getFileCount() const override;
};
