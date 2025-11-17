#include <optional>

#include "iblock_device.hpp"

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
     */
    HammingBlockDevice(int block_size_power, IDisk& disk);

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
    std::expected<size_t, FsError> writeBlock(
        const std::vector<std::byte>& data, DataLocation data_location) override;

    /**
     * @brief Reads a block of data from the device and performs Hamming error correction.
     * @param data_location Data location specifying block index and offset.
     * @param bytes_to_read Number of bytes to read.
     * @return Expected vector of decoded data bytes, or a FsError on failure.
     *
     * If size of requested bytes exceeds the data size available on the block, it will be
     * truncated.
     */
    std::expected<std::vector<std::byte>, FsError> readBlock(
        DataLocation data_location, size_t bytes_to_read) override;

    /**
     * @brief Fills a specific block with zeros.
     */
    std::expected<void, FsError> formatBlock(unsigned int block_index) override;

    /**
     * @brief Returns the total raw block size in bytes (including ECC bits).
     */
    size_t rawBlockSize() const override;

    /**
     * @brief Returns the size in bytes of the actual data per block (without ECC bits).
     */
    size_t dataSize() const override;
    /**
     * @brief Returns the total number of blocks available on the underlying disk.
     */
    size_t numOfBlocks() const override;

private:
    size_t _block_size;
    size_t _data_size;
    IDisk& _disk;

    std::vector<std::byte> _encodeData(const std::vector<std::byte>& data);
    std::vector<std::byte> _extractData(const std::vector<std::byte>& encoded_data);

    std::expected<std::vector<std::byte>, FsError> _readAndFixBlock(int block_index);
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
