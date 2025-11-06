#pragma once
#include "common/types.hpp"
#include "disk/idisk.hpp"

#include <expected>

struct IBlockManager {
    std::expected<void, DiskError> reserve(block_index_t block);
    std::expected<void, DiskError> free(block_index_t inode);
    std::expected<block_index_t, DiskError> getFree();
    std::expected<unsigned int, DiskError> numFree();
    std::expected<unsigned int, DiskError> numTotal();
};