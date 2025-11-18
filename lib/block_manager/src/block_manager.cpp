#include "block_manager/block_manager.hpp"

#include "bitmap/bitmap.hpp"

block_index_t BlockManager::_toRelative(block_index_t absolute_block) const
{
    return absolute_block - _data_blocks_start;
}

block_index_t BlockManager::_toAbsolute(block_index_t relative_block) const
{
    return relative_block + _data_blocks_start;
}

BlockManager::BlockManager(
    block_index_t blocks_start, block_index_t space_for_data_and_bitmap, IBlockDevice& block_device)
    : _bitmap(block_device, blocks_start,
          (8 * block_device.dataSize() * space_for_data_and_bitmap)
              / (1 + 8 * block_device.dataSize()))
{
    _data_blocks_start = blocks_start + _bitmap.blocksSpanned();
    _num_data_blocks = space_for_data_and_bitmap - _bitmap.blocksSpanned();
}

std::expected<void, FsError> BlockManager::format()
{
    if (auto ret = _bitmap.setAll(false); !ret.has_value()) {
        return std::unexpected(ret.error());
    }
    return {};
}

std::expected<void, FsError> BlockManager::reserve(block_index_t block)
{
    block = _toRelative(block);
    auto read_ret = _bitmap.getBit(block);
    if (!read_ret.has_value()) {
        return std::unexpected(read_ret.error());
    }
    auto prev_value = read_ret.value();
    if (prev_value == true) {
        return std::unexpected(FsError::AlreadyTaken);
    }
    auto write_ret = _bitmap.setBit(block, true);
    if (!write_ret.has_value()) {
        return std::unexpected(write_ret.error());
    }
    return {};
}

std::expected<void, FsError> BlockManager::free(block_index_t block)
{
    block = _toRelative(block);
    auto read_ret = _bitmap.getBit(block);
    if (!read_ret.has_value()) {
        return std::unexpected(read_ret.error());
    }
    auto prev_value = read_ret.value();
    if (prev_value == false) {
        return std::unexpected(FsError::AlreadyFree);
    }
    auto write_ret = _bitmap.setBit(block, false);
    if (!write_ret.has_value()) {
        return std::unexpected(write_ret.error());
    }

    return {};
}

std::expected<block_index_t, FsError> BlockManager::getFree()
{
    auto get_ret = _bitmap.getFirstEq(false);
    if (!get_ret.has_value()) {
        return std::unexpected(get_ret.error());
    }
    return _toAbsolute(get_ret.value());
}

std::expected<std::uint32_t, FsError> BlockManager::numFree() { return _bitmap.count(false); }

std::expected<unsigned int, FsError> BlockManager::numTotal() { return _num_data_blocks; }
