#pragma once

#include "common/static_vector.hpp"
#include "directory_manager/directory.hpp"
#include "disk/idisk.hpp"
#include "inode_manager/inode.hpp"
#include <expected>

/**
 * Interface of directory operations
 *
 * Directory is just a file with list of directory entries, composed of inode and file name.
 * This interface is operations on those lists.
 */
struct IDirectoryManager {
    /**
     * Get entries of a directory.
     *
     * @param inode inode of a directory
     * @param elements elements to retrieve (0 = all)
     * @param offset element offset to start from
     * @param buf buffer to fill with directory entries, must have sufficient capacity
     * @return void on success, error otherwise
     */
    [[nodiscard]] virtual std::expected<void, FsError> getEntries(
        inode_index_t inode, std::uint32_t elements, std::uint32_t offset, static_vector<DirectoryEntry>& buf)
        = 0;

    /**
     * Add entry to existing directory. Checks if the name is unique in the parent directory.
     *
     * @param directory inode of directory to add entry to
     * @param entry new entry
     * @return void on success, error otherwise
     */
    [[nodiscard]] virtual std::expected<void, FsError> addEntry(inode_index_t directory, DirectoryEntry entry)
        = 0;

    /**
     * Remove entry from a directory. Does not free file blocks and
     * does not free inode.
     *
     * @param directory directory to remove entry from
     * @param entry inode of entry to be removed
     * @return void on success, error otherwise
     */
    [[nodiscard]] virtual std::expected<void, FsError> removeEntry(inode_index_t directory, inode_index_t entry)
        = 0;

    /**
     * Check if name is unique in the given directory.
     * @param directory inode of directory to check
     * @param name name to check
     * @return directory inode on success, error otherwise
     */
    [[nodiscard]] virtual std::expected<Inode, FsError> checkNameUnique(inode_index_t directory, const char* name)
        = 0;
};
