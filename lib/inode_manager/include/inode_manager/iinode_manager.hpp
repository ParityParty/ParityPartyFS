#pragma once
#include "common/types.hpp"
#include "disk/idisk.hpp"
#include "inode_manager/inode.hpp"

#include <expected>
/**
 * Interface containing inode operations
 */
struct IInodeManager {
    /**
     * Creates new inode.
     *
     * Method makes new entry in inode table, and updates inode bitmap.
     *
     * @param inode data of the new inode
     * @return inode index on success, error otherwise
     */
    std::expected<inode_index_t, DiskError> create(Inode inode);

    /**
     * Remove inode.
     *
     * Method removes inode from inode table, and updates bitmap. Inode occurrences in directories
     * are uneffected by this method, so they need to be handled separately.
     *
     * @param inode index of inode to be deleted
     * @return void on success, error otherwise
     */
    std::expected<void, DiskError> remove(inode_index_t inode);

    /**
     * Read inode data from disc.
     *
     * @param inode index of inode to read
     * @return inode data on success, error otherwise
     */
    std::expected<Inode, DiskError> get(inode_index_t inode);

    /**
     * Calculate number of free inodes
     *
     * @return number of free inodes on success, error otherwise
     */
    std::expected<unsigned int, DiskError> numFree();

    /**
     * Get total number of inodes
     *
     * @return total number of inodes on success, error otherwise
     */
    std::expected<unsigned int, DiskError> numTotal();
};