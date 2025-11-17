#pragma once
#include "iblock_manager.hpp"

#include "bitmap/bitmap.hpp"
#include "common/types.hpp"

class BlockManager : public IBlockManager {
    Bitmap _bitmap;
    block_index_t _data_blocks_start;
    block_index_t _num_data_blocks;

public:
    BlockManager(
        block_index_t blocks_start, block_index_t num_data_blocks, IBlockDevice& block_device);
    std::expected<void, BitmapError> format() override;
    std::expected<void, BitmapError> reserve(block_index_t block) override;
    std::expected<void, BitmapError> free(block_index_t block) override;
    std::expected<block_index_t, BitmapError> getFree() override;
    std::expected<unsigned int, BitmapError> countFree() override;
    std::expected<unsigned int, BitmapError> numTotal() override;
};
