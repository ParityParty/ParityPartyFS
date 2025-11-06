#pragma once
#include "disk/idisk.hpp"
#include "super_block_manager/super_block.hpp"

#include <expected>

struct ISuperBlockManager {
    std::expected<SuperBlock, DiskError> get();
    std::expected<void, DiskError> update(SuperBlock new_super_block);
};
