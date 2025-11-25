#pragma once
#include "directory_manager/directory.hpp"
#include "disk/idisk.hpp"

#include "common/types.hpp"
#include <expected>

// TODO: change errors to be more descriptive

/**
 * Interface of basic filesystem operations
 */
struct IFilesystem {
    virtual ~IFilesystem() = default;
    /**
     * Formats underlying disk to be used with filesystem.
     *
     * Method should create all needed structures (i.e. Superblocks, inode
     * table, bitmaps, journal)
     * for filesystem on a disk. Format can destroy all data on disk.
     *
     * @return void in case of success, error otherwise.
     */
    virtual std::expected<void, FsError> format() = 0;

    /**
     * Create new file
     *
     * The file needs to be opened to write after creation.
     *
     * @param path absolute path to new file, including its name
     * @return void on success, error otherwise
     */
    virtual std::expected<void, FsError> create(std::string path) = 0;

    /**
     * Opens file for read/write operations.
     *
     * Method can check data integrity. Every call to open should be paired with
     * close one the returned inode
     *
     * @param path absolute path to file to be opened
     * @return inode to be used with read, write, close operations on success,
     * error otherwise
     */
    virtual std::expected<inode_index_t, FsError> open(std::string path) = 0;

    /**
     * Close file
     *
     * @param inode inode of file to close
     * @return void on success, error otherwise
     */
    virtual std::expected<void, FsError> close(inode_index_t inode) = 0;

    /**
     * Deletes file from the disk
     *
     * @param path absolute path to file to be removed
     * @return void on success, error otherwise
     */
    virtual std::expected<void, FsError> remove(std::string path) = 0;

    /**
     * Read bytes from disk
     *
     * @param inode inode of a file to read from
     * @param offset beginning of byte sequence to read
     * @param size amount of bytes to read
     * @return vector of bytes on success, error otherwise
     */
    virtual std::expected<std::vector<std::uint8_t>, FsError> read(
        inode_index_t inode, size_t offset, size_t size)
        = 0;

    /**
     * Write data from buffer to file.
     *
     * Write data from buffer starting at offset. If file size is too small,
     * method makes it larger.
     *
     * @param inode inode of file to write to
     * @param buffer data to write
     * @param offset place to write data
     * @return void on success, error otherwise
     */
    virtual std::expected<void, FsError> write(
        inode_index_t inode, std::vector<std::uint8_t> buffer, size_t offset)
        = 0;

    /**
     * Create new directory
     *
     * @param path absolute path to new directory, including new directory's
     * name
     * @return void success, error otherwise
     */
    virtual std::expected<void, FsError> createDirectory(std::string path) = 0;

    /**
     * Read entries of a directory
     *
     * @param path Absolute path to directory
     * @return list of filenames on success, error otherwise
     */
    virtual std::expected<std::vector<std::string>, FsError> readDirectory(std::string path) = 0;
};
