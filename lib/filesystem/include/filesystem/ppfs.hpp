#pragma once
#include "common/ppfs_mutex.hpp"
#include "common/static_vector.hpp"
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

/**
 * ParityPartyFS - A fault-tolerant filesystem with configurable error correction.
 *
 * PpFS is a complete filesystem implementation that provides:
 * - Multiple error correction schemes (None, CRC, Hamming, Parity, Reed-Solomon)
 * - Inode-based file and directory management
 * - Block allocation and management
 * - Concurrent file access with mutex protection
 * - Superblock redundancy for metadata protection
 * - File descriptor-based API similar to POSIX
 *
 * The filesystem must be formatted with format() before use and initialized
 * with init() to set up internal structures. All operations are thread-safe.
 */
class PpFS : public virtual IFilesystem {
protected:
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
        std::string_view path) const;
    [[nodiscard]] std::expected<inode_index_t, FsError> _getInodeFromPath(std::string_view path);
    [[nodiscard]] std::expected<inode_index_t, FsError> _getInodeFromParent(
        inode_index_t parent_inode, std::string_view path) const;
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
    [[nodiscard]] std::expected<void, FsError> _unprotectedRead(
        file_descriptor_t fd, std::size_t bytes_to_read, static_vector<std::uint8_t>& data);
    [[nodiscard]] std::expected<size_t, FsError> _unprotectedWrite(
        file_descriptor_t fd, const static_vector<std::uint8_t>& buffer);
    [[nodiscard]] std::expected<void, FsError> _unprotectedSeek(
        file_descriptor_t fd, size_t position);
    [[nodiscard]] std::expected<void, FsError> _unprotectedCreateDirectory(std::string_view path);
    [[nodiscard]] std::expected<void, FsError> _unprotectedReadDirectory(
        std::string_view path, static_vector<DirectoryEntry>& entries);
    [[nodiscard]] std::expected<void, FsError> _unprotectedReadDirectory(file_descriptor_t fd,
        std::uint32_t elements, std::uint32_t offset, static_vector<DirectoryEntry>& entries);
    [[nodiscard]] virtual std::expected<std::size_t, FsError> _unprotectedGetFileCount();
    [[nodiscard]] virtual std::expected<FileStat, FsError> _unprotectedGetFileStat(
        std::string_view path);

public:
    /**
     * Constructs a PpFS instance.
     * @param disk Underlying disk device for storage.
     * @param logger Optional logger for error tracking and diagnostics.
     */
    PpFS(IDisk& disk, std::shared_ptr<Logger> logger = nullptr);

    /**
     * Initializes the filesystem from existing structures on disk.
     *
     * Loads the superblock, verifies error correction codes if configured,
     * and sets up all internal managers. Must be called before any filesystem operations.
     *
     * @return void on success, error otherwise.
     */
    [[nodiscard]] virtual std::expected<void, FsError> init() override;

    /**
     * Formats the underlying disk and creates filesystem structures.
     *
     * Creates superblock (with redundant copies), inode table, block/inode bitmaps,
     * and root directory. Configures the specified error correction scheme which will
     * be used for all subsequent read/write operations. All existing data is destroyed.
     *
     * @param options Configuration including block size, ECC type, and parameters.
     * @return void on success, error otherwise.
     */
    [[nodiscard]] virtual std::expected<void, FsError> format(FsConfig options) override;

    /**
     * Creates a new empty file at the specified path.
     *
     * The file must be opened before writing. Error correction encoding is applied
     * when the file is written to.
     *
     * @param path Absolute path including filename.
     * @return void on success, error otherwise.
     */
    [[nodiscard]] virtual std::expected<void, FsError> create(std::string_view path) override;

    /**
     * Opens a file or directory.
     *
     * For files, performs error detection/correction during initial metadata access.
     * Returns a file descriptor for subsequent operations. Each open must be paired with close.
     * For directories, mode is ignored.
     *
     * @param path Absolute path to file or directory.
     * @param mode Access mode (Normal, Append, Truncate, Exclusive, Protected).
     * @return File descriptor on success, error otherwise.
     */
    [[nodiscard]] virtual std::expected<file_descriptor_t, FsError> open(
        std::string_view path, OpenMode mode = OpenMode::Normal) override;

    /**
     * Closes an open file descriptor.
     *
     * @param fd File descriptor to close.
     * @return void on success, error otherwise.
     */
    [[nodiscard]] virtual std::expected<void, FsError> close(file_descriptor_t fd) override;

    /**
     * Removes a file or directory from the filesystem.
     *
     * Fails if the file is currently open. When removing files, associated data blocks
     * are freed after verifying integrity through error correction.
     *
     * @param path Absolute path to file or directory.
     * @param recursive If true, removes directories and their contents recursively.
     * @return void on success, error otherwise.
     */
    [[nodiscard]] virtual std::expected<void, FsError> remove(
        std::string_view path, bool recursive = false) override;

    /**
     * Reads data from an open file.
     *
     * Performs error detection and correction on each block read according to the
     * configured ECC scheme.
     *
     * @param fd File descriptor from open().
     * @param bytes_to_read Number of bytes to read.
     * @param data Output buffer (must have sufficient capacity).
     * @return void on success, error otherwise.
     */
    [[nodiscard]] virtual std::expected<void, FsError> read(file_descriptor_t fd,
        std::size_t bytes_to_read, static_vector<std::uint8_t>& data) override;

    /**
     * Writes data to an open file.
     *
     * Applies error correction encoding to each block written according to the configured
     * ECC scheme. Allocates additional blocks if needed and extends file size if writing
     * beyond current end.
     *
     * @param fd File descriptor from open().
     * @param buffer Data to write.
     * @return Number of bytes written on success, error otherwise.
     */
    [[nodiscard]] virtual std::expected<size_t, FsError> write(
        file_descriptor_t fd, const static_vector<std::uint8_t>& buffer) override;

    /**
     * Changes the current read/write position in a file.
     *
     * @param fd File descriptor from open().
     * @param position Byte offset from beginning of file.
     * @return void on success, error otherwise.
     */
    [[nodiscard]] virtual std::expected<void, FsError> seek(
        file_descriptor_t fd, size_t position) override;

    /**
     * Creates a new directory.
     *
     * Directory metadata is protected by the configured error correction scheme.
     *
     * @param path Absolute path including new directory name.
     * @return void on success, error otherwise.
     */
    [[nodiscard]] virtual std::expected<void, FsError> createDirectory(
        std::string_view path) override;

    /**
     * Reads all entries from a directory by path.
     *
     * Directory data is verified through error correction during read.
     *
     * @param path Absolute path to directory.
     * @param entries Output buffer (must have sufficient capacity).
     * @return void on success, error otherwise.
     */
    [[nodiscard]] virtual std::expected<void, FsError> readDirectory(
        std::string_view path, static_vector<DirectoryEntry>& entries) override;

    /**
     * Reads directory entries from an open directory descriptor.
     *
     * Allows paginated reading of large directories. Directory data is verified
     * through error correction during read.
     *
     * @param fd Directory file descriptor from open().
     * @param elements Number of entries to read (0 for all remaining).
     * @param offset Starting entry offset.
     * @param entries Output buffer (must have sufficient capacity).
     * @return void on success, error otherwise.
     */
    [[nodiscard]] virtual std::expected<void, FsError> readDirectory(file_descriptor_t fd,
        std::uint32_t elements, std::uint32_t offset,
        static_vector<DirectoryEntry>& entries) override;

    /**
     * Retrieves statistics for a file or directory.
     *
     * @param path Absolute path to file or directory.
     * @return FileStat structure on success, error otherwise.
     */
    [[nodiscard]] virtual std::expected<FileStat, FsError> getFileStat(
        std::string_view path) override;

    /**
     * Checks if the filesystem has been initialized.
     *
     * @return true if init() completed successfully, false otherwise.
     */
    virtual bool isInitialized() const override;

    /**
     * Returns the total number of files in the filesystem.
     *
     * @return File count on success, error otherwise.
     */
    [[nodiscard]] virtual std::expected<std::size_t, FsError> getFileCount() override;
};
