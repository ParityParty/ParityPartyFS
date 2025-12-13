#pragma once

#include "blockdevice/iblock_device.hpp"
#include <memory>

class Logger;

/**
 * Simple parity-based block device for error detection.
 *
 * This device wraps a lower-level disk and adds a single parity bit per block.
 * It can detect single-bit flips but cannot correct them.
 */
class ParityBlockDevice : public IBlockDevice {
public:
    /** Constructs a parity-protected block device of given block size. */
    ParityBlockDevice(int block_size, IDisk& disk, std::shared_ptr<Logger> logger = nullptr);

    /**
     * Writes data with an appended parity byte.
     * If data corruption is detected, returns an error.
     */
    std::expected<size_t, FsError> writeBlock(
        const buffer<std::uint8_t>& data, DataLocation data_location) override;

    /**
     * Reads data and verifies parity to detect bit flips.
     * If data corruption is detected, returns an error.
     */

    std::expected<void, FsError> readBlock(
        DataLocation data_location, size_t bytes_to_read, static_vector<uint8_t>& data) override;

    /** Formats a block (fills it with zeros and valid parity). */
    [[nodiscard]] virtual std::expected<void, FsError> formatBlock(
        unsigned int block_index) override;
    /** Returns the total raw block size including the parity byte. */
    virtual size_t rawBlockSize() const override;

    /** Returns the usable data size (excluding parity byte). */
    virtual size_t dataSize() const override;

    /** Returns the number of available blocks on the disk. */
    virtual size_t numOfBlocks() const override;

private:
    IDisk& _disk; /**< Reference to the underlying disk. */
    const int _raw_block_size; /**< Total block size including parity. */
    const int _data_size; /**< Usable data size without parity. */
    std::shared_ptr<Logger> _logger; /**< Optional logger for error detection. */

    /** Calculates overall parity of the block (used to detect bit flips). */
    bool _checkParity(std::vector<std::uint8_t> data);
};
