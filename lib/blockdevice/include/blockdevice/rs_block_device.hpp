#include "blockdevice/iblock_device.hpp"
#include "ecc_helpers/polynomial_gf256.hpp"

#define BLOCK_LENGTH 255
#define MESSAGE_LENGTH 251

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
    /** Constructs the RS block device over a given disk. */
    ReedSolomonBlockDevice(IDisk& disk);

    /** Writes data to a block at the specified location. */
    virtual std::expected<size_t, DiskError> writeBlock(
        const std::vector<std::byte>& data, DataLocation data_location);
    
    /** Reads a block from the specified location, returning only the requested bytes. */
    virtual std::expected<std::vector<std::byte>, DiskError> readBlock(
        DataLocation data_location, size_t bytes_to_read);

    /** Returns the size of a raw encoded block in bytes. */
    virtual size_t rawBlockSize() const;

    /** Returns the usable data size of a block in bytes. */
    virtual size_t dataSize() const;

    /** Returns the total number of blocks on the underlying disk. */
    virtual size_t numOfBlocks() const;

    /** Formats a block (zeroes it out) at the given index. */
    virtual std::expected<void, DiskError> formatBlock(unsigned int block_index);

private:
    IDisk& _disk;                    /**< Reference to the underlying disk. */
    PolynomialGF256 _generator;      /**< Reed-Solomon generator polynomial. */

    /** Encodes data into a full RS block with parity bytes. */
    std::vector<std::byte> _encodeBlock(std::vector<std::byte>);

    /** Fixes a block using Reed-Solomon decoding. After fixing, data is returned. */
    std::vector<std::byte> _fixBlockAndExtract(std::vector<std::byte>);

    /** Computes the RS generator polynomial. */
    PolynomialGF256 _calculateGenerator();

    /** Extracts the original message bytes from a full RS-encoded polynomial. */
    std::vector<std::byte> _extractMessage(PolynomialGF256);

    /** Computes error values using Forney’s algorithm. */
    std::vector<GF256> _forney(const PolynomialGF256& omega,
        PolynomialGF256& sigma, const std::vector<GF256>& error_locations);

    /** Computes the error evaluator polynomial omega(x). */
    PolynomialGF256 _calculateOmega(const std::vector<GF256>& syndromes, PolynomialGF256& sigma);

    /** Computes the error locator polynomial sigma(x) using Berlekamp-Massey. */
    PolynomialGF256 _berlekamp_massey(const std::vector<GF256>& syndromes);

    /** Finds error locations (inverse of roots of sigma(x)). */
    std::vector<GF256> _errorLocations(PolynomialGF256 error_location_polynomial);
};
