#pragma once
#include "blockdevice/ecc_type.hpp"
#include "common/types.hpp"

#include <cstdint>

struct __attribute__((packed)) SuperBlock {
    std::uint8_t signature[4] = { 'P', 'P', 'F', 'S' };
    block_index_t total_blocks;
    block_index_t total_inodes;
    block_index_t block_bitmap_address;
    block_index_t inode_bitmap_address;
    block_index_t inode_table_address;
    block_index_t journal_address;
    block_index_t first_data_blocks_address;
    block_index_t last_data_block_address;
    unsigned int block_size;
    unsigned long int crc_polynomial;
    size_t rs_correctable_bytes;
    ECCType ecc_type;
};
