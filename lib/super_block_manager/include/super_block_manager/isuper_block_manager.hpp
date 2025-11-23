#pragma once
#include "super_block_manager/super_block.hpp"

#include <expected>

struct BlockRange {
    block_index_t start_block; /**< Inclusive start block index */
    block_index_t end_block; /**< Exclusive end block index */
};

/**
 * Interface with superblock operations
 */
struct ISuperBlockManager {
    /**
     * Returns current configuration of super_block
     *
     * @return Superblock on success, error otherwise
     */
    virtual std::expected<SuperBlock, FsError> get() = 0;

    /**
     * Writes a new superblock to disk, creating a zero version.
     * Only to be used during disk formatting
     *
     * @param new_super_block SuperBlock to be written.
     * @return void on success; DiskError on failure.
     */
    virtual std::expected<void, FsError> put(SuperBlock new_super_block) = 0;

    /**
     * Returns first and last block index that is not occupied by superblock.
     * Last block is exclusive (the first index occupied by suberblock).
     */
    virtual std::expected<BlockRange, FsError> getFreeBlocksIndexes() = 0;
};
