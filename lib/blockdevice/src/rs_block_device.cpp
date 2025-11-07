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

std::vector<std::byte> ReedSolomonBlockDevice::_encode(std::vector<std::byte> data){
    int t = BLOCK_LENGTH - MESSAGE_LENGTH;
    
    auto message = PolynomialGF256(gf256_utils::bytes_to_gf(data));
    auto shifted_message = message.multiply_by_xk(t);

    auto encoded = message + message.mod(_generator);

    return gf256_utils::gf_to_bytes(encoded.slice(0, BLOCK_LENGTH));

}

PolynomialGF256 ReedSolomonBlockDevice::_calculateGenerator() {
    PolynomialGF256 g({ GF256(1) });
    GF256 alpha(2);

    GF256 power(1);
    for (int i = 0; i < BLOCK_LENGTH - MESSAGE_LENGTH; ++i) {
        // (x - α^(i+1))
        PolynomialGF256 term({ -power, GF256(1) });
        g = g * term;
        power = power * alpha;
    }

    return g;
}

