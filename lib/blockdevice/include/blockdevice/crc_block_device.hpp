#pragma once

#include "blockdevice/crc_polynomial.hpp"
#include "blockdevice/iblock_device.hpp"

/**
 * Block device with customizable crc error detection
 *
 * Everybody is invited to Parity Party!
 */
class CrcBlockDevice : public IBlockDevice {
    CrcPolynomial _polynomial;
    IDisk& _disk;
    size_t _block_size;

    /**
     * Calculate block crc and write to disk
     *
     * @param block block to be written, block has rawBlockSize bytes, redundancy bits at the end
     * will be changed
     * @return void if successful, error otherwise
     */
    std::expected<void, FsError> _calculateAndWrite(
        std::vector<std::uint8_t>& block, block_index_t block_index);

    /**
     * reads whole block with redundancy bits and checks integrity
     *
     * @param block index of a block to read
     * @return block with redundancy bits on success, error otherwise
     */
    std::expected<std::vector<std::uint8_t>, FsError> _readAndCheckRaw(block_index_t block);

public:
    /**
     * Create CrcBlockDevice with specified polynomial
     *
     * At the party there is enough error detection for everybody, unfortunately there isn't
     * enough generator polynomials, so we had to enforce a rule: Bring your own polynomial to
     * Parity Party! If you forgot yours, ask around, maybe somebody have a *redundant* polynomials.
     *
     * @param polynomial polynomial used for crc with most significant bit first with explicit +1
     */
    CrcBlockDevice(CrcPolynomial polynomial, IDisk& disk, size_t block_size);

    /**
     * Writes a sequence of bytes into the device at a specified data location.
     *
     * Before writing, crc algorithm is performed to add redundancy bits
     *
     * @param data The buffer of bytes to be written.
     * @param data_location The target location (block index and offset) on the device.
     * @return On success, returns the number of bytes written; otherwise returns a FsError.
     */
    std::expected<size_t, FsError> writeBlock(
        const std::vector<std::uint8_t>& data, DataLocation data_location) override;

    /**
     * Reads a sequence of bytes from the device at a specified data location.
     *
     * Error detection is performed to ensure data integrity
     *
     * @param data_location The source location (block index and offset) on the device.
     * @param bytes_to_read Number of bytes to read starting from the specified location.
     * @return On success, returns the bytes read; otherwise returns a FsError.
     */
    std::expected<std::vector<std::uint8_t>, FsError> readBlock(
        DataLocation data_location, size_t bytes_to_read) override;

    /**
     * Returns the physical (raw) block size of the underlying device.
     * @return Size of one raw block in bytes.
     */
    size_t rawBlockSize() const override;

    /**
     * Returns the usable data size within a block.
     *
     * There is n-1 bits reserved for redundancy bits (n is degree of polynomial)
     *
     * @return The size of usable (payload) data in a single block.
     */
    size_t dataSize() const override;

    /**
     * Returns the number of available blocks.
     * @return Number of blocks.
     */
    size_t numOfBlocks() const override;

    /**
     * Formats a specific block on the device.
     * @param block_index The index of the block to format.
     * @return On success, returns void; otherwise returns a FsError.
     *
     * After formatting, the block is set to a correct state (e.g., all zeros with valid redundancy
     * bits).
     */
    std::expected<void, FsError> formatBlock(unsigned int block_index) override;
};