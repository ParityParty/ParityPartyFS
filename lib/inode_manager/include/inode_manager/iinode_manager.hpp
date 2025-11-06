#pragma once
#include "common/types.hpp"
#include "disk/idisk.hpp"
#include "inode_manager/inode.hpp"

#include <expected>

struct IInodeManager {
    std::expected<inode_index_t, DiskError> create(Inode inode);
    std::expected<void, DiskError> remove(inode_index_t inode);
    std::expected<Inode, DiskError> get(inode_index_t inode);
    std::expected<unsigned int, DiskError> numFree();
    std::expected<unsigned int, DiskError> numTotal();
};