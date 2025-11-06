#pragma once
#include "disk/idisk.hpp"

#include <common/types.hpp>
#include <expected>

// TODO: change errors to be more descriptive

/**
 * Interface of basic filesystem operations
 */
struct IFilesystem {
    /**
     * Formats underlying disk to be used with filesystem.
     *
     * Method should create all needed structures (i.e. Superblocks, inode
     * table, bitmaps, journal)
     * for filesystem on a disk. Format can destroy all data on disk.
     *
     * @return void in case of success, error otherwise.
     */
    std::expected<void, DiskError> format();

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
    std::expected<inode_index_t, DiskError> open(std::string path);

    /**
     * Close file
     *
     * @param inode inode of file to close
     * @return void on success, error otherwise
     */
    std::expected<void, DiskError> close(inode_index_t inode);

    /**
     * Deletes file from the disk
     *
     * @param path absolute path to file to be removed
     * @return void on success, error otherwise
     */
    std::expected<void, DiskError> remove(std::string path);

    /**
     * Read bytes from disk
     *
     * @param inode inode of a file to read from
     * @param offset beggining of byte sequence to read
     * @param size amout of bytes to read
     * @return vector of bytes on success, error otherwise
     */
    std::expected<std::vector<std::byte>, DiskError> read(
        inode_index_t inode, size_t offset, size_t size);

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
    std::expected<void, DiskError> write(
        inode_index_t inode, std::vector<std::byte> buffer, size_t offset);

    /**
     * Creat new directory
     *
     * @param path absolute path to new directory, including new directory's
     * name
     * @return inode of new directory on success, error otherwise
     */
    std::expected<inode_index_t, DiskError> createDirectory(std::string path);
};
