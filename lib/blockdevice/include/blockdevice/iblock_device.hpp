#pragma once
#include <expected>
#include <vector>

#include "disk/idisk.hpp"

struct DataLocation {
    int block_index;
    size_t offset;

    DataLocation(int block_index, size_t offset);
    DataLocation() = default;
};

class IBlockDevice {
public:
    virtual ~IBlockDevice() = default;

    virtual std::expected<size_t, DiskError> writeBlock(
        const std::vector<std::byte>& data, DataLocation data_location) = 0;

    virtual std::expected<std::vector<std::byte>, DiskError> readBlock(
        DataLocation data_location, size_t bytes_to_read) = 0;

    virtual size_t rawBlockSize() const = 0;
    virtual size_t dataSize() const = 0;
};