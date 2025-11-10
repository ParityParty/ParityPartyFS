#pragma once
#include "disk/idisk.hpp"
#include "super_block_manager/super_block.hpp"

#include <expected>

#define SUPERBLOCK_INDX1 0
#define SUPERBLOCK_INDX2 9

/**
 * Interface with superblock operations.
 *
 * We store two copies of super block in fs, under indexes
 *  defined above.
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
