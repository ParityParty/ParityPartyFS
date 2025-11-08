#include "blockdevice/crc_block_device.hpp"

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

CrcBlockDevice::CrcBlockDevice(CrcPolynomial polynomial, IDisk& disk)
    : _polynomial(polynomial)
    , _disk(disk)
{
}

std::expected<size_t, DiskError> CrcBlockDevice::writeBlock(
    const std::vector<std::byte>& data, DataLocation data_location)
{
}

std::expected<std::vector<std::byte>, DiskError> CrcBlockDevice::readBlock(
    DataLocation data_location, size_t bytes_to_read)
{
}

size_t CrcBlockDevice::rawBlockSize() const { }

size_t CrcBlockDevice::dataSize() const { }

size_t CrcBlockDevice::numOfBlocks() const { }

std::expected<void, DiskError> CrcBlockDevice::formatBlock(unsigned int block_index) { }
