#include "bitmap/bitmap.hpp"
#include "blockdevice/"

#include <cmath>

DataLocation Bitmap::_getByteLocation(unsigned int bit_index)
{
    auto byte = bit_index / 8;
    block_index_t block = byte / _block_device.dataSize();
    auto offset = byte % _block_device.dataSize();
    return { block, offset };
}

std::expected<unsigned char, DiskError> Bitmap::_getByte(unsigned int bit_index)
{
    auto location = _getByteLocation(bit_index);
    auto read_ret = _block_device.readBlock(location, 1);
    if (!read_ret.has_value()) {
        return std::unexpected(read_ret.error());
    }
    return static_cast<unsigned char>(read_ret.value().front());
}

Bitmap::Bitmap(IBlockDevice& block_device, size_t start_block, size_t size)
    : _block_device(block_device)
    , _start_block(start_block)
    , _size(size)
{
}

std::expected<bool, DiskError> Bitmap::getBit(unsigned int bit_index)
{
    auto byte_ret = _getByte(bit_index);
    if (!byte_ret.has_value()) {
        return std::unexpected(byte_ret.error());
    }
    auto byte = byte_ret.value();

    auto bit = bit_index % 8;

    return ((byte >> (7 - bit)) & 1) > 0;
}
std::expected<void, DiskError> Bitmap::setBit(unsigned int bit_index, bool value)
{
    auto byte_ret = _getByte(bit_index);
    if (!byte_ret.has_value()) {
        return std::unexpected(byte_ret.error());
    }
    auto byte = byte_ret.value();
    auto bit = bit_index % 8;

    if (value) {
        byte = byte | (1 << (7 - bit));
    } else {
        byte = byte & ~(1 << (7 - bit));
    }

    auto location = _getByteLocation(bit_index);

    auto write_ret = _block_device.writeBlock({ static_cast<std::byte>(byte) }, location);
    if (write_ret.has_value()) {
        return {};
    };
    return std::unexpected(write_ret.error());
}
std::expected<unsigned int, DiskError> Bitmap::getFirstEq(bool value)
{
    int blocks_spanned = std::ceil(static_cast<float>(std::ceil(static_cast<float>(_size) / 8.0))
        / static_cast<float>(_block_device.dataSize()));

    for (int block = 0; block < blocks_spanned; block++) {
        auto block_ret = _block_device.readBlock(DataLocation(_start_block + block, 0), _block_device.dataSize());
        if (!block_ret.has_value()) {
            return std::unexpected(block_ret.error());
        }
        for (int i = 0; i < _block_device.dataSize(); i++) {
            BitHelpers::
        }

    }
}
