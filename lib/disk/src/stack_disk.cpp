#include "disk/stack_disk.hpp"

#include <cstring>
#include <expected>

size_t StackDisk::size() { return _size;}

std::expected<std::vector<std::byte>, DiskError> StackDisk::read(size_t address, size_t size)
{
    if (address + size > _size) { // size_t is unsigned so we don't need to check lower bound
        return std::unexpected(DiskError::OutOfBounds);
    }
    return std::vector(_data + address, _data + address + size);
}

std::expected<size_t, DiskError> StackDisk::write(size_t address, const std::vector<std::byte>& data)
{
    if (address + data.size() > _size) {
        return std::unexpected(DiskError::OutOfBounds);
    }
    std::memcpy(_data + address, data.data(), data.size());
    return data.size();
}


