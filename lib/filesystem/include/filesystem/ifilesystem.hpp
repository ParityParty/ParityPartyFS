#pragma once
#include "disk/idisk.hpp"

#include <common/types.hpp>
#include <expected>

struct IFilesystem {
    std::expected<void, DiskError> format();
    std::expected<inode_index_t, DiskError> open(std::string path);
    std::expected<void, DiskError> close(inode_index_t inode);
    std::expected<void, DiskError> remove(std::string path);
    std::expected<std::vector<std::byte>, DiskError> read(
        inode_index_t inode, size_t offset, size_t size);
    std::expected<void, DiskError> write(
        inode_index_t inode, std::vector<std::byte> buffer, size_t offset);
    std::expected<inode_index_t, DiskError> createDirectory(std::string path);
};
