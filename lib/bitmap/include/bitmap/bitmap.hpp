#pragma once
#include "blockdevice/iblock_device.hpp"
#include "common/types.hpp"

#include <cstdint>
#include <optional>

enum BitmapError { IndexOutOfRange, Disk, NotFound };

/**
 * Class for working with bitmaps in disk
 */
class Bitmap {
    IBlockDevice& _block_device;
    block_index_t _start_block;
    size_t _bit_count;
    std::optional<std::uint32_t> _ones_count;

    DataLocation _getByteLocation(unsigned int bit_index);
    std::expected<unsigned char, FsError> _getByte(unsigned int bit_index);
    int _blockSpanned() const;

public:
    /**
     * @param block_device Block device on which bitmap is stored
     * @param start_block Starting block of the bitmap (bitmap always starts at the start of a
     * block)
     * @param bit_count Number of bits stored in bitmap
     */
    Bitmap(IBlockDevice& block_device, block_index_t start_block, size_t bit_count);
    std::expected<std::uint32_t, FsError> count(bool value);
    std::expected<bool, FsError> getBit(unsigned int bit_index);
    std::expected<void, FsError> setBit(unsigned int bit_index, bool value);
    std::expected<unsigned int, FsError> getFirstEq(bool value);
    std::expected<void, FsError> setAll(bool value);
};