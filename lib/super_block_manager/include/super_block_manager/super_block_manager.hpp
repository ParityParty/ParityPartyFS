#pragma once
#include "blockdevice/iblock_device.hpp"
#include "isuper_block_manager.hpp"
#include "super_block_manager/isuper_block_manager.hpp"

#include <optional>

struct SuperBlockEntry {
    block_index_t block_index; /**< Index on disk where this copy of the superblock is stored */

    SuperBlockEntry(block_index_t block_index);
};

/**
 * Implements superblock management for the filesystem.
 *
 * Stores two copies of the superblock at the beginning of a disk and one at
 * the end; and provides methods to read and update the filesystem metadata safely.
 *
 * We assume there is only one class instance, so cached superblock
 * is the most recent one.
 */
class SuperBlockManager : public ISuperBlockManager {
    IBlockDevice& _block_device; /**< Underlying block device storing superblocks */

public:
    SuperBlockManager(IBlockDevice& block_device);

    /**
     * Returns the current superblock from disk or from cache if available.
     *
     * @return On success, the SuperBlock; on failure, DiskError.
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

    /**
     * Returns block indexes which are occupied by super blocks
     */
    std::vector<block_index_t> getOccupiedIndexes();

private:
    std::optional<SuperBlock> _superBlock; /**< Cached copy of the superblock */
    block_index_t _endBlock; /**< Index where super blocks written at the beginning end */
    block_index_t _startBlock; /**< Index where super blocks written at the end start */

    /**
     * Writes cached superblock to a specific index.
     */
    std::expected<void, DiskError> _writeToDisk();

    /**
     * Reads superblock from disk and returns it.
     */
    std::expected<SuperBlock, DiskError> _readFromDisk();

    /**
     * Constructs manager.
     *
     * @param block_device Reference to underlying block device.
     */
};
