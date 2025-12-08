#include "disk/stack_disk.hpp"

#include <cstring>
#include <expected>

size_t StackDisk::size() { return _size; }

std::expected<void, FsError> StackDisk::read(size_t address, size_t size, buffer<uint8_t>& data)
{
    if (address + size > _size) { // size_t is unsigned so we don't need to check lower bound
        return std::unexpected(FsError::OutOfBounds);
    }
    std::memcpy(data.data(), _data + address, data.size());
    return {};
}

std::expected<size_t, FsError> StackDisk::write(size_t address, const buffer<uint8_t>& data)
{
    if (address + data.size() > _size) {
        return std::unexpected(FsError::OutOfBounds);
    }
    std::memcpy(_data + address, data.data(), data.size());
    return data.size();
}
