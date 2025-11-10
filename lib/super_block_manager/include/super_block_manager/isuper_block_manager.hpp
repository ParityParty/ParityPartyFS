#pragma once
#include "disk/idisk.hpp"
#include "super_block_manager/super_block.hpp"

#include <expected>

/**
 * Interface with superblock operations.
 */
struct ISuperBlockManager {
    /**
     * Returns current configuration of super_block
     *
     * @return Superblock on success, error otherwise
     */
    std::expected<SuperBlock, DiskError> get();

    /**
     * Update superblock on disk
     *
     * @param new_super_block new superblock data
     * @return void on success, error otherwise
     */
    std::expected<void, DiskError> update(SuperBlock new_super_block);
};
