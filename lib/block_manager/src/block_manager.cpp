#include "block_manager/block_manager.hpp"

#include "bitmap/bitmap.hpp"

BlockManager::BlockManager(
    block_index_t blocks_start, block_index_t num_data_blocks, IBlockDevice& block_device)
    : _bitmap(block_device, blocks_start, num_data_blocks)
{
    _data_blocks_start = blocks_start + _bitmap.blocksSpanned();
    _num_data_blocks = num_data_blocks - _bitmap.blocksSpanned();
}

std::expected<void, BitmapError> BlockManager::format()
{
    if (auto ret = _bitmap.setAll(false); !ret.has_value()) {
        return std::unexpected(ret.error());
    }
    return {};
}

std::expected<void, BitmapError> BlockManager::reserve(block_index_t block)
{
    return _bitmap.setBit(block, true);
}

std::expected<void, BitmapError> BlockManager::free(block_index_t block)
{
    return _bitmap.setBit(block, false);
}

std::expected<block_index_t, BitmapError> BlockManager::getFree()
{
    return _bitmap.getFirstEq(false);
}

std::expected<unsigned int, BitmapError> BlockManager::countFree() { }

std::expected<unsigned int, BitmapError> BlockManager::numTotal() { return _num_data_blocks; }
