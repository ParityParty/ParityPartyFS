#pragma once
#include "directory_manager/directory.hpp"
#include "disk/idisk.hpp"
#include <expected>
#include <vector>

/**
 * Interface of directory operations
 */
struct IDirectoryManager {
    /**
     * Get entries of a directory.
     *
     * @param inode inode of a directory
     * @return list of directory entries on success, error otherwise
     */
    std::expected<std::vector<DirectoryEntry>, DiskError> getEntries(inode_index_t inode);

    /**
     * Add entry to existing directory.
     *
     * @param directory inode of directory to add entry to
     * @param entry new entry
     * @return void on success, error otherwise
     */
    std::expected<void, DiskError> addEntry(inode_index_t directory, DirectoryEntry entry);

    /**
     * Remove entry from a directory
     *
     * @param directory directory to remove entry from
     * @param entry inode of entry to be removed
     * @return void on success, error otherwise
     */
    std::expected<void, DiskError> removeEntry(inode_index_t directory, inode_index_t entry);
};
