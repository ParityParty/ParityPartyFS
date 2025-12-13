#include "blockdevice/parity_block_device.hpp"
#include "data_collection/data_colection.hpp"

#include <cstdint>
#include <memory>

ParityBlockDevice::ParityBlockDevice(int block_size, IDisk& disk, std::shared_ptr<Logger> logger)
    : _raw_block_size(block_size)
    , _data_size(block_size - 1)
    , _disk(disk)
    , _logger(logger)
{
}

size_t ParityBlockDevice::dataSize() const { return _data_size; }

size_t ParityBlockDevice::rawBlockSize() const { return _raw_block_size; }

size_t ParityBlockDevice::numOfBlocks() const { return _disk.size() / _raw_block_size; }
std::expected<void, FsError> ParityBlockDevice::formatBlock(unsigned int block_index)
{
    auto res = _disk.write(block_index, std::vector<std::uint8_t>(_raw_block_size, 0));
    return res.has_value() ? std::expected<void, FsError>() : std::unexpected(res.error());
}

std::expected<size_t, FsError> ParityBlockDevice::writeBlock(
    const std::vector<std::uint8_t>& data, DataLocation data_location)
{
    size_t to_write = std::min(data.size(), _data_size - data_location.offset);

    auto raw_block = _disk.read(data_location.block_index, _raw_block_size);
    if (!raw_block.has_value())
        return std::unexpected(raw_block.error());

    bool parity = _checkParity(raw_block.value());
    if (!parity) {
        if (_logger) {
            _logger->logEvent(ErrorDetectionEvent("Parity", data_location.block_index));
        }
        return std::unexpected(FsError::BlockDevice_CorrectionError);
    }

    std::copy(
        data.begin(), data.begin() + to_write, raw_block.value().begin() + data_location.offset);

    parity = _checkParity(raw_block.value());

    if (!parity)
        raw_block.value()[_raw_block_size - 1] ^= static_cast<std::uint8_t>(1);

    return _disk.write(_raw_block_size * data_location.block_index, raw_block.value());
}

std::expected<void, FsError> ParityBlockDevice::readBlock(
    DataLocation data_location, size_t to_read, static_vector<uint8_t>& data)
{
    to_read = std::min(to_read, _data_size - data_location.offset);

    auto raw_block = _disk.read(data_location.block_index, _raw_block_size);
    if (!raw_block.has_value())
        return std::unexpected(raw_block.error());

    bool parity = _checkParity(raw_block.value());
    if (!parity) {
        if (_logger) {
            _logger->logEvent(ErrorDetectionEvent("Parity", data_location.block_index));
        }
        return std::unexpected(FsError::BlockDevice_CorrectionError);
    }

    auto data = raw_block.value();
    return std::vector<std::uint8_t>(
        data.begin() + data_location.offset, data.begin() + data_location.offset + to_read);
}

bool ParityBlockDevice::_checkParity(std::vector<std::uint8_t> data)
{
    size_t ones = 0;
    for (auto b : data) {
        ones += std::popcount(b);
    }
    return !(ones & 1);
}
