#include "blockdevice/raw_block_device.hpp"

/// @brief Constructs a RawBlockDevice instance.
/// @param block_size The size of a single block in bytes.
/// @param disk Reference to a disk object used for low-level read/write operations.
RawBlockDevice::RawBlockDevice(size_t block_size, IDisk& disk)
    : _block_size(block_size), _disk(disk)
{
}

/// @brief Returns the raw block size, all metadata included.
/// @return The size of one block in bytes.
size_t RawBlockDevice::rawBlockSize() const
{
    return _block_size;
}

/// @brief Returns the data size per block.
/// @return The size of data that fits in a single block (equal to the block size here).
size_t RawBlockDevice::dataSize() const
{
    return _block_size;
}

/// @brief Writes a portion of data into a block on the disk.
/// If the data exceeds the block size, it will be truncated.
/// @param data The buffer containing data to be written.
/// @param data_location The block index and offset specifying where to write the data.
/// @return On success, returns the number of bytes written; otherwise returns a DiskError.
std::expected<size_t, DiskError> RawBlockDevice::writeBlock(
    const std::vector<std::byte>& data, DataLocation data_location)
{
    size_t to_write = std::min(data.size(), _block_size - data_location.offset);

    size_t address = data_location.block_index * _block_size + data_location.offset;
    auto disk_result = _disk.write(address, data);
    if (!disk_result.has_value()) {
        return std::unexpected(disk_result.error());
    }
    
    return to_write;
}

/// @brief Reads a portion of data from a block on the disk.
/// If the requested bytes exceed the block size, they will be truncated.
/// @param data_location The block index and offset specifying where to start reading.
/// @param bytes_to_read The number of bytes requested to be read.
/// @return On success, returns a vector of bytes read; otherwise returns a DiskError.
std::expected<std::vector<std::byte>, DiskError> RawBlockDevice::readBlock(
    DataLocation data_location, size_t bytes_to_read)
{
    size_t to_read = std::min(bytes_to_read, _block_size - data_location.offset);

    size_t address = data_location.block_index * _block_size + data_location.offset;
    return _disk.read(address, bytes_to_read);
}

/// @brief Constructs a DataLocation object representing a position on the disk.
/// @param block_index The index of the block on the disk.
/// @param offset The offset within the block where data starts.
DataLocation::DataLocation(int block_index, size_t offset)
    : block_index(block_index)
    , offset(offset)
{
}