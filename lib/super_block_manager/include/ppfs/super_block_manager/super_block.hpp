#pragma once
#include "ppfs/blockdevice/ecc_type.hpp"
#include "ppfs/common/types.hpp"

#include <cstdint>

/**
 * On-disk superblock containing filesystem metadata.
 */
struct __attribute__((packed)) SuperBlock {
    std::uint8_t signature[4] = { 'P', 'P', 'F', 'S' }; ///< Filesystem signature "PPFS"
    block_index_t total_blocks; ///< Total number of blocks in filesystem
    block_index_t total_inodes; ///< Total number of inodes
    block_index_t block_bitmap_address; ///< Block address of data block bitmap
    block_index_t inode_bitmap_address; ///< Block address of inode bitmap
    block_index_t inode_table_address; ///< Block address of inode table
    block_index_t journal_address; ///< Block address of journal (if used)
    block_index_t first_data_blocks_address; ///< First block of data region
    block_index_t last_data_block_address; ///< Last block of data region
    std::uint32_t block_size; ///< Size of one block in bytes
    std::uint64_t crc_polynomial; ///< CRC polynomial (if ecc_type is CRC)
    std::uint32_t rs_correctable_bytes; ///< Reed-Solomon correctable bytes (if ecc_type is RS)
    ECCType ecc_type; ///< Error correction type used
};
