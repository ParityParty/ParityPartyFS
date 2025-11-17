#pragma once
#include "disk/idisk.hpp"
#include "super_block_manager/super_block.hpp"

#include <expected>

/**
 * Interface with superblock operations
 */
struct ISuperBlockManager {
    /**
     * Read superblock from disk.
     *
     * @return Superblock on success, error otherwise
     */
    std::expected<SuperBlock, FsError> get();

    /**
     * Update superblock on disk
     *
     * @param new_super_block new superblock data
     * @return void on success, error otherwise
     */
    std::expected<void, FsError> update(SuperBlock new_super_block);
};
