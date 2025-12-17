#include "disk/heap_disk.hpp"

#include <cstring>

HeapDisk::HeapDisk(size_t size)
    : _size(size)
{
    _buffer = new uint8_t[_size];
}
HeapDisk::~HeapDisk() { delete[] _buffer; }

std::expected<std::vector<std::uint8_t>, FsError> HeapDisk::read(size_t address, size_t size)
{
    if (address + size > _size) { // size_t is unsigned so we don't need to check lower bound
        return std::unexpected(FsError::Disk_OutOfBounds);
    }
    return std::vector(_buffer + address, _buffer + address + size);
}

std::expected<size_t, FsError> HeapDisk::write(
    size_t address, const std::vector<std::uint8_t>& data)
{
    if (address + data.size() > _size) {
        return std::unexpected(FsError::Disk_OutOfBounds);
    }
    std::memcpy(_buffer + address, data.data(), data.size());
    return data.size();
}
size_t HeapDisk::size() { return _size; }
