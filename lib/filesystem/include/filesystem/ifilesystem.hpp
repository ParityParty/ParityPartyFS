#pragma once
#include "common/static_vector.hpp"
#include "directory_manager/directory.hpp"
#include "disk/idisk.hpp"
#include "filesystem/types.hpp"

#include "common/types.hpp"
#include <expected>
#include <string>

// TODO: change errors to be more descriptive

/**
 * Interface of basic filesystem operations
 */
struct IFilesystem {
    virtual ~IFilesystem() = default;
    /**
     * Initializes filesystem with structures already existing on disk.
     *
     * @return void in case of success, error otherwise.
     */
    [[nodiscard]] virtual std::expected<void, FsError> init() = 0;

    /**
     * Formats underlying disk to be used with filesystem.
     *
     * Method creates all structures (i.e. Superblocks, inode table, bitmaps, journal)
     * for the filesystem on a disk. Format can destroy all data on disk.
     *
     * @param options configuration options for filesystem
     * @return void in case of success, error otherwise.
     */
    [[nodiscard]] virtual std::expected<void, FsError> format(FsConfig options) = 0;

    /**
     * Create new file
     *
     * The file needs to be opened to write after creation.
     *
     * @param path absolute path to new file, including its name
     * @return void on success, error otherwise
     */
    [[nodiscard]] virtual std::expected<void, FsError> create(std::string_view path) = 0;

    /**
     * Opens file for read/write and directories for read.
     *
     * Method can check data integrity. Every call to open should be paired with
     * close on the returned file descriptor.
     *
     * If the file is a directory, mode argument is ignored.
     *
     * @param path absolute path to file to be opened
     * @param mode mode in which to open the file
     * @return file descriptor to be used with read, write, close operations on success,
     * error otherwise
     */
    [[nodiscard]] virtual std::expected<file_descriptor_t, FsError> open(
        std::string_view path, OpenMode mode = OpenMode::Normal) = 0;

    /**
     * Close file
     *
     * @param fd file descriptor of file to close
     * @return void on success, error otherwise
     */
    [[nodiscard]] virtual std::expected<void, FsError> close(file_descriptor_t fd) = 0;

    /**
     * Remove file(s) from the disk. If the files are open, removal fails and no action is taken.
     *
     * @param path absolute path to file or directory to be removed
     * @param recursive if true, remove directories and their contents recursively
     * @return void on success, error otherwise
     */
    [[nodiscard]] virtual std::expected<void, FsError> remove(std::string_view path, bool recursive = false) = 0;

    /**
     * Read bytes from disk
     *
     * @param fd file descriptor of a file to read from
     * @param bytes_to_read number of bytes to read
     * @param data buffer to fill with read data, must have sufficient capacity
     * @return void on success, error otherwise
     */
    [[nodiscard]] virtual std::expected<void, FsError> read(
        file_descriptor_t fd, std::size_t bytes_to_read, static_vector<std::uint8_t>& data) = 0;

    /**
     * Write data from buffer to file.
     *
     * Write data from buffer starting at offset. If file size is too small,
     * method makes it larger.
     *
     * @param fd file descriptor of file to write to
     * @param buffer data to write
     * @return number of written bytes, error otherwise
     */

    [[nodiscard]] virtual std::expected<size_t, FsError> write(
        file_descriptor_t fd, const static_vector<std::uint8_t>& buffer)
        = 0;

    /**
     * Seek to position in file
     *
     * @param fd file descriptor of file to seek in
     * @param position position to seek to
     * @return void on success, error otherwise
     */
    [[nodiscard]] virtual std::expected<void, FsError> seek(file_descriptor_t fd, size_t position) = 0;

    /**
     * Create new directory
     *
     * @param path absolute path to new directory, including new directory's
     * name
     * @return void success, error otherwise
     */
    [[nodiscard]] virtual std::expected<void, FsError> createDirectory(std::string_view path) = 0;

    /**
     * Read entries of a directory
     *
     * @param fd file descriptor of the directory
     * @param elements number of directory entries to read (0 reads all)
     * @param offset element offset from which to start reading
     * @param entries buffer to fill with directory entries, must have sufficient capacity
     * @return void on success, error otherwise
     */
    [[nodiscard]] virtual std::expected<void, FsError> readDirectory(
        file_descriptor_t fd, std::uint32_t elements, std::uint32_t offset, static_vector<DirectoryEntry>& entries) = 0;

    /**
     * Read all entries of a directory
     *
     * @param path absolute path to directory
     * @param entries buffer to fill with directory entries, must have sufficient capacity
     * @return void on success, error otherwise
     */
    [[nodiscard]] virtual std::expected<void, FsError> readDirectory(
        std::string_view path, static_vector<DirectoryEntry>& entries) = 0;

    /**
     * Check if filesystem has been initialized and is ready for operations
     *
     * @return true if initialized, false otherwise
     */
    virtual bool isInitialized() const = 0;

    /**
     * Get number of files in the filesystem
     *
     * @return number of files
     */
    [[nodiscard]] virtual std::expected<std::size_t, FsError> getFileCount() = 0;
};
