#pragma once
#include "common/types.hpp"
#include "disk/idisk.hpp"

#include <expected>

/**
 * Interface containing data blocks operations
 *
 * Block should take range of blocks to manage, then it should calculate size, create and format its
 * own bitmap.
 */
struct IBlockManager {
    /**
     * Formats data on disk to work with block manager
     *
     * @return void on success, error otherwise
     */
    virtual std::expected<void, FsError> format();

    /**
     * Mark block as used
     *
     * @param block block index to be reserved, indexed from the first data block
     * @return void on success, error otherwise
     */
    virtual std::expected<void, FsError> reserve(block_index_t block);

    /**
     * Mark block as free
     *
     * @param block block that wants to break free
     * @return void on success, error otherwise
     */
    virtual std::expected<void, FsError> free(block_index_t block);

    /**
     * Get one free block
     *
     * @return index of a free block on success, error otherwise
     */
    virtual std::expected<block_index_t, FsError> getFree();

    /**
     * Calculate number of free blocks
     *
     * @return number of free data blocks on disk
     */
    virtual std::expected<unsigned int, FsError> numFree();

    /**
     * Get total number of data blocks
     *
     * @return total number of data blocks
     */
    virtual std::expected<unsigned int, FsError> numTotal();
};