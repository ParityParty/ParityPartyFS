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
 * Stores two copies of the superblock on disk and provides
 * methods to read and update the filesystem metadata safely.
 */
class SuperBlockManager : public ISuperBlockManager {
    IBlockDevice& _block_device; /**< Underlying block device storing superblocks */

public:
    /**
     * Constructs manager.
     *
     * @param block_device Reference to underlying block device.
     */
    SuperBlockManager(IBlockDevice& block_device);

    /**
     * Returns the current superblock from disk or from cache if available.
     *
     * @return On success, the SuperBlock; on failure, DiskError.
     */
    std::expected<SuperBlock, DiskError> get();

    /**
     * Updates the superblocks on disk with new data.
     *
     * @param new_super_block SuperBlock to persist.
     * @return void on success; DiskError on failure.
     */
    std::expected<void, DiskError> update(SuperBlock new_super_block);

private:
    std::optional<SuperBlock> _superBlock; /**< Cached copy of the superblock */

    std::vector<SuperBlockEntry> _entries; /**< List of all superblock copies with status */

    /**
     * Writes cached superblock to a specific index.
     */
    std::expected<void, DiskError> _writeToDisk(block_index_t block_index);

    /**
     * Reads superblock from disk and returns it.
     */
    std::expected<SuperBlock, DiskError> _readFromDisk(block_index_t block_index);
};
