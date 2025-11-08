#include "blockdevice/rs_block_device.hpp"
#include "ecc_helpers/gf256_utils.h"

ReedSolomonBlockDevice::ReedSolomonBlockDevice(IDisk& disk) : _disk(disk) { 
    _generator = _calculateGenerator();
}

size_t ReedSolomonBlockDevice::numOfBlocks() const {
    return _disk.size() / BLOCK_LENGTH;
}

size_t ReedSolomonBlockDevice::dataSize() const {
    return MESSAGE_LENGTH;
}

size_t ReedSolomonBlockDevice::rawBlockSize() const {
    return BLOCK_LENGTH;
}

std::expected<void, DiskError> ReedSolomonBlockDevice::formatBlock(unsigned int block_index) {
    std::vector<std::byte> zero_data(BLOCK_LENGTH, std::byte(0));
    auto write_result = _disk.write(block_index * BLOCK_LENGTH, zero_data);
    return write_result.has_value() ? std::expected<void, DiskError>{} : std::unexpected(write_result.error());
}


std::expected<std::vector<std::byte>, DiskError> ReedSolomonBlockDevice::readBlock(
    DataLocation data_location, size_t bytes_to_read){
    
    bytes_to_read = std::min(MESSAGE_LENGTH - data_location.offset, bytes_to_read);
    auto raw_block = _disk.read(data_location.block_index * BLOCK_LENGTH, BLOCK_LENGTH);
    if(!raw_block.has_value()){
        return std::unexpected(raw_block.error());
    }

    auto decoded = _readAndFixBlock(raw_block.value());
    if(!decoded.has_value()){
        return std::unexpected(decoded.error());
    }

    auto decoded_data = decoded.value();
    return std::vector<std::byte>(decoded_data.begin() + data_location.offset, decoded_data.begin() + data_location.offset + bytes_to_read);
}

std::expected<size_t, DiskError> ReedSolomonBlockDevice::writeBlock(const std::vector<std::byte>& data, DataLocation data_location) {
    size_t to_write = std::min(data.size(), MESSAGE_LENGTH - data_location.offset);
    
    auto raw_block = _disk.read(BLOCK_LENGTH * data_location.block_index, BLOCK_LENGTH);
    if (!raw_block.has_value()) {
        return std::unexpected(raw_block.error());
    }

    auto decoding_res = _readAndFixBlock(raw_block.value());
    if (!decoding_res.has_value()){
        return std::unexpected(decoding_res.error());
    }

    auto decoded = decoding_res.value();

    std::copy(data.begin(), data.begin() + to_write, decoded.begin() + data_location.offset);
    
    auto new_encoded_block = _encodeBlock(decoded);
    
    auto disk_result = _disk.write(data_location.block_index * BLOCK_LENGTH, new_encoded_block);
    
    return to_write;
}





std::vector<std::byte> ReedSolomonBlockDevice::_encodeBlock(std::vector<std::byte> data){
    int t = BLOCK_LENGTH - MESSAGE_LENGTH;
    
    auto message = PolynomialGF256(gf256_utils::bytes_to_gf(data));
    auto shifted_message = message.multiply_by_xk(t);

    auto encoded = message + message.mod(_generator);

    return gf256_utils::gf_to_bytes(encoded.slice(0, BLOCK_LENGTH));

}

std::expected<std::vector<std::byte>, DiskError> ReedSolomonBlockDevice::_readAndFixBlock(std::vector<std::byte> raw_bytes){
    // Calculate syndroms
    auto code_word = PolynomialGF256(gf256_utils::bytes_to_gf(raw_bytes));
    auto syndromes = std::vector<GF256>();
    syndromes.reserve(BLOCK_LENGTH - MESSAGE_LENGTH);
    
    GF256 alpha = GF256::getPrimitiveElement();
    GF256 power = GF256(alpha);

    bool is_message_correct = true;
    for (int i = 0; i <= BLOCK_LENGTH - MESSAGE_LENGTH; i++) {
        auto syndrome = code_word.evaluate(alpha);
        
        if (syndrome != 0)
            is_message_correct = false;
        syndromes.push_back(code_word.evaluate(alpha));
        power = power * alpha;
    }

    //TODO - fixing block
    if(!is_message_correct)
        return std::unexpected(DiskError::CorrectionError);

    return _extractMessage(code_word);
}

std::vector<std::byte> _extractMessage(PolynomialGF256 endoced_polynomial) {
    return gf256_utils::gf_to_bytes(endoced_polynomial.slice(BLOCK_LENGTH - MESSAGE_LENGTH, BLOCK_LENGTH));
}


PolynomialGF256 ReedSolomonBlockDevice::_calculateGenerator() {
    PolynomialGF256 g({ GF256(1) });
    GF256 alpha = GF256::getPrimitiveElement();

    GF256 power(alpha);
    for (int i = 0; i <= BLOCK_LENGTH - MESSAGE_LENGTH; i++) {
        // (x - α^(i+1))
        PolynomialGF256 term({ power, GF256(1) });
        g = g * term;
        power = power * alpha;
    }

    return g;
}

