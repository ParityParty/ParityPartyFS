#pragma once

#include "filesystem/ifilesystem_low_level.hpp"
#include "filesystem/ppfs.hpp"
class PpFSLowLevel : virtual public IFilesystemLowLevel, public PpFS {
public:
    PpFSLowLevel(IDisk& disk);
    /**
     * This method returns attribues of a file
     * specified by a given inode.
     */
    [[nodiscard]] virtual std::expected<FileAttributes, FsError> getAttributes(
        inode_index_t inode_index) override;

    [[nodiscard]] virtual std::expected<inode_index_t, FsError> lookup(
        inode_index_t parent_index, std::string_view name) override;

    [[nodiscard]] virtual std::expected<std::vector<DirectoryEntry>, FsError> getDirectoryEntries(
        inode_index_t inode) override;

    [[nodiscard]] virtual std::expected<inode_index_t, FsError> createDirectoryByParent(
        inode_index_t parent, std::string_view name) override;

    [[nodiscard]] virtual std::expected<file_descriptor_t, FsError> openByInode(
        inode_index_t inode, OpenMode mode) override;

    [[nodiscard]] virtual std::expected<inode_index_t, FsError> createWithParentInode(
        std::string_view name, inode_index_t parent) override;

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