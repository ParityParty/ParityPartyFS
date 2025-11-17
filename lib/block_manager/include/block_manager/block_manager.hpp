#pragma once
#include "iblock_manager.hpp"

#include "bitmap/bitmap.hpp"
#include "common/types.hpp"

#include <optional>

class BlockManager : public IBlockManager {
    Bitmap _bitmap;
    block_index_t _data_blocks_start;
    block_index_t _num_data_blocks;
    std::optional<unsigned int> _num_free_blocks;

    block_index_t _toRelative(block_index_t absolute_block) const;
    block_index_t _toAbsolute(block_index_t relative_block) const;

public:
    /**
     * @param blocks_start Start of bitmap + data blocks region
     * @param space_for_data_blocks size of space for bitmap + data blocks
     * @param block_device device for io
     */
    BlockManager(block_index_t blocks_start, block_index_t space_for_data_blocks,
        IBlockDevice& block_device);
    std::expected<void, FsError> format() override;
    std::expected<void, FsError> reserve(block_index_t block) override;
    std::expected<void, FsError> free(block_index_t block) override;
    std::expected<block_index_t, FsError> getFree() override;
    std::expected<unsigned int, FsError> numFree() override;
    std::expected<unsigned int, FsError> numTotal() override;
};
