#pragma once
#include "directory_manager/directory.hpp"
#include "disk/idisk.hpp"
#include <expected>
#include <vector>

struct IDirectoryManager {
    std::expected<std::vector<DirectoryEntry>, DiskError> getEntries(inode_index_t inode);
    std::expected<void, DiskError> addEntry(inode_index_t directory, DirectoryEntry entry);
    std::expected<void, DiskError> removeEntry(inode_index_t directory, inode_index_t entry);
};
