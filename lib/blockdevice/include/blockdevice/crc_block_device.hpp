#pragma once

#include "blockdevice/iblock_device.hpp"
#include "common/static_vector.hpp"
#include "ecc_helpers/crc_polynomial.hpp"
#include <memory>

class Logger;

/**
 * Block device with customizable crc error detection
 *
 * Everybody is invited to Parity Party!
 */
class CrcBlockDevice : public IBlockDevice {
    CrcPolynomial _polynomial;
    IDisk& _disk;
    size_t _block_size;
    std::shared_ptr<Logger> _logger;

    /**
     * Calculate block crc and write to disk
     *
     * @param block block to be written, block has rawBlockSize bytes, redundancy bits at the end
     * will be changed
     * @return void if successful, error otherwise
     */
    [[nodiscard]] std::expected<void, FsError> _calculateAndWrite(
        static_vector<std::uint8_t>& block, block_index_t block_index);

    /**
     * reads whole block with redundancy bits and checks integrity
     *
     * @param block index of a block to read
     * @param block_buffer buffer to read into, must have capacity >= rawBlockSize
     * @return void on success, error otherwise
     */
    [[nodiscard]] std::expected<void, FsError> _readAndCheckRaw(
        block_index_t block, static_vector<std::uint8_t>& block_buffer);

public:
    /**
     * Create CrcBlockDevice with specified polynomial
     *
     * At the party there is enough error detection for everybody, unfortunately there isn't
     * enough generator polynomials, so we had to enforce a rule: Bring your own polynomial to
     * Parity Party! If you forgot yours, ask around, maybe somebody have a *redundant* polynomials.
     *
     * @param polynomial polynomial used for crc with most significant bit first with explicit +1
     * @param logger Optional shared_ptr to Logger for tracking error detections
     */
    CrcBlockDevice(CrcPolynomial polynomial, IDisk& disk, size_t block_size,
        std::shared_ptr<Logger> logger = nullptr);

    /**
     * Writes a sequence of bytes into the device at a specified data location.
     *
     * Before writing, crc algorithm is performed to add redundancy bits
     *
     * @param data The buffer of bytes to be written.
     * @param data_location The target location (block index and offset) on the device.
     * @return On success, returns the number of bytes written; otherwise returns a FsError.
     */
    [[nodiscard]] std::expected<size_t, FsError> writeBlock(
        const static_vector<std::uint8_t>& data, DataLocation data_location) override;

    /**
     * Reads a sequence of bytes from the device at a specified data location.
     *
     * Error detection is performed to ensure data integrity
     *
     * @param data_location The source location (block index and offset) on the device.
     * @param bytes_to_read Number of bytes to read starting from the specified location.
     * @param data Output buffer to fill with read data, must have sufficient capacity
     * @return void on success, error otherwise
     */
    [[nodiscard]] std::expected<void, FsError> readBlock(
        DataLocation data_location, size_t bytes_to_read, static_vector<uint8_t>& data) override;

    /**
     * Returns the physical (raw) block size of the underlying device.
     * @return Size of one raw block in bytes.
     */
    virtual size_t rawBlockSize() const override;

    /**
     * Returns the usable data size within a block.
     *
     * There is n-1 bits reserved for redundancy bits (n is degree of polynomial)
     *
     * @return The size of usable (payload) data in a single block.
     */
    virtual size_t dataSize() const override;

    /**
     * Returns the number of available blocks.
     * @return Number of blocks.
     */
    virtual size_t numOfBlocks() const override;

    /**
     * Formats a specific block on the device.
     * @param block_index The index of the block to format.
     * @return On success, returns void; otherwise returns a FsError.
     *
     * After formatting, the block is set to a correct state (e.g., all zeros with valid redundancy
     * bits).
     */
    [[nodiscard]] virtual std::expected<void, FsError> formatBlock(
        unsigned int block_index) override;
};