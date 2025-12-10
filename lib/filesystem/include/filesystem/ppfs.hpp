#pragma once
#include "common/ppfs_mutex.hpp"
#include "filesystem/ifilesystem.hpp"
#include "filesystem/open_files_table.hpp"

#include <optional>
#include <variant>

#include "block_manager/block_manager.hpp"
#include "blockdevice/crc_block_device.hpp"
#include "blockdevice/hamming_block_device.hpp"
#include "blockdevice/iblock_device.hpp"
#include "blockdevice/parity_block_device.hpp"
#include "blockdevice/raw_block_device.hpp"
#include "blockdevice/rs_block_device.hpp"
#include "directory_manager/directory_manager.hpp"
#include "disk/idisk.hpp"
#include "file_io/file_io.hpp"
#include "inode_manager/inode_manager.hpp"
#include "super_block_manager/super_block_manager.hpp"

#include <functional>

static constexpr size_t MAX_OPEN_FILES = 32;

class PpFS : public IFilesystem {
    IDisk& _disk;
    std::shared_ptr<Logger> _logger;

    std::variant<std::monostate, RawBlockDevice, CrcBlockDevice, HammingBlockDevice,
        ParityBlockDevice, ReedSolomonBlockDevice>
        _blockDeviceStorage;
    IBlockDevice* _blockDevice = nullptr;

    std::variant<std::monostate, SuperBlockManager> _superBlockManagerStorage;
    SuperBlockManager* _superBlockManager = nullptr;

    std::variant<std::monostate, InodeManager> _inodeManagerStorage;
    InodeManager* _inodeManager = nullptr;

    std::variant<std::monostate, BlockManager> _blockManagerStorage;
    BlockManager* _blockManager = nullptr;

    std::variant<std::monostate, DirectoryManager> _directoryManagerStorage;
    DirectoryManager* _directoryManager = nullptr;

    std::variant<std::monostate, FileIO> _fileIOStorage;
    FileIO* _fileIO = nullptr;

    inode_index_t _root = 0;
    SuperBlock _superBlock;

    PpFSMutex _mutex;
    OpenFilesTable<MAX_OPEN_FILES> _openFilesTable;

    [[nodiscard]] std::expected<inode_index_t, FsError> _getParentInodeFromPath(
        std::string_view path);
    [[nodiscard]] std::expected<inode_index_t, FsError> _getInodeFromPath(std::string_view path);
    [[nodiscard]] std::expected<inode_index_t, FsError> _getInodeFromParent(
        inode_index_t parent_inode, std::string_view path);
    bool _isPathValid(std::string_view path);
    [[nodiscard]] std::expected<void, FsError> _checkIfInUseRecursive(inode_index_t inode);
    [[nodiscard]] std::expected<void, FsError> _removeRecursive(
        inode_index_t parent, inode_index_t inode);
    [[nodiscard]] std::expected<void, FsError> _createAppropriateBlockDevice(size_t block_size,
        ECCType eccType, std::uint64_t polynomial, std::uint32_t correctable_bytes);

    [[nodiscard]] std::expected<void, FsError> _unprotectedCreate(std::string_view path);
    [[nodiscard]] std::expected<file_descriptor_t, FsError> _unprotectedOpen(
        std::string_view path, OpenMode mode = OpenMode::Normal);
    [[nodiscard]] std::expected<void, FsError> _unprotectedClose(file_descriptor_t fd);
    [[nodiscard]] std::expected<void, FsError> _unprotectedRemove(
        std::string_view path, bool recursive = false);
    [[nodiscard]] std::expected<std::vector<std::uint8_t>, FsError> _unprotectedRead(
        file_descriptor_t fd, std::size_t bytes_to_read);
    [[nodiscard]] std::expected<void, FsError> _unprotectedWrite(
        file_descriptor_t fd, std::vector<std::uint8_t> buffer);
    [[nodiscard]] std::expected<void, FsError> _unprotectedSeek(
        file_descriptor_t fd, size_t position);
    [[nodiscard]] std::expected<void, FsError> _unprotectedCreateDirectory(std::string_view path);
    [[nodiscard]] std::expected<std::vector<std::string>, FsError> _unprotectedReadDirectory(
        std::string_view path);
    [[nodiscard]] virtual std::expected<std::size_t, FsError> _unprotectedGetFileCount() const;

public:
    PpFS(IDisk& disk, std::shared_ptr<Logger> logger = nullptr);

    [[nodiscard]] virtual std::expected<void, FsError> init() override;
    [[nodiscard]] virtual std::expected<void, FsError> format(FsConfig options) override;
    [[nodiscard]] virtual std::expected<void, FsError> create(std::string_view path) override;
    [[nodiscard]] virtual std::expected<file_descriptor_t, FsError> open(
        std::string_view path, OpenMode mode = OpenMode::Normal) override;
    [[nodiscard]] virtual std::expected<void, FsError> close(file_descriptor_t fd) override;
    [[nodiscard]] virtual std::expected<void, FsError> remove(
        std::string_view path, bool recursive = false) override;
    [[nodiscard]] virtual std::expected<std::vector<std::uint8_t>, FsError> read(
        file_descriptor_t fd, std::size_t bytes_to_read) override;
    [[nodiscard]] virtual std::expected<void, FsError> write(
        file_descriptor_t fd, std::vector<std::uint8_t> buffer) override;
    [[nodiscard]] virtual std::expected<void, FsError> seek(
        file_descriptor_t fd, size_t position) override;
    [[nodiscard]] virtual std::expected<void, FsError> createDirectory(
        std::string_view path) override;
    [[nodiscard]] virtual std::expected<std::vector<std::string>, FsError> readDirectory(
        std::string_view path) override;
    virtual bool isInitialized() const override;
    [[nodiscard]] virtual std::expected<std::size_t, FsError> getFileCount() override;
};
