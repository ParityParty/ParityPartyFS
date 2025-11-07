#pragma once
#include "iblock_manager.hpp"

#include "bitmap/bitmap.hpp"
#include "common/types.hpp"

class BlockManager : public IBlockManager {
    Bitmap& _bitmap;
    block_index_t _data_blocks_start;

public:
    BlockManager(Bitmap& bitmap, block_index_t data_blocks_start);
};
