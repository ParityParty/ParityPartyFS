#pragma once
#include "disk/idisk.hpp"
#include "isuper_block_manager.hpp"
#include "super_block_manager/isuper_block_manager.hpp"

#include <optional>

struct SuperBlockEntry {
    block_index_t block_index; /**< Index on disk where this copy of the superblock is stored */

    SuperBlockEntry(block_index_t block_index);
};

struct VotingResult {
    const buffer<uint8_t>& finalData;
    bool damaged1;
    bool damaged2;
    bool damaged3;

    VotingResult(const buffer<uint8_t>& data, bool d1, bool d2, bool d3)
        : finalData(data)
        , damaged1(d1)
        , damaged2(d2)
        , damaged3(d3)
    {
    }
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
    IDisk& _disk; /**< Underlying disk */

public:
    SuperBlockManager(IDisk& disk);

    /**
     * Returns the current superblock from disk or from cache if available.
     *
     * @return On success, the SuperBlock; on failure, FsError.
     */
    std::expected<SuperBlock, FsError> get() override;

    /**
     * Writes a new superblock to disk, creating a zero version.
     * Only to be used during disk formatting
     *
     * @param new_super_block SuperBlock to be written.
     * @return void on success; FsError on failure.
     */
    std::expected<void, FsError> put(SuperBlock new_super_block) override;

    /**
     * Returns first and last block index that is not occupied by superblock.
     * Last block is exclusive (the first index occupied by suberblock).
     */
    std::expected<BlockRange, FsError> getFreeBlocksIndexes() override;

private:
    std::optional<SuperBlock> _superBlock; /**< Cached copy of the superblock */
    block_index_t _endByte; /**< Index where super blocks written at the beginning end */
    block_index_t _startByte; /**< Index where super blocks written at the end start */

    /**
     * Writes cached superblock to disk.
     */
    std::expected<void, FsError> _writeToDisk(bool writeAtBeginning, bool writeAtEnd);

    /**
     * Reads superblock from disk and returns it.
     */
    std::expected<void, FsError> _readFromDisk();

    /**
     * Performs bit voting on multiple copies of superblock data.
     */
    VotingResult _performBitVoting(const static_vector<uint8_t, 3 * sizeof(SuperBlock)> copies);
};
