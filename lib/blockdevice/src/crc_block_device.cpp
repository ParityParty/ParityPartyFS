#include "blockdevice/crc_block_device.hpp"
#include "blockdevice/bit_helpers.hpp"

unsigned int CrcPolynomial::_findDegree(unsigned long int coefficients)
{
    unsigned int counter = 0;
    while (coefficients > 0) {
        coefficients >>= 1;
        counter++;
    }

    // Counter counts terms, so we need to subtract one to get degree
    return counter - 1;
}

CrcPolynomial::CrcPolynomial(unsigned long int coefficients, unsigned int n)
    : _coefficients(coefficients)
    , _n(n)
{
}

CrcPolynomial CrcPolynomial::MsgExplicit(unsigned long int polynomial)
{
    return { polynomial, _findDegree(polynomial) };
}

CrcPolynomial CrcPolynomial::MsgImplicit(unsigned long int polynomial)
{
    unsigned long int explicitPolynomial = (polynomial << 1) + 1;
    return { explicitPolynomial, _findDegree(explicitPolynomial) };
}

unsigned long int CrcPolynomial::getCoefficients() const { return _coefficients; }

unsigned int CrcPolynomial::getDegree() const { return _n; }

bool CrcBlockDevice::_checkBlock(const std::vector<std::byte>& block)
{
    auto bits = BitHelpers::blockToBits(block);
}

CrcBlockDevice::CrcBlockDevice(CrcPolynomial polynomial, IDisk& disk, size_t block_size)
    : _polynomial(polynomial)
    , _disk(disk)
    , _block_size(block_size)
{
}

std::expected<size_t, DiskError> CrcBlockDevice::writeBlock(
    const std::vector<std::byte>& data, DataLocation data_location)
{
}

std::expected<std::vector<std::byte>, DiskError> CrcBlockDevice::readBlock(
    DataLocation data_location, size_t bytes_to_read)
{
    auto bytes_res = _disk.read(data_location.block_index * _block_size, _block_size);
    if (!bytes_res.has_value()) {
        return std::unexpected(bytes_res.error());
    }
    auto block = bytes_res.value();
}

size_t CrcBlockDevice::rawBlockSize() const { return _block_size; }

size_t CrcBlockDevice::dataSize() const { return _block_size - _polynomial.getDegree(); }

size_t CrcBlockDevice::numOfBlocks() const { _disk.size() / _block_size; }

std::expected<void, DiskError> CrcBlockDevice::formatBlock(unsigned int block_index) { }
