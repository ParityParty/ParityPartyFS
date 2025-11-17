#pragma once
#include "common/types.hpp"

#include <cstdint>

struct SuperBlock {
    block_index_t total_blocks;
    block_index_t free_blocks;
    block_index_t total_inodes;
    block_index_t free_inodes;
    block_index_t block_bitmap_address;
    block_index_t inode_bitmap_address;
    block_index_t inode_table_address;
    block_index_t journal_address;
    block_index_t data_blocks_address;
};
