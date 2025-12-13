#include "blockdevice/crc_block_device.hpp"
#include "common/bit_helpers.hpp"
#include "common/static_vector.hpp"
#include "data_collection/data_colection.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

std::expected<void, FsError> CrcBlockDevice::_readAndCheckRaw(
    block_index_t block_index, static_vector<std::uint8_t>& block_buffer)
{
    block_buffer.resize(_block_size);
    auto bytes_res = _disk.read(block_index * _block_size, _block_size, block_buffer);
    if (!bytes_res.has_value()) {
        return std::unexpected(bytes_res.error());
    }
    std::array<bool, MAX_BLOCK_SIZE * 8> block_bits_buffer;
    static_vector<bool> block_bits(block_bits_buffer.data(), MAX_BLOCK_SIZE * 8);
    BitHelpers::blockToBits(block_buffer, block_bits);
    auto amount_unused_bits = (_block_size - dataSize()) * 8 - _polynomial.getDegree();
    block_bits.resize(block_bits.size() - amount_unused_bits);

    // Convert to std::vector<bool> for divide() call
    std::vector<bool> block_bits_vec(block_bits.begin(), block_bits.end());
    auto remainder = _polynomial.divide(block_bits_vec);

    // reminder should be 0
    if (std::ranges::contains(remainder.begin(), remainder.end(), true)) {
        if (_logger) {
            _logger->logEvent(ErrorDetectionEvent("CRC", block_index));
        }
        return std::unexpected(FsError::BlockDevice_CorrectionError);
    }
    return {};
}

std::expected<void, FsError> CrcBlockDevice::_calculateAndWrite(
    static_vector<std::uint8_t>& block, block_index_t block_index)
{
    // Get data bits - only process the data portion, not the redundancy area
    static_vector<uint8_t> data_view(block.data(), dataSize(), dataSize());
    std::array<bool, MAX_BLOCK_SIZE * 8> block_bits_buffer;
    static_vector<bool> block_bits(block_bits_buffer.data(), MAX_BLOCK_SIZE * 8);
    BitHelpers::blockToBits(data_view, block_bits);
    
    // Add padding
    size_t original_size = block_bits.size();
    block_bits.resize(original_size + _polynomial.getDegree());
    for (size_t i = original_size; i < block_bits.size(); i++) {
        block_bits[i] = false;
    }

    // Convert to std::vector<bool> for divide() call
    std::vector<bool> block_bits_vec(block_bits.begin(), block_bits.end());
    auto remainder = _polynomial.divide(block_bits_vec);
    for (int i = 0; i < _polynomial.getDegree(); i++) {
        BitHelpers::setBit(block, dataSize() * 8 + i, remainder[i]);
    }

    // Ensure block is the correct size before writing
    block.resize(_block_size);
    auto disk_res = _disk.write(_block_size * block_index, block);
    if (!disk_res.has_value()) {
        return std::unexpected(disk_res.error());
    }
    return {};
}

CrcBlockDevice::CrcBlockDevice(
    CrcPolynomial polynomial, IDisk& disk, size_t block_size, std::shared_ptr<Logger> logger)
    : _polynomial(std::move(polynomial))
    , _disk(disk)
    , _block_size(block_size)
    , _logger(logger)
{
}

std::expected<size_t, FsError> CrcBlockDevice::writeBlock(
    const static_vector<std::uint8_t>& data, DataLocation data_location)
{
    std::array<uint8_t, MAX_BLOCK_SIZE> block_buffer;
    static_vector<uint8_t> block(block_buffer.data(), MAX_BLOCK_SIZE);
    auto read_res = _readAndCheckRaw(data_location.block_index, block);
    if (!read_res.has_value()) {
        return std::unexpected(read_res.error());
    }
    size_t to_write = std::min(data.size(), dataSize() - data_location.offset);
    std::copy_n(data.begin(), to_write, block.begin() + data_location.offset);
    auto ret = _calculateAndWrite(block, data_location.block_index);
    if (!ret.has_value()) {
        return std::unexpected(ret.error());
    }
    return to_write;
}

std::expected<void, FsError> CrcBlockDevice::readBlock(
    DataLocation data_location, size_t bytes_to_read, static_vector<uint8_t>& data)
{
    std::array<uint8_t, MAX_BLOCK_SIZE> block_buffer;
    static_vector<uint8_t> block(block_buffer.data(), MAX_BLOCK_SIZE);
    auto read_ret = _readAndCheckRaw(data_location.block_index, block);
    if (!read_ret.has_value()) {
        return std::unexpected(read_ret.error());
    }
    size_t to_read = std::min(bytes_to_read, dataSize() - data_location.offset);
    data.resize(to_read);
    std::copy_n(block.begin() + data_location.offset, to_read, data.begin());
    return {};
}

size_t CrcBlockDevice::rawBlockSize() const { return _block_size; }

size_t CrcBlockDevice::dataSize() const
{
    return _block_size - std::ceil(static_cast<float>(_polynomial.getDegree()) / 8.0);
}

size_t CrcBlockDevice::numOfBlocks() const { return _disk.size() / _block_size; }

std::expected<void, FsError> CrcBlockDevice::formatBlock(unsigned int block_index)
{
    std::array<uint8_t, MAX_BLOCK_SIZE> block_buffer;
    static_vector<uint8_t> data(block_buffer.data(), MAX_BLOCK_SIZE, _block_size);
    std::fill(data.begin(), data.end(), static_cast<std::uint8_t>(0x00));
    auto ret = _calculateAndWrite(data, block_index);
    if (!ret.has_value()) {
        return std::unexpected(ret.error());
    }
    return {};
}
