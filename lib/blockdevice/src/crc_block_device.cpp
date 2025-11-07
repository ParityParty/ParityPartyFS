#include "blockdevice/crc_block_device.hpp"

CrcBlockDevice::CrcBlockDevice(unsigned long int polynomial)
    : _polynomial(polynomial)
{
}