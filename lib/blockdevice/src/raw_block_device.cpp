#include "blockdevice/raw_block_device.hpp"
#include "static_vector.hpp"

RawBlockDevice::RawBlockDevice(size_t block_size, IDisk& disk)
    : _block_size(block_size)
    , _disk(disk)
{
}

size_t RawBlockDevice::rawBlockSize() const { return _block_size; }

size_t RawBlockDevice::dataSize() const { return _block_size; }

std::expected<size_t, FsError> RawBlockDevice::writeBlock(
    const buffer<std::uint8_t>& data, DataLocation data_location)
{
    size_t to_write = std::min(data.size(), _block_size - data_location.offset);

    size_t address = data_location.block_index * _block_size + data_location.offset;
    static_vector<uint8_t, MAX_BLOCK_SIZE> temp(data.begin(), data.begin() + to_write);
    auto disk_result = _disk.write(address, temp);
    if (!disk_result.has_value()) {
        return std::unexpected(disk_result.error());
    }

    return to_write;
}

std::expected<void, FsError> RawBlockDevice::readBlock(
    DataLocation data_location, size_t bytes_to_read, static_vector<uint8_t>& data)
{
    size_t to_read = std::min(bytes_to_read, _block_size - data_location.offset);

    size_t address = data_location.block_index * _block_size + data_location.offset;
    return _disk.read(address, to_read, data);
}

std::expected<void, FsError> RawBlockDevice::formatBlock(unsigned int block_index) { return {}; }

size_t RawBlockDevice::numOfBlocks() const { return _disk.size() / _block_size; }

DataLocation::DataLocation(int block_index, size_t offset)
    : block_index(block_index)
    , offset(offset)
{
}