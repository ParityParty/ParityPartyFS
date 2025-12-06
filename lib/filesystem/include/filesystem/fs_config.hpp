#pragma once
#include "blockdevice/crc_polynomial.hpp"
#include "blockdevice/ecc_type.hpp"
#include "common/types.hpp"

struct FsConfig {
    // Total size of the filesystem in bytes. Has to be multiple of block_size.
    std::uint64_t total_size = 0;

    // Expected average file size in bytes. Used for calculating filesystem parameters.
    std::uint64_t average_file_size = 0;

    // Block size in bytes. Must be a power of two.
    std::uint32_t block_size = 512;

    // Error correction type.
    ECCType ecc_type = ECCType::None;

    // If ECC type is CRC, polynomial used for cyclic redundancy check.
    //
    // The default polynomial guarantees error detection up to 5 bit flips in messages up to 30 000
    // bits according to CrcZoo
    CrcPolynomial crc_polynomial = CrcPolynomial::MsgImplicit(0x9960034c);

    // If ECC type is Reed-Solomon, number of correctable bytes.
    std::uint32_t rs_correctable_bytes = 3;

    bool use_journal = false;
};
