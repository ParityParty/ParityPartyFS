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
 *
 * We assume there is only one class instance, so cached superblock
 * is the most recent one.
 */
class SuperBlockManager : public ISuperBlockManager {
    IBlockDevice& _block_device; /**< Underlying block device storing superblocks */

public:
    /**
     * Checks if provided entries are valid and superblocks won't overlap.
     * If validation passes, returns superblock instance.
     *
     * @param block_device Reference to underlying block device.
     */
    static std::optional<SuperBlockManager> CreateInstance(
        IBlockDevice& block_device, std::vector<SuperBlockEntry> entries);

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
     * Updates the superblocks on disk with new data.
     *
     * @param new_super_block SuperBlock to persist.
     * @return void on success; DiskError on failure.
     */
    std::expected<void, DiskError> update(SuperBlock new_super_block);

private:
    std::optional<RawSuperBlock> _rawSuperBlock; /**< Cached copy of the superblock */

    std::vector<SuperBlockEntry> _entries; /**< List of all superblock copies with status */

    /**
     * Writes cached superblock to a specific index.
     */
    std::expected<void, DiskError> _writeToDisk(block_index_t block_index);

    /**
     * Reads superblock from disk and returns it.
     */
    std::expected<RawSuperBlock, DiskError> _readFromDisk(block_index_t block_index);

    /**
     * Constructs manager.
     *
     * @param block_device Reference to underlying block device.
     */
    SuperBlockManager(IBlockDevice& block_device, std::vector<SuperBlockEntry> entries);
};
