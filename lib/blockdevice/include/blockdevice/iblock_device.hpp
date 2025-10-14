#include <expected>
#include <vector>

#include "disk/idisk.hpp"

class IBlockDevice {
public:
    virtual ~IBlockDevice() = default;

    virtual std::expected<size_t, DiskError> writeBlock(
        const std::vector<std::byte>& data, size_t block_index, size_t offset) = 0;

    virtual std::expected<std::vector<std::byte>, DiskError> readBlock(
        size_t block_index, size_t offset, size_t bytes_to_read) = 0;

    virtual size_t rawBlockSize() const = 0;
    virtual size_t dataSize() const = 0;
};