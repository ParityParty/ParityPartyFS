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
     * Writes a new superblock to disk, creating a zero version.
     * Only to be used during disk formatting
     *
     * @param new_super_block SuperBlock to be written.
     * @return void on success; DiskError on failure.
     */
    std::expected<void, DiskError> put(SuperBlock new_super_block);
};
