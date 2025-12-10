#pragma once

#include "iblock_device.hpp"
#include <memory>
#include <optional>

class Logger;

/**
 * @brief Block device that applies Hamming code for error detection and correction.
 *
 * The HammingBlockDevice class provides an abstraction layer over a raw disk (IDisk),
 * encoding each written block with Extended Hamming ECC bits and decoding on read.
 * It allows automatic detection and correction of single-bit errors during read/write operations.
 * It detects double-bit errors but cannot correct them.
 */
class HammingBlockDevice : public IBlockDevice {
public:
    /**
     * @brief Constructs a Hamming-encoded block device.
     * @param block_size_power Power of two determining the raw block size (2^block_size_power
     * bytes).
     * @param disk Reference to the underlying disk device implementing IDisk.
     * @param logger Optional shared_ptr to Logger for tracking error corrections/detections.
     */
    HammingBlockDevice(int block_size_power, IDisk& disk, std::shared_ptr<Logger> logger = nullptr);

    /**
     * @brief Writes a block of data to the device using Hamming encoding.
     * @param data Raw data bytes to be written.
     * @param data_location Target data location on the device (block index and offset).
     * @return Expected number of bytes written, or a FsError on failure.
     *
     * Before writing, data on the target block is read and corrected if necessary.
     *
     * If size of data exceeds the data size per block, it will be truncated.
     */
    [[nodiscard]] virtual std::expected<size_t, FsError> writeBlock(
        const std::vector<std::uint8_t>& data, DataLocation data_location) override;

    /**
     * @brief Reads a block of data from the device and performs Hamming error correction.
     * @param data_location Data location specifying block index and offset.
     * @param bytes_to_read Number of bytes to read.
     * @return Expected vector of decoded data bytes, or a FsError on failure.
     *
     * If size of requested bytes exceeds the data size available on the block, it will be
     * truncated.
     */
    [[nodiscard]] virtual std::expected<std::vector<std::uint8_t>, FsError> readBlock(
        DataLocation data_location, size_t bytes_to_read) override;

    /**
     * @brief Fills a specific block with zeros.
     */
    [[nodiscard]] virtual std::expected<void, FsError> formatBlock(
        unsigned int block_index) override;

    /**
     * @brief Returns the total raw block size in bytes (including ECC bits).
     */
    virtual size_t rawBlockSize() const override;

    /**
     * @brief Returns the size in bytes of the actual data per block (without ECC bits).
     */
    virtual size_t dataSize() const override;
    /**
     * @brief Returns the total number of blocks available on the underlying disk.
     */
    virtual size_t numOfBlocks() const override;

private:
    size_t _block_size;
    size_t _data_size;
    IDisk& _disk;
    std::shared_ptr<Logger> _logger;

    std::vector<std::uint8_t> _encodeData(const std::vector<std::uint8_t>& data);
    std::vector<std::uint8_t> _extractData(const std::vector<std::uint8_t>& encoded_data);

    [[nodiscard]] std::expected<std::vector<std::uint8_t>, FsError> _readAndFixBlock(
        int block_index);
};

/**
 * @brief Iterator to traverse data bit indices in a Hamming-encoded block.
 *
 * This iterator skips parity bits and returns indices of data bits only.
 */
class HammingDataBitsIterator {
public:
    HammingDataBitsIterator(int block_size, int data_size);
    std::optional<unsigned int> next();

private:
    int _block_size;
    int _data_size;
    unsigned int _current_index;
    int _data_bits_returned;
};

/**
 * @brief Iterator to traverse used byte indices in a Hamming-encoded block.
 *
 * This iterator returns indices of bytes that contain data bits and
 * parity bits.
 */
class HammingUsedBitsIterator {
public:
    HammingUsedBitsIterator(int block_size, int data_size);
    std::optional<unsigned int> next();

private:
    int _block_size;
    int _data_size;
    unsigned int _current_index;
    int _data_bits_returned;
    int _next_parity_bit;
};
