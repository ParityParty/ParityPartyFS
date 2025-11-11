#pragma once
#include "blockdevice/iblock_device.hpp"
#include "common/types.hpp"

enum BitmapError { IndexOutOfRange, Disk, NotFound };

/**
 * Class for working with bitmaps in disk
 */
class Bitmap {
    IBlockDevice& _block_device;
    block_index_t _start_block;
    size_t _size;

    DataLocation _getByteLocation(unsigned int bit_index);
    std::expected<unsigned char, DiskError> _getByte(unsigned int bit_index);

public:
    /**
     * @param block_device Block device on which bitmap is stored
     * @param start_block Starting block of the bitmap (bitmap always starts at the start of a
     * block)
     * @param size Number of bits stored in bitmap
     */
    Bitmap(IBlockDevice& block_device, block_index_t start_block, size_t size);
    std::expected<bool, BitmapError> getBit(unsigned int bit_index);
    std::expected<void, BitmapError> setBit(unsigned int bit_index, bool value);
    std::expected<unsigned int, BitmapError> getFirstEq(bool value);
};