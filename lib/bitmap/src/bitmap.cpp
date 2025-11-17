#include "bitmap/bitmap.hpp"
#include "blockdevice/bit_helpers.hpp"

#include <cmath>

DataLocation Bitmap::_getByteLocation(unsigned int bit_index)
{
    auto byte = bit_index / 8;
    block_index_t block = byte / _block_device.dataSize();
    auto offset = byte % _block_device.dataSize();
    return { _start_block + block, offset };
}

std::expected<unsigned char, FsError> Bitmap::_getByte(unsigned int bit_index)
{
    auto location = _getByteLocation(bit_index);
    auto read_ret = _block_device.readBlock(location, 1);
    if (!read_ret.has_value()) {
        return std::unexpected(read_ret.error());
    }
    return static_cast<unsigned char>(read_ret.value().front());
}
int Bitmap::_blockSpanned() const
{
    return std::ceil(
        std::ceil(static_cast<float>(_size) / 8.0) / static_cast<float>(_block_device.dataSize()));
}

Bitmap::Bitmap(IBlockDevice& block_device, block_index_t start_block, size_t size)
    : _block_device(block_device)
    , _start_block(start_block)
    , _size(size)
{
}

std::expected<bool, FsError> Bitmap::getBit(unsigned int bit_index)
{
    if (bit_index >= _size) {
        return std::unexpected(FsError::IndexOutOfRange);
    }
    auto byte_ret = _getByte(bit_index);
    if (!byte_ret.has_value()) {
        return std::unexpected(byte_ret.error());
    }
    auto byte = byte_ret.value();

    auto bit = bit_index % 8;

    return ((byte >> (7 - bit)) & 1) > 0;
}
std::expected<void, FsError> Bitmap::setBit(unsigned int bit_index, bool value)
{
    if (bit_index >= _size) {
        return std::unexpected(FsError::IndexOutOfRange);
    }
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
std::expected<unsigned int, FsError> Bitmap::getFirstEq(bool value)
{
    int blocks_spanned = _blockSpanned();

    for (int block = 0; block < blocks_spanned; block++) {
        auto block_ret = _block_device.readBlock(
            DataLocation(_start_block + block, 0), _block_device.dataSize());
        if (!block_ret.has_value()) {
            return std::unexpected(block_ret.error());
        }
        const auto& block_data = block_ret.value();

        for (int i = 0; i < _block_device.dataSize() * 8; i++) {
            if (block * _block_device.dataSize() + i >= _size) {
                // there is no more value in bitmap
                return std::unexpected(FsError::NotFound);
            }
            if (BitHelpers::getBit(block_data, i) == value) {
                return block * _block_device.dataSize() + i;
            }
        }
    }
    return std::unexpected(FsError::NotFound);
}
std::expected<void, FsError> Bitmap::setAll(bool value)
{
    auto blocks_spanned = _blockSpanned();
    auto value_byte = value ? std::byte { 0xff } : std::byte { 0x00 };
    std::vector<std::byte> block_data { _block_device.dataSize(), value_byte };
    for (int block = 0; block < blocks_spanned - 1; block++) {
        auto ret = _block_device.writeBlock(block_data, { _start_block + block, 0 });
        if (!ret.has_value()) {
            return std::unexpected(ret.error());
        }
    }

    auto last_block_ret = _block_device.readBlock(
        { _start_block + blocks_spanned - 1, 0 }, _block_device.dataSize());
    auto last_block = last_block_ret.value();
    if (!last_block_ret.has_value()) {
        return std::unexpected(last_block_ret.error());
    }
    for (int bit_index = 0; bit_index < _size % (_block_device.dataSize() * 8); bit_index++) {
        BitHelpers::setBit(last_block, bit_index, value);
    }

    auto write_ret = _block_device.writeBlock(last_block, { _start_block + blocks_spanned - 1, 0 });
    if (!write_ret.has_value()) {
        return std::unexpected(write_ret.error());
    }
    return {};
}
