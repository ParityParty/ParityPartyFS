#include "disk/stack_disk.hpp"

#include <cstring>
#include <expected>
#include <iostream>

size_t StackDisk::size() { return _size; }

std::expected<std::vector<std::uint8_t>, FsError> StackDisk::read(size_t address, size_t size)
{
    if (address + size > _size) { // size_t is unsigned so we don't need to check lower bound
        return std::unexpected(FsError::OutOfBounds);
    }
    return std::vector(_data + address, _data + address + size);
}

std::expected<size_t, FsError> StackDisk::write(
    size_t address, const std::vector<std::uint8_t>& data)
{
    if (address + data.size() > _size) {
        std::cout << "Whyyy" << std::endl;
        return std::unexpected(FsError::OutOfBounds);
    }
    std::memcpy(_data + address, data.data(), data.size());
    return data.size();
}
