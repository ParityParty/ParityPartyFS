#include "bitmap/bitmap.hpp"

Bitmap::Bitmap(IBlockDevice& block_device, size_t start_block, size_t size)
    : _block_device(block_device)
    , _start_block(start_block)
    , _size(size)
{
}
