#pragma once
#include "blockdevice/iblock_device.hpp"
#include "common/types.hpp"

class Bitmap {
    IBlockDevice& _block_device;
    block_index_t _start_block;
    size_t _size;

public:
    Bitmap(IBlockDevice& block_device, size_t start_block, size_t size);
};