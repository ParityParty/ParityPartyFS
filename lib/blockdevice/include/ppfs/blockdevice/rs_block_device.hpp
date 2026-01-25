#pragma once
#include "ppfs/blockdevice/iblock_device.hpp"
#include "ppfs/common/static_vector.hpp"
#include "ppfs/ecc_helpers/polynomial_gf256.hpp"
#include <memory>

#define MAX_RS_BLOCK_SIZE 255

class Logger;

/**
 * Implements a block device with Reed-Solomon error correction.
 *
 * Wraps a lower-level disk (IDisk) and provides block-level read/write operations
 * with automatic encoding, decoding, and error correction using Reed-Solomon codes.
 *
 * It treats bytes as symbols.
 * With current setup, it can correct up to two byte errors.
 */
class ReedSolomonBlockDevice : public IBlockDevice {
public:
    /**
     * Constructs the Reed–Solomon block device over a given disk.
     *
     * @param disk Reference to the underlying physical disk device.
     * @param raw_block_size The total encoded block size (in bytes).
     *        Maximum supported value is 255; if a larger value is provided,
     *        it will be automatically limited to 255.
     * @param correctable_bytes Number of data bytes that the code can correct.
     *        The redundancy (parity) region will be twice this size (2 * correctable_bytes).
     *        If the requested value exceeds half of the block size, it will be
     *        automatically reduced to raw_block_size / 2 (hich effectively
     *        makes the block unusable, so we really recommend providing the right configuration.
     * @param logger Optional shared_ptr to Logger for tracking error corrections
     */
    ReedSolomonBlockDevice(IDisk& disk, size_t raw_block_size, size_t correctable_bytes,
        std::shared_ptr<Logger> logger = nullptr);

    /** Writes data to a block at the specified location. */
    [[nodiscard]] virtual std::expected<size_t, FsError> writeBlock(
        const static_vector<std::uint8_t>& data, DataLocation data_location) override;

    /** Reads a block from the specified location, returning only the requested bytes. */
    [[nodiscard]] virtual std::expected<void, FsError> readBlock(
        DataLocation data_location, size_t bytes_to_read, static_vector<uint8_t>& data);

    /** Returns the size of a raw encoded block in bytes. */
    virtual size_t rawBlockSize() const override;

    /** Returns the usable data size of a block in bytes. */
    virtual size_t dataSize() const override;

    /** Returns the total number of blocks on the underlying disk. */
    virtual size_t numOfBlocks() const override;

    /** Formats a block (zeroes it out) at the given index. */
    [[nodiscard]] virtual std::expected<void, FsError> formatBlock(
        unsigned int block_index) override;

private:
    IDisk& _disk; /**< Reference to the underlying disk. */
    PolynomialGF256 _generator; /**< Reed-Solomon generator polynomial. */
    size_t _raw_block_size; /**< Total size of one encoded block in bytes (data + redundancy). */
    size_t
        _correctable_bytes; /**< Number of individual bytes that the code can detect and correct. */
    std::shared_ptr<Logger> _logger; /**< Optional logger for error corrections. */

    /** Encodes data into a full RS block with parity bytes. */
    void _encodeBlock(
        const static_vector<std::uint8_t>& raw_block, static_vector<std::uint8_t>& data);

    /** Fixes a block using Reed-Solomon decoding. After fixing, data is returned. */
    void _fixBlockAndExtract(static_vector<std::uint8_t> raw_block,
        const static_vector<std::uint8_t>& data, block_index_t block_index);

    /** Computes the RS generator polynomial. */
    PolynomialGF256 _calculateGenerator();

    /** Extracts the original message bytes from a full RS-encoded polynomial. */
    void _extractMessage(PolynomialGF256 p, static_vector<std::uint8_t>& data);

    /** Computes error values using Forney’s algorithm. */
    void _forney(const PolynomialGF256& omega, PolynomialGF256& sigma,
        const static_vector<GF256>& error_locations, static_vector<GF256>& error_values);

    /** Computes the error evaluator polynomial omega(x). */
    PolynomialGF256 _calculateOmega(const static_vector<GF256>& syndromes, PolynomialGF256& sigma);

    /** Computes the error locator polynomial sigma(x) using Berlekamp-Massey. */
    PolynomialGF256 _berlekampMassey(const static_vector<GF256>& syndromes);

    /** Finds error locations (inverse of roots of sigma(x)). */
    void _errorLocations(
        PolynomialGF256 error_location_polynomial, static_vector<GF256>& error_locations);
};
