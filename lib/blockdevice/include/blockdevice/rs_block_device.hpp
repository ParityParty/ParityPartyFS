#include "blockdevice/iblock_device.hpp"
#include "ecc_helpers/polynomial_gf256.hpp"

#define BLOCK_LENGTH 255
#define MESSAGE_LENGTH 251

class ReedSolomonBlockDevice : public IBlockDevice {
public:
    ReedSolomonBlockDevice(IDisk& disk);

    virtual std::expected<size_t, DiskError> writeBlock(
        const std::vector<std::byte>& data, DataLocation data_location);
    
    virtual std::expected<std::vector<std::byte>, DiskError> readBlock(
        DataLocation data_location, size_t bytes_to_read);

    virtual size_t rawBlockSize() const;

    virtual size_t dataSize() const;

    virtual size_t numOfBlocks() const;

    virtual std::expected<void, DiskError> formatBlock(unsigned int block_index);

private:
    IDisk& _disk;
    PolynomialGF256 _generator;

    std::vector<std::byte> _encodeBlock(std::vector<std::byte>);
    std::expected<std::vector<std::byte>, DiskError> _readAndFixBlock(std::vector<std::byte>);
    PolynomialGF256 _calculateGenerator();
    std::vector<std::byte> _extractMessage(PolynomialGF256);
};