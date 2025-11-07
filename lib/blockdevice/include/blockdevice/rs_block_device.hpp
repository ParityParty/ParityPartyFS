#include "blockdevice/iblock_device.hpp"

#define BLOCK_LENGTH 255
#define MESSAGE_LENGTH 251

class ReedSolomonBlockDevice : public IBlockDevice {
public:
    virtual std::expected<size_t, DiskError> writeBlock(
        const std::vector<std::byte>& data, DataLocation data_location) = 0;
    
    virtual std::expected<std::vector<std::byte>, DiskError> readBlock(
        DataLocation data_location, size_t bytes_to_read) = 0;

    virtual size_t rawBlockSize() const = 0;

    virtual size_t dataSize() const = 0;

    virtual size_t numOfBlocks() const = 0;

    virtual std::expected<void, DiskError> formatBlock(unsigned int block_index) = 0;
};