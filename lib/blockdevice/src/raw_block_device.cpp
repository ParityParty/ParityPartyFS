#include "blockdevice/raw_block_device.hpp"

RawBlockDevice::RawBlockDevice(size_t block_size, IDisk& disk)
    : _block_size(block_size), _disk(disk)
{
}

size_t RawBlockDevice::rawBlockSize() const
{
    return _block_size;
}

size_t RawBlockDevice::dataSize() const
{
    return _block_size;
}

std::expected<size_t, BlockDeviceError> RawBlockDevice::writeBlock(
    const std::vector<std::byte>& data, DataLocation data_location)
{
    size_t to_write = std::min(data.size(), _block_size - data_location.offset);

    size_t address = data_location.block_index * _block_size + data_location.offset;
    auto disk_result = _disk.write(address, {data.begin(), data.begin() + to_write});
    if (!disk_result.has_value()) {
        return std::unexpected(BlockDeviceError::DiskError);
    }
    
    return to_write;
}

std::expected<std::vector<std::byte>, BlockDeviceError> RawBlockDevice::readBlock(
    DataLocation data_location, size_t bytes_to_read)
{
    size_t to_read = std::min(bytes_to_read, _block_size - data_location.offset);

    size_t address = data_location.block_index * _block_size + data_location.offset;
    auto result = _disk.read(address, to_read);
    if (!result.has_value()) {
        return std::unexpected(BlockDeviceError::DiskError);
    }
    return result.value();
}

size_t RawBlockDevice::numOfBlocks() const {
    return _disk.size() / _block_size;
}

DataLocation::DataLocation(int block_index, size_t offset)
    : block_index(block_index)
    , offset(offset)
{
}