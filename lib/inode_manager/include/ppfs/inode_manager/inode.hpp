#pragma once
#include "ppfs/common/types.hpp"

#include <array>
#include <cstdint>

enum class InodeType : std::uint8_t {
    File,
    Directory,
};

/**
 * Structure representing one entry in inode table.
 *
 * Inode is assumed to have allocated enough data blocks to contain all data. All unoccupied block
 * pointers have undefined values. Time values are unix time in milliseconds.
 */
struct __attribute__((packed)) Inode {
    std::uint64_t time_creation;
    std::uint64_t time_modified;

    /**
     * First 12 block pointers are stored directly in the inode.
     */
    std::array<block_index_t, 12> direct_blocks;
    /**
     * Points to block with pointers to data blocks
     */
    block_index_t indirect_block;
    /**
     * Points to block with pointers to indirect blocks
     */
    block_index_t doubly_indirect_block;
    /**
     * Points to block with pointers to doubly indirect blocks
     */
    block_index_t trebly_indirect_block;

    std::uint32_t file_size = 0;
    InodeType type;
};