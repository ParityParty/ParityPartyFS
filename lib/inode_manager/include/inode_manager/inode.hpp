#pragma once
#include "common/types.hpp"

#include <array>

enum FileType {
    File,
    Directory,
};

struct Inode {
    FileType type;
    unsigned int file_size;
    unsigned long int time_creation;
    unsigned long int time_modified;
    std::array<block_index_t, 12> direct_blocks;
    block_index_t indirect_block;
    block_index_t doubly_indirect_block;
    block_index_t trebly_indirect_block;
};