#include "ppfs/blockdevice/parity_block_device.hpp"
#include "ppfs/common/static_vector.hpp"
#include "ppfs/data_collection/data_colection.hpp"

#include <array>
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
    std::array<uint8_t, MAX_BLOCK_SIZE> zero_buffer;
    static_vector<uint8_t> zero_data(zero_buffer.data(), MAX_BLOCK_SIZE, _raw_block_size);
    std::fill(zero_data.begin(), zero_data.end(), 0);
    auto res = _disk.write(block_index * _raw_block_size, zero_data);
    return res.has_value() ? std::expected<void, FsError>() : std::unexpected(res.error());
}

std::expected<size_t, FsError> ParityBlockDevice::writeBlock(
    const static_vector<std::uint8_t>& data, DataLocation data_location)
{
    size_t to_write = std::min(data.size(), _data_size - data_location.offset);

    std::array<uint8_t, MAX_BLOCK_SIZE> raw_block_buffer;
    static_vector<uint8_t> raw_block(raw_block_buffer.data(), MAX_BLOCK_SIZE);
    raw_block.resize(_raw_block_size);
    auto read_res
        = _disk.read(data_location.block_index * _raw_block_size, _raw_block_size, raw_block);
    if (!read_res.has_value())
        return std::unexpected(read_res.error());

    bool parity = _checkParity(raw_block);
    if (!parity) {
        return std::unexpected(FsError::BlockDevice_CorrectionError);
    }

    std::copy_n(data.begin(), to_write, raw_block.begin() + data_location.offset);

    parity = _checkParity(raw_block);

    if (!parity)
        raw_block[_raw_block_size - 1] ^= static_cast<std::uint8_t>(1);

    auto written = _disk.write(_raw_block_size * data_location.block_index, raw_block);
    if (!written.has_value())
        return std::unexpected(written.error());

    return to_write;
}

std::expected<void, FsError> ParityBlockDevice::readBlock(
    DataLocation data_location, size_t to_read, static_vector<uint8_t>& data)
{
    data.resize(0);
    if (data.capacity() < to_read) {
        return std::unexpected(FsError::Disk_InvalidRequest);
    }
    to_read = std::min(to_read, _data_size - data_location.offset);

    std::array<uint8_t, MAX_BLOCK_SIZE> raw_block_buffer;
    static_vector<uint8_t> raw_block(raw_block_buffer.data(), MAX_BLOCK_SIZE);
    raw_block.resize(_raw_block_size);
    auto read_res
        = _disk.read(data_location.block_index * _raw_block_size, _raw_block_size, raw_block);
    if (!read_res.has_value())
        return std::unexpected(read_res.error());

    bool parity = _checkParity(raw_block);
    if (!parity) {
        return std::unexpected(FsError::BlockDevice_CorrectionError);
    }

    data.resize(to_read);
    std::copy_n(raw_block.begin() + data_location.offset, to_read, data.begin());
    return {};
}

bool ParityBlockDevice::_checkParity(const static_vector<std::uint8_t>& data)
{
    size_t ones = 0;
    for (auto b : data) {
        ones += std::popcount(b);
    }
    return !(ones & 1);
}
