#include "bitmap/bitmap.hpp"
#include "common/bit_helpers.hpp"

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
int Bitmap::blocksSpanned() const
{
    return std::ceil(std::ceil(static_cast<float>(_bit_count) / 8.0)
        / static_cast<float>(_block_device.dataSize()));
}

Bitmap::Bitmap(IBlockDevice& block_device, block_index_t start_block, size_t bit_count)
    : _block_device(block_device)
    , _start_block(start_block)
    , _bit_count(bit_count)
{
}

std::expected<std::uint32_t, FsError> Bitmap::count(bool value)
{
    if (_ones_count.has_value()) {
        if (value) {
            return _ones_count.value();
        }
        return _bit_count - _ones_count.value();
    }
    std::uint32_t count = 0;
    auto blocks_spanned = blocksSpanned();
    for (int block = 0; block < blocks_spanned - 1; block++) {
        auto block_data = _block_device.readBlock(
            DataLocation { _start_block + block, 0 }, _block_device.dataSize());
        if (!block_data.has_value()) {
            return std::unexpected(block_data.error());
        }
        for (auto byte : block_data.value()) {
            for (std::uint8_t bit = 0; bit < 8; bit += 1) {
                count += (byte & (1 << bit)) != 0;
            }
        }
    }

    auto last_block_ret = _block_device.readBlock(
        { _start_block + blocks_spanned - 1, 0 }, _block_device.dataSize());
    auto last_block = last_block_ret.value();
    if (!last_block_ret.has_value()) {
        return std::unexpected(last_block_ret.error());
    }

    auto bits_left = _bit_count % (_block_device.dataSize() * 8);
    if (bits_left == 0)
        bits_left = _block_device.dataSize() * 8;

    for (int bit_index = 0; bit_index < bits_left; bit_index++) {
        count += BitHelpers::getBit(last_block, bit_index);
    }
    _ones_count = count;
    if (value) {
        return count;
    }
    return _bit_count - count;
}

std::expected<bool, FsError> Bitmap::getBit(unsigned int bit_index)
{
    if (bit_index >= _bit_count) {
        return std::unexpected(FsError::Bitmap_IndexOutOfRange);
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
    if (bit_index >= _bit_count) {
        return std::unexpected(FsError::Bitmap_IndexOutOfRange);
    }
    auto byte_ret = _getByte(bit_index);
    if (!byte_ret.has_value()) {
        return std::unexpected(byte_ret.error());
    }
    auto old_byte = byte_ret.value();
    auto bit = bit_index % 8;

    std::uint8_t byte;
    if (value) {
        byte = old_byte | (1 << (7 - bit));
    } else {
        byte = old_byte & ~(1 << (7 - bit));
    }

    auto location = _getByteLocation(bit_index);

    auto write_ret = _block_device.writeBlock({ byte }, location);
    if (!write_ret.has_value()) {
        std::unexpected(write_ret.error());
    }

    if (_ones_count.has_value() && byte != old_byte) {
        if (value) {
            _ones_count.value()++;
        } else {
            _ones_count.value()--;
        }
    }

    return {};
}

std::expected<unsigned int, FsError> Bitmap::getFirstEq(bool value)
{
    int blocks_spanned = blocksSpanned();

    for (int block = 0; block < blocks_spanned; block++) {
        auto block_ret = _block_device.readBlock(
            DataLocation(_start_block + block, 0), _block_device.dataSize());
        if (!block_ret.has_value()) {
            return std::unexpected(block_ret.error());
        }
        const auto& block_data = block_ret.value();

        for (int i = 0; i < _block_device.dataSize() * 8; i++) {
            if (block * _block_device.dataSize() * 8 + i >= _bit_count) {
                // there is no more value in bitmap
                return std::unexpected(FsError::Bitmap_NotFound);
            }
            if (BitHelpers::getBit(block_data, i) == value) {
                return block * _block_device.dataSize() * 8 + i;
            }
        }
    }
    return std::unexpected(FsError::Bitmap_NotFound);
}

std::expected<void, FsError> Bitmap::setAll(bool value)
{
    auto blocks_spanned = blocksSpanned();
    std::uint8_t value_byte = value ? 0xff : 0x00;
    auto block_data = std::vector<std::uint8_t>(_block_device.dataSize(), value_byte);
    for (int block = 0; block < blocks_spanned; block++) {
        auto ret = _block_device.writeBlock(block_data, { _start_block + block, 0 });
        if (!ret.has_value()) {
            return std::unexpected(ret.error());
        }
    }

    _ones_count = value * _bit_count;
    return {};
}
