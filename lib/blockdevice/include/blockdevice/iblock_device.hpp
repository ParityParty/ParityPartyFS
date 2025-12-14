#pragma once

#define MAX_BLOCK_SIZE 4096

#include <expected>
#include <vector>

#include "common/static_vector.hpp"
#include "disk/idisk.hpp"

/**
 * Represents a specific position on the disk defined by a block index and an offset.
 * It represents location as seen from the upper ppfs layer, not the raw disk layer.
 */
struct DataLocation {
    int block_index;
    size_t offset;

    DataLocation(int block_index, size_t offset);
    DataLocation() = default;
};

/**
 * Abstract interface for block-level storage operations.
 *
 * This interface serves as a bridge between higher-level file systems (such as PPFS)
 * and the lower-level disk layer represented by @ref IDisk.
 * Implementations of IBlockDevice may perform additional data processing,
 * such as error correction codes (ECC), checksums and encryption,
 * before persisting data onto the underlying disk.
 *
 * The interface provides generic read and write operations that work with
 * blocks of raw bytes, without exposing internal details of the physical storage device.
 */
class IBlockDevice {
public:
    virtual ~IBlockDevice() = default;

    /**
     * Writes a sequence of bytes into the device at a specified data location.
     *
     * Implementations may modify or encode the data (e.g., by adding redundancy or ECC)
     * before writing to the underlying disk.
     *
     * @param data The buffer of bytes to be written.
     * @param data_location The target location (block index and offset) on the device.
     * @return On success, returns the number of bytes written; otherwise returns a FsError.
     */
    [[nodiscard]] virtual std::expected<size_t, FsError> writeBlock(
        const static_vector<std::uint8_t>& data, DataLocation data_location)
        = 0;

    /**
     * Reads a sequence of bytes from the device at a specified data location.
     *
     * Implementations may perform additional processing, such as verifying or decoding
     * correction codes, before returning the requested data.
     *
     * @param data_location The source location (block index and offset) on the device.
     * @param bytes_to_read Number of bytes to read starting from the specified location.
     * @param data Output buffer to fill with read data, must have sufficient capacity
     * @return void on success, error otherwise
     */
    [[nodiscard]] virtual std::expected<void, FsError> readBlock(
        DataLocation data_location, size_t bytes_to_read, static_vector<uint8_t>& data)
        = 0;

    /**
     * Returns the physical (raw) block size of the underlying device.
     * @return Size of one raw block in bytes.
     */
    virtual size_t rawBlockSize() const = 0;

    /**
     * Returns the usable data size within a block.
     *
     * This may be smaller than the raw block size if additional bytes are reserved
     * for metadata or correction codes.
     *
     * @return The size of usable (payload) data in a single block.
     */
    virtual size_t dataSize() const = 0;

    /**
     * Returns the number of available blocks.
     * @return Number of blocks.
     */
    virtual size_t numOfBlocks() const = 0;

    /**
     * Formats a specific block on the device.
     * @param block_index The index of the block to format.
     * @return On success, returns void; otherwise returns a FsError.
     *
     * After formatting, the block is set to a correct state (e.g., all zeros with valid ECC).
     */
    [[nodiscard]] virtual std::expected<void, FsError> formatBlock(unsigned int block_index) = 0;
};