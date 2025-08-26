#include "disk/stack_disk.hpp"
#include <expected>

std::expected<std::vector<std::byte>, DiskError> StackDisk::read(size_t address, size_t size)
{
    return std::unexpected(DiskError::InternalError);
}

std::expected<size_t, DiskError> StackDisk::write(size_t address, const std::vector<std::byte>& data)
{
    return std::unexpected(DiskError::InternalError);
}


