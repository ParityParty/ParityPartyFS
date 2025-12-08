#include "blockdevice/crc_block_device.hpp"
#include "common/bit_helpers.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

std::expected<std::vector<std::uint8_t>, FsError> CrcBlockDevice::_readAndCheckRaw(
    block_index_t block_index)
{
    auto bytes_res = _disk.read(block_index * _block_size, _block_size);
    if (!bytes_res.has_value()) {
        return std::unexpected(bytes_res.error());
    }
    auto block = bytes_res.value();
    auto block_bits = BitHelpers::blockToBits(block);
    auto amount_unused_bits = (_block_size - dataSize()) * 8 - _polynomial.getDegree();
    for (int i = 0; i < amount_unused_bits; i++) {
        block_bits.pop_back();
    }

    auto remainder = _polynomial.divide(block_bits);

    // reminder should be 0
    if (std::ranges::contains(remainder.begin(), remainder.end(), true)) {
        return std::unexpected(FsError::BlockDevice_CorrectionError);
    }
    return block;
}

std::expected<void, FsError> CrcBlockDevice::_calculateAndWrite(
    std::vector<std::uint8_t>& block, block_index_t block_index)
{
    // Get data bits
    auto block_bits = BitHelpers::blockToBits({ block.begin(), block.begin() + dataSize() });
    block_bits.reserve(block_bits.size() + _polynomial.getDegree());

    // Add padding
    for (int i = 0; i < _polynomial.getDegree(); i++) {
        block_bits.push_back(false);
    }

    // Calculate redundancy bits
    auto remainder = _polynomial.divide(block_bits);
    for (int i = 0; i < _polynomial.getDegree(); i++) {
        BitHelpers::setBit(block, dataSize() * 8 + i, remainder[i]);
    }

    auto disk_res = _disk.write(_block_size * block_index, block);
    if (!disk_res.has_value()) {
        return std::unexpected(disk_res.error());
    }
    return {};
}

CrcBlockDevice::CrcBlockDevice(CrcPolynomial polynomial, IDisk& disk, size_t block_size)
    : _polynomial(std::move(polynomial))
    , _disk(disk)
    , _block_size(block_size)
{
}

std::expected<size_t, FsError> CrcBlockDevice::writeBlock(
    const std::vector<std::uint8_t>& data, DataLocation data_location)
{
    auto read_res = _readAndCheckRaw(data_location.block_index);
    if (!read_res.has_value()) {
        return std::unexpected(read_res.error());
    }
    auto block = read_res.value();
    size_t to_write = std::min(data.size(), dataSize() - data_location.offset);
    std::copy_n(data.begin(), to_write, block.begin() + data_location.offset);
    auto ret = _calculateAndWrite(block, data_location.block_index);
    if (!ret.has_value()) {
        return std::unexpected(ret.error());
    }
    return to_write;
}

std::expected<std::vector<std::uint8_t>, FsError> CrcBlockDevice::readBlock(
    DataLocation data_location, size_t bytes_to_read)
{
    auto read_ret = _readAndCheckRaw(data_location.block_index);
    if (!read_ret.has_value()) {
        return std::unexpected(read_ret.error());
    }
    auto block = read_ret.value();
    size_t to_read = std::min(bytes_to_read, dataSize() - data_location.offset);
    return std::vector(
        block.begin() + data_location.offset, block.begin() + data_location.offset + to_read);
}

size_t CrcBlockDevice::rawBlockSize() const { return _block_size; }

size_t CrcBlockDevice::dataSize() const
{
    return _block_size - std::ceil(static_cast<float>(_polynomial.getDegree()) / 8.0);
}

size_t CrcBlockDevice::numOfBlocks() const { return _disk.size() / _block_size; }

std::expected<void, FsError> CrcBlockDevice::formatBlock(unsigned int block_index)
{
    std::vector<std::uint8_t> data(_block_size, static_cast<std::uint8_t>(0x00));
    auto ret = _calculateAndWrite(data, block_index);
    if (!ret.has_value()) {
        return std::unexpected(ret.error());
    }
    return {};
}
