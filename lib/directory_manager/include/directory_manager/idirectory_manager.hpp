#pragma once
#include "directory_manager/directory.hpp"
#include "disk/idisk.hpp"
#include <expected>
#include <vector>

struct IDirectoryManager {
    std::expected<std::vector<DirectoryEntry>, DiskError> get(inode_index_t inode);
};
