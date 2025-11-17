#include "block_manager/block_manager.hpp"

#include "bitmap/bitmap.hpp"

BlockManager::BlockManager(
    block_index_t blocks_start, block_index_t space_for_data_blocks, IBlockDevice& block_device)
    : _bitmap(block_device, blocks_start, space_for_data_blocks)
    , _num_free_blocks(std::nullopt)
{
    _data_blocks_start = blocks_start + _bitmap.blocksSpanned();
    _num_data_blocks = space_for_data_blocks - _bitmap.blocksSpanned();
}

std::expected<void, FsError> BlockManager::format()
{
    if (auto ret = _bitmap.setAll(false); !ret.has_value()) {
        return std::unexpected(ret.error());
    }
    _num_free_blocks = _num_data_blocks;
    return {};
}

std::expected<void, FsError> BlockManager::reserve(block_index_t block)
{
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
    if (_num_free_blocks.has_value()) {
        _num_free_blocks = _num_free_blocks.value() - 1;
    }

    auto num_free_ret = numFree();
    if (!num_free_ret.has_value()) {
        return std::unexpected(num_free_ret.error());
    }
    _num_free_blocks = num_free_ret.value();
    return {};
}

std::expected<void, FsError> BlockManager::free(block_index_t block)
{
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

    if (_num_free_blocks.has_value()) {
        _num_free_blocks = _num_free_blocks.value() + 1;
    }

    auto num_free_ret = numFree();
    if (!num_free_ret.has_value()) {
        return std::unexpected(num_free_ret.error());
    }
    _num_free_blocks = num_free_ret.value();
    return {};
}

std::expected<block_index_t, FsError> BlockManager::getFree() { return _bitmap.getFirstEq(false); }

std::expected<unsigned int, FsError> BlockManager::numFree()
{
    if (_num_free_blocks.has_value()) {
        return _num_free_blocks.value();
    }
    auto ret = _bitmap.count(false);
    if (!ret.has_value()) {
        return std::unexpected(ret.error());
    }
    _num_free_blocks = ret.value();
    return _num_free_blocks.value();
}

std::expected<unsigned int, FsError> BlockManager::numTotal() { return _num_data_blocks; }
