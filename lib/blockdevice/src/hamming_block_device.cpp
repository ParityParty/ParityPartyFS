#include "blockdevice/hamming_block_device.hpp"
#include "common/bit_helpers.hpp"
#include "data_collection/data_colection.hpp"

#include <cmath>
#include <cstdint>
#include <memory>

HammingBlockDevice::HammingBlockDevice(
    int block_size_power, IDisk& disk, std::shared_ptr<Logger> logger)
    : _disk(disk)
    , _logger(logger)
{
    _block_size = 1 << block_size_power;
    int parity_bytes = (int)std::ceil(((float)(block_size_power * 3 + 1)) / 8.0);
    _data_size = _block_size - parity_bytes;
}

std::expected<std::vector<std::uint8_t>, FsError> HammingBlockDevice::_readAndFixBlock(
    int block_index)
{
    auto read_result = _disk.read(block_index * _block_size, _block_size);
    if (!read_result.has_value()) {
        return std::unexpected(read_result.error());
    }

    auto encoded_data = read_result.value();

    unsigned int error_position = 0;
    bool parity = true;

    HammingUsedBitsIterator it(_block_size, _data_size);
    while (auto index = it.next()) {
        if (BitHelpers::getBit(encoded_data, *index)) {
            error_position ^= *index;
            parity = !parity;
        }
    }

    if (!parity) {
        bool flipped_bit_value = !BitHelpers::getBit(encoded_data, error_position);

        BitHelpers::setBit(encoded_data, error_position, flipped_bit_value);
        auto disk_result = _disk.write(block_index * _block_size + error_position / 8,
            { encoded_data.begin() + error_position / 8,
                encoded_data.begin() + error_position / 8 + 1 });
        if (!disk_result.has_value()) {
            return std::unexpected(disk_result.error());
        }

        // Log error correction
        if (_logger) {
            ErrorCorrectionEvent event("Hamming", block_index);
            _logger->logEvent(event);
        }
    } else {
        if (error_position != 0) {
            // Log error detection (uncorrectable)
            if (_logger) {
                ErrorDetectionEvent event("Hamming", block_index);
                _logger->logEvent(event);
            }
            return std::unexpected(FsError::BlockDevice_CorrectionError);
        }
    }

    return encoded_data;
}

std::vector<std::uint8_t> HammingBlockDevice::_extractData(
    const std::vector<std::uint8_t>& encoded_data)
{
    std::vector<std::uint8_t> data(_data_size, std::uint8_t(0));
    HammingDataBitsIterator it(_block_size, _data_size);
    for (unsigned int i = 0; i < _data_size * 8; i++)
        BitHelpers::setBit(data, i, BitHelpers::getBit(encoded_data, *it.next()));
    return data;
}

std::vector<std::uint8_t> HammingBlockDevice::_encodeData(const std::vector<std::uint8_t>& data)
{
    std::vector<std::uint8_t> encoded_data(_block_size, std::uint8_t(0));

    bool parity = true;

    unsigned int parity_xor = 0;

    HammingDataBitsIterator it(_block_size, _data_size);
    for (unsigned int i = 0; i < _data_size * 8; i++) {
        bool bit_value = BitHelpers::getBit(data, i);
        int raw_index = *it.next();
        if (bit_value) {
            parity = !parity;
            parity_xor ^= raw_index;
        }
        BitHelpers::setBit(encoded_data, raw_index, bit_value);
    }

    unsigned int parity_index = 1;
    while (parity_index < _block_size * 8) {
        bool parity_bit_value = false;
        if (parity_xor & parity_index) {
            parity = !parity;
            parity_bit_value = true;
        }
        BitHelpers::setBit(encoded_data, parity_index, parity_bit_value);
        parity_index <<= 1;
    }

    bool parity_bit = !parity;
    BitHelpers::setBit(encoded_data, 0, parity_bit);

    return encoded_data;
}

std::expected<size_t, FsError> HammingBlockDevice::writeBlock(
    const std::vector<std::uint8_t>& data, DataLocation data_location)
{
    size_t to_write = std::min(data.size(), _data_size - data_location.offset);

    auto raw_block = _readAndFixBlock(data_location.block_index);
    if (!raw_block.has_value()) {
        return std::unexpected(raw_block.error());
    }
    auto decoded_data = _extractData(raw_block.value());
    std::copy(data.begin(), data.begin() + to_write, decoded_data.begin() + data_location.offset);

    auto new_encoded_block = _encodeData(decoded_data);

    auto disk_result = _disk.write(data_location.block_index * _block_size, new_encoded_block);

    if (!disk_result.has_value()) {
        return std::unexpected(disk_result.error());
    }

    return to_write;
}

std::expected<std::vector<std::uint8_t>, FsError> HammingBlockDevice::readBlock(
    DataLocation data_location, size_t bytes_to_read)
{
    bytes_to_read = std::min(_data_size - data_location.offset, bytes_to_read);

    auto raw_block = _readAndFixBlock(data_location.block_index);
    if (!raw_block.has_value()) {
        return std::unexpected(raw_block.error());
    }

    auto decoded_data = _extractData(raw_block.value());

    return std::vector<std::uint8_t>(decoded_data.begin() + data_location.offset,
        decoded_data.begin() + data_location.offset + bytes_to_read);
}

std::expected<void, FsError> HammingBlockDevice::formatBlock(unsigned int block_index)
{
    std::vector<std::uint8_t> zero_data(_block_size, std::uint8_t(0));
    auto write_result = _disk.write(block_index * _block_size, zero_data);
    return write_result.has_value() ? std::expected<void, FsError> {}
                                    : std::unexpected(write_result.error());
}

size_t HammingBlockDevice::rawBlockSize() const { return _block_size; }

size_t HammingBlockDevice::dataSize() const { return _data_size; }

size_t HammingBlockDevice::numOfBlocks() const { return _disk.size() / _block_size; }

HammingDataBitsIterator::HammingDataBitsIterator(int block_size, int data_size)
    : _block_size(block_size)
    , _data_size(data_size)
    , _current_index(0)
    , _data_bits_returned(0)
{
}

std::optional<unsigned int> HammingDataBitsIterator::next()
{
    if (_data_bits_returned >= _data_size * 8) {
        return {}; // No more data bits
    }
    while ((_current_index & (_current_index - 1)) == 0) {
        _current_index++;
    }
    _data_bits_returned++;
    return _current_index++;
}

HammingUsedBitsIterator::HammingUsedBitsIterator(int block_size, int data_size)
    : _block_size(block_size)
    , _data_size(data_size)
    , _current_index(0)
    , _data_bits_returned(0)
    , _next_parity_bit(1)
{
}

std::optional<unsigned int> HammingUsedBitsIterator::next()
{
    if (_data_bits_returned >= _data_size * 8 && _next_parity_bit >= _block_size * 8) {
        return {}; // No more data bytes
    }

    if (_data_bits_returned >= _data_size * 8) {
        _current_index = _next_parity_bit;
        _next_parity_bit <<= 1;
        return _current_index;
    }

    if ((_current_index && (_current_index & (_current_index - 1)) == 0)) {
        _next_parity_bit = _current_index << 1;
    }

    if (_current_index && (_current_index & (_current_index - 1)) != 0) {
        _data_bits_returned++;
    }

    return _current_index++;
}
