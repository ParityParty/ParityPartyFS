#include "ppfs/disk/heap_disk.hpp"

#include <cstring>

HeapDisk::HeapDisk(size_t size)
    : _size(size)
{
    _buffer = new uint8_t[_size];
}
HeapDisk::~HeapDisk() { delete[] _buffer; }
std::expected<void, FsError> HeapDisk::read(
    size_t address, size_t size, static_vector<uint8_t>& data)
{
    if (address > _size || address + size > _size) {
        return std::unexpected { FsError::Disk_OutOfBounds };
    }
    data.resize(size);
    std::memcpy(data.data(), _buffer + address, data.size());
    return {};
}
std::expected<size_t, FsError> HeapDisk::write(size_t address, const static_vector<uint8_t>& data)
{
    if (address > _size || address + data.size() > _size) {
        return std::unexpected { FsError::Disk_OutOfBounds };
    }

    std::memcpy(_buffer + address, data.data(), data.size());
    return {};
}

size_t HeapDisk::size() { return _size; }
