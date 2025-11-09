#include "blockdevice/parity_block_device.hpp"

#include <cstdint>

ParityBlockDevice::ParityBlockDevice(int block_size, IDisk& disk)
    : _raw_block_size(block_size), _data_size(block_size - 1), _disk(disk) { }

size_t ParityBlockDevice::dataSize() const {
    return _data_size;
}

size_t ParityBlockDevice::rawBlockSize() const {
    return _raw_block_size;
}

size_t ParityBlockDevice::numOfBlocks() const {
    return _disk.size() / _raw_block_size;
}
std::expected<void, DiskError> ParityBlockDevice::formatBlock(unsigned int block_index) {
    auto res = _disk.write(block_index, std::vector<std::byte>(_raw_block_size,  static_cast<std::byte>(0)));
    return res.has_value() ? std::expected<void, DiskError>() : std::unexpected(res.error());
}

std::expected<size_t, DiskError> ParityBlockDevice::writeBlock(
    const std::vector<std::byte>& data, DataLocation data_location){
    size_t to_write = std::min(data.size(), _data_size - data_location.offset);

    auto raw_block = _disk.read(data_location.block_index, _raw_block_size);
    if(!raw_block.has_value())
        return std::unexpected(raw_block.error());

    bool parity = _checkParity(raw_block.value());
    if(!parity)
        return std::unexpected(DiskError::CorrectionError);
    
    std::copy(data.begin(), data.begin() + to_write, raw_block.value().begin() + data_location.offset);

    parity = _checkParity(raw_block.value());

    if(!parity)
        raw_block.value()[_raw_block_size - 1] ^=  static_cast<std::byte>(1);
    
    return _disk.write(_raw_block_size * data_location.block_index, raw_block.value());
}

std::expected<std::vector<std::byte>, DiskError> ParityBlockDevice::readBlock(
    DataLocation data_location, size_t to_read) {
    to_read = std::min(to_read, _data_size - data_location.offset);
    
    auto raw_block = _disk.read(data_location.block_index, _raw_block_size);
    if(!raw_block.has_value())
        return std::unexpected(raw_block.error());

    bool parity = _checkParity(raw_block.value());
    if(!parity)
        return std::unexpected(DiskError::CorrectionError);
    
    auto data = raw_block.value();
    return std::vector<std::byte>(data.begin() + data_location.offset, data.begin() + data_location.offset + to_read);
}

bool ParityBlockDevice::_checkParity(std::vector<std::byte> data){
    size_t ones = 0;
    for (auto b : data) {
        uint8_t v = std::to_integer<uint8_t>(b);
        ones += std::popcount(v);
    }
    return !(ones & 1);
}
