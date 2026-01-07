#pragma once
#include "iblock_manager.hpp"
#include "super_block_manager/super_block.hpp"

#include "bitmap/bitmap.hpp"
#include "common/types.hpp"

#include <optional>

/**
 * Manages allocation and deallocation of data blocks.
 */
class BlockManager : public IBlockManager {
    Bitmap _bitmap;
    block_index_t _data_blocks_start;
    block_index_t _num_data_blocks;

    block_index_t _toRelative(block_index_t absolute_block) const;
    block_index_t _toAbsolute(block_index_t relative_block) const;

public:
    /**
     * @param sb superblock with valid bitmap and data addresses
     * @param block_device device for io
     */
    BlockManager(const SuperBlock& sb, IBlockDevice& block_device);
    [[nodiscard]] virtual std::expected<void, FsError> format() override;
    [[nodiscard]] virtual std::expected<void, FsError> reserve(block_index_t block) override;
    [[nodiscard]] virtual std::expected<void, FsError> free(block_index_t block) override;
    [[nodiscard]] virtual std::expected<block_index_t, FsError> getFree() override;
    [[nodiscard]] virtual std::expected<std::uint32_t, FsError> numFree() override;
    [[nodiscard]] virtual std::expected<std::uint32_t, FsError> numTotal() override;
};
