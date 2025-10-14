#include "blockdevice/raw_block_device.hpp"

RawBlockDevice::RawBlockDevice(size_t block_size, IDisk* disk)
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

std::expected<size_t, DiskError> RawBlockDevice::writeBlock(
    const std::vector<std::byte>& data, size_t block_index, size_t offset)
{
    size_t to_write = std::min(data.size(), _block_size - offset);

    size_t address = block_index * _block_size + offset;
    auto disk_result = _disk->write(address, data);
    if (!disk_result.has_value()) {
        return std::unexpected(disk_result.error());
    }
    
    return to_write;
}

std::expected<std::vector<std::byte>, DiskError> RawBlockDevice::readBlock(
    size_t block_index, size_t offset, size_t bytes_to_read)
{
    size_t to_read = std::min(bytes_to_read, _block_size - offset);

    size_t address = block_index * _block_size + offset;
    return _disk->read(address, bytes_to_read);
}