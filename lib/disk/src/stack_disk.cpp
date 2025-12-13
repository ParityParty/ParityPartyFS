#include "disk/stack_disk.hpp"

#include <cstring>
#include <expected>

size_t StackDisk::size() { return _size; }

std::expected<void, FsError> StackDisk::read(
    size_t address, size_t size, static_vector<uint8_t>& data)
{
    if (address + size > _size) {
        return std::unexpected(FsError::Disk_OutOfBounds);
    }
    if (data.capacity() < size)
        return std::unexpected(FsError::Disk_InvalidRequest);

    data.resize(size);
    std::memcpy(data.data(), _data + address, data.size());
    return {};
}

std::expected<size_t, FsError> StackDisk::write(size_t address, const static_vector<uint8_t>& data)
{
    if (address + data.size() > _size) {
        return std::unexpected(FsError::Disk_OutOfBounds);
    }
    std::memcpy(_data + address, data.data(), data.size());
    return data.size();
}
