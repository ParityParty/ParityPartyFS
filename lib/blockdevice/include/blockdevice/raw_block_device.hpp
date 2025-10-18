#include "blockdevice/iblock_device.hpp"

/**
 * A basic implementation of the IBlockDevice interface that directly accesses the disk.
 *
 * The RawBlockDevice provides unprocessed, low-level block read and write operations.
 * It acts as a thin wrapper around the underlying @ref IDisk interface, exposing
 * block-oriented operations without adding additional logic such as error correction
 * and encryption.
 *
 * This class can serve as the simplest possible bridge between higher-level
 * PPFS and a physical or virtual disk.
 */
class RawBlockDevice : public IBlockDevice {
private:
    size_t _block_size;
    IDisk& _disk;
public:
    /**
     * Constructs a RawBlockDevice instance.
     * @param block_size The size of a single block in bytes.
     * @param disk Reference to a disk object used for low-level read/write operations.
     */
    RawBlockDevice(size_t block_size, IDisk& disk);

    /**
     * Writes a portion of data into a block on the disk.
     * If the data exceeds the block size, it will be truncated.
     * @param data The buffer containing data to be written.
     * @param data_location The block index and offset specifying where to write the data.
     * @return On success, returns the number of bytes written; otherwise returns a DiskError.
     */
    std::expected<size_t, DiskError> writeBlock(
        const std::vector<std::byte>& data, DataLocation data_location) override;
    
    /**
     * Reads a portion of data from a block on the disk.
     * If the requested bytes exceed the block size, they will be truncated.
     * @param data_location The block index and offset specifying where to start reading.
     * @param bytes_to_read The number of bytes requested to be read.
     * @return On success, returns a vector of bytes read; otherwise returns a DiskError.
     */
    std::expected<std::vector<std::byte>, DiskError> readBlock(
        DataLocation data_location, size_t bytes_to_read) override;
    
    /**
     * Returns the raw block size, all metadata included.
     * @return The size of one block in bytes.
     */
    size_t rawBlockSize() const override;

    /**
     * Returns the data size per block.
     * @return The size of data that fits in a single block (equal to the block size here).
     */
    size_t dataSize() const override;

    /**
     * Returns the number of available blocks.
     * @return Number of blocks.
     */
     size_t numOfBlocks() const override;
};