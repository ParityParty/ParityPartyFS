#pragma once

#include "filesystem/ifilesystem.hpp"

/**
 * Mock of filesystem. Does not perform any operations. Just sleeps.
 */
class FsMock : public IFilesystem {
public:
    std::expected<void, FsError> format() override;
    std::expected<void, FsError> create(std::string path) override;
    std::expected<inode_index_t, FsError> open(std::string path) override;
    std::expected<void, FsError> close(inode_index_t inode) override;
    std::expected<void, FsError> remove(std::string path) override;
    std::expected<std::vector<std::uint8_t>, FsError> read(
        inode_index_t inode, size_t offset, size_t size) override;
    std::expected<void, FsError> write(
        inode_index_t inode, std::vector<std::uint8_t> buffer, size_t offset) override;
    std::expected<void, FsError> createDirectory(std::string path) override;
    std::expected<std::vector<std::string>, FsError> readDirectory(std::string path) override;
};
