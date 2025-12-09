#pragma once

#include "filesystem/ppfs.hpp"

struct FileAttributes {
    size_t size;
    size_t block_size;
    InodeType type;
};

/**
 * This class extends PpFS with functionalities needed for working
 * in linux envirionment
 */
class PpFSLinux : public PpFS {
public:
    PpFSLinux(IDisk& disk);
    /**
     * This method returns attribues of a file
     * specified by a given inode.
     */
    [[nodiscard]] std::expected<FileAttributes, FsError> getAttributes(inode_index_t inode_index);

    [[nodiscard]] std::expected<inode_index_t, FsError> lookup(
        inode_index_t parent_index, std::string_view name);

    [[nodiscard]] std::expected<std::vector<DirectoryEntry>, FsError> getDirectoryEntries(
        inode_index_t inode);

    [[nodiscard]] std::expected<inode_index_t, FsError> createDirectoryByParent(
        inode_index_t parent, std::string_view name);

    [[nodiscard]] std::expected<file_descriptor_t, FsError> openByInode(
        inode_index_t inode, OpenMode mode);

    [[nodiscard]] std::expected<inode_index_t, FsError> createWithParentInode(
        std::string_view name, inode_index_t parent);

private:
    [[nodiscard]] std::expected<FileAttributes, FsError> _unprotectedGetAttributes(
        inode_index_t inode_index);

    [[nodiscard]] std::expected<inode_index_t, FsError> _unprotectedLookup(
        inode_index_t parent_index, std::string_view name);

    [[nodiscard]] std::expected<std::vector<DirectoryEntry>, FsError>
    _unprotectedGetDirectoryEntries(inode_index_t inode);

    [[nodiscard]] std::expected<inode_index_t, FsError> _unprotectedCreateDirectoryByParent(
        inode_index_t parent, std::string_view name);

    [[nodiscard]] std::expected<file_descriptor_t, FsError> _unprotectedOpenByInode(
        inode_index_t inode, OpenMode mode);

    [[nodiscard]] std::expected<inode_index_t, FsError> _unprotectedCreateWithParentInode(
        std::string_view name, inode_index_t parent);
};