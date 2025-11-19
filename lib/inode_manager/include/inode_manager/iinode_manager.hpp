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
    virtual std::expected<inode_index_t, FsError> create(Inode inode) = 0;

    /**
     * Remove inode.
     *
     * Method removes inode from inode table, and updates bitmap. Inode occurrences in directories
     * are uneffected by this method, so they need to be handled separately.
     *
     * @param inode index of inode to be deleted
     * @return void on success, error otherwise
     */
    virtual std::expected<void, FsError> remove(inode_index_t inode) = 0;

    /**
     * Read inode data from disc.
     *
     * @param inode index of inode to read
     * @return inode data on success, error otherwise
     */
    virtual std::expected<Inode, FsError> get(inode_index_t inode) = 0;

    /**
     * Calculate number of free inodes
     *
     * @return number of free inodes on success, error otherwise
     */
    virtual std::expected<unsigned int, FsError> numFree() = 0;

    /**
     * Update inode data on disc.
     *
     * @param inode index of inode to update
     * @return void on success, error otherwise
     */
    virtual std::expected<void, FsError> update(const Inode& inode) = 0;
};