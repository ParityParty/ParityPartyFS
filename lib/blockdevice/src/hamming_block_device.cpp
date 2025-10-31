#include "blockdevice/hamming_block_device.hpp"

#include <cmath>
#include <cstdint>

HammingBlockDevice::HammingBlockDevice(int block_size_power, IDisk& disk) 
: _disk(disk)
{
    _block_size = 1 << block_size_power;
    int parity_bytes = (int) std::ceil(((float)(block_size_power * 3 + 1)) / 8.0);
    _data_size = _block_size - parity_bytes;
}

std::expected<std::vector<std::byte>, DiskError> HammingBlockDevice::_readAndFixBlock(int block_index) {
    auto read_result = _disk.read(block_index * _block_size, _block_size);
    if (!read_result.has_value()) {
        return std::unexpected(read_result.error());
    }
    
    auto encoded_data = read_result.value();
    
    unsigned int error_position = 0;
    bool parity = true;
    
    HammingUsedBitsIterator it(_block_size, _data_size);
    int index = it.next();
    while (index != -1) {
        if (_getBit(encoded_data, index)) {
            error_position ^= (unsigned int)index;
            parity = !parity;
        }
        index = it.next();
    }
    
    if(!parity){
        unsigned int flipped_bit_value = _getBit(encoded_data, error_position) ? 0 : 1;
        _setBit(encoded_data, error_position, flipped_bit_value);
        auto disk_result = _disk.write(block_index * _block_size + error_position / 8, { encoded_data.begin() + error_position / 8, encoded_data.begin() + error_position / 8 + 1} );
        if (!disk_result.has_value()) {
            return std::unexpected(disk_result.error());
        }
    }
    else{
        if(error_position != 0){
            return std::unexpected(DiskError::CorrectionError);
        }
    }
    
    return encoded_data;
    
}

int HammingBlockDevice::_getBit(const std::vector<std::byte>& data, unsigned int index) {
    unsigned int byteIndex = index / 8;
    unsigned int bitIndex = index % 8;
    
    uint8_t byteValue = std::to_integer<uint8_t>(data[byteIndex]);
    return (byteValue >> bitIndex) & 0x1;
}

void HammingBlockDevice::_setBit(std::vector<std::byte>& data, unsigned int index, int value) {
    unsigned int byteIndex = index / 8;
    unsigned int bitIndex = index % 8;
    
    uint8_t byteValue = std::to_integer<uint8_t>(data[byteIndex]);
    
    if (value)
    byteValue |= (1 << bitIndex); // set 1
    else
    byteValue &= ~(1 << bitIndex);  // set 0
    
    data[byteIndex] = std::byte(byteValue);
}

std::vector<std::byte> HammingBlockDevice::_extractData(const std::vector<std::byte>& encoded_data) {
    std::vector<std::byte> data(_data_size, std::byte(0));
    HammingDataBitsIterator it(_block_size, _data_size);
    for (unsigned int i = 0; i < _data_size * 8; i++)
            _setBit(data, i, _getBit(encoded_data, it.next()));
    return data;
}

std::vector<std::byte> HammingBlockDevice::_encodeData(const std::vector<std::byte>& data) {
    std::vector<std::byte> encoded_data(_block_size, std::byte(0));
    
    bool parity = true;

    unsigned int parity_xor = 0;
    
    HammingDataBitsIterator it(_block_size, _data_size);
    for (unsigned int i = 0; i < _data_size * 8; i++) {
        int bit_value = _getBit(data, i);
        int raw_index = it.next();
        if (bit_value) {
            parity = !parity;
            parity_xor ^= raw_index;
        }
        _setBit(encoded_data, raw_index, bit_value);
    }
    
    unsigned int parity_index = 1;
    while(parity_index <= _block_size * 8){
        int parity_bit_value = 0;
        if(parity_xor & parity_index){
            parity = !parity;
            parity_bit_value = 1;
        }
        _setBit(encoded_data, parity_index, parity_bit_value);
        parity_index <<= 1;
    }
    
    uint8_t parity_bit = parity ? 0 : 1;
    _setBit(encoded_data, 0, parity_bit);
    
    return encoded_data;
}

std::expected<size_t, DiskError> HammingBlockDevice::writeBlock(const std::vector<std::byte>& data, DataLocation data_location){
    size_t to_write = std::min(data.size(), _data_size - data_location.offset);
    
    auto raw_block = _readAndFixBlock(data_location.block_index);
    if (!raw_block.has_value()) {
        return std::unexpected(raw_block.error());
    }
    auto decoded_data = _extractData(raw_block.value());
    std::copy(data.begin(), data.begin() + to_write, decoded_data.begin() + data_location.offset);
    
    auto new_encoded_block = _encodeData(decoded_data);
    
    auto disk_result = _disk.write(data_location.block_index * _block_size, new_encoded_block);
    
    return to_write;
}

std::expected<std::vector<std::byte>, DiskError> HammingBlockDevice::readBlock(
    DataLocation data_location, size_t bytes_to_read){
    bytes_to_read = std::min(_data_size - data_location.offset, bytes_to_read);
        
    auto raw_block = _readAndFixBlock(data_location.block_index);
    if(!raw_block.has_value()){
        return std::unexpected(raw_block.error());
    }
        
    auto decoded_data = _extractData(raw_block.value());
        
    return std::vector<std::byte>(decoded_data.begin() + data_location.offset, decoded_data.begin() + data_location.offset + bytes_to_read);
}

std::expected<void, DiskError> HammingBlockDevice::formatBlock(unsigned int block_index) {
    std::vector<std::byte> zero_data(_block_size, std::byte(0));
    auto write_result = _disk.write(block_index * _block_size, zero_data);
    return write_result.has_value() ? std::expected<void, DiskError>{} : std::unexpected(write_result.error());
}
    
size_t HammingBlockDevice::rawBlockSize() const
{
    return _block_size;
}

size_t HammingBlockDevice::dataSize() const
{
    return _data_size;
}

size_t HammingBlockDevice::numOfBlocks() const 
{
    return _disk.size() / _block_size;
}

HammingDataBitsIterator::HammingDataBitsIterator(int block_size, int data_size)
: _block_size(block_size), _data_size(data_size), _current_index(0), _data_bits_returned(0)
{
}

int HammingDataBitsIterator::next() {
    if (_data_bits_returned >= _data_size * 8) {
        return -1; // No more data bits
    }
    while ((_current_index & (_current_index - 1)) == 0) {
        _current_index++;
    }
    _data_bits_returned++;
    return _current_index++;
}

HammingUsedBitsIterator::HammingUsedBitsIterator(int block_size, int data_size)
: _block_size(block_size), _data_size(data_size), _current_index(0), _data_bits_returned(0), _next_parity_bit(1)
{
}

int HammingUsedBitsIterator::next() {
    if (_data_bits_returned >= _data_size * 8 && _next_parity_bit >= _block_size * 8) {
        return -1; // No more data bytes
    }
    
    if (_data_bits_returned >= _data_size * 8) {
        _current_index = _next_parity_bit;
        _next_parity_bit <<= 1;
        return _current_index;    
    }
    
    if ((_current_index && (_current_index & (_current_index - 1)) == 0)) {
        _next_parity_bit = _current_index << 1;
    }
    
    if (_current_index && (_current_index & (_current_index - 1)) != 0) {
        _data_bits_returned++;
    }
    
    return _current_index++;
    
}

