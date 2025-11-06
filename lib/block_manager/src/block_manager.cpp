#include "block_manager/block_manager.hpp"

#include "bitmap/bitmap.hpp"

BlockManager::BlockManager(Bitmap& bitmap, block_index_t data_blocks_start)
    : _bitmap(bitmap)
    , _data_blocks_start(data_blocks_start)
{
}
