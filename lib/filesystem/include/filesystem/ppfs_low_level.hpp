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

    [[nodiscard]] virtual std::expected<void, FsError> getDirectoryEntries(
        inode_index_t inode, static_vector<DirectoryEntry>& buf) override;

    [[nodiscard]] virtual std::expected<inode_index_t, FsError> createDirectoryByParent(
        inode_index_t parent, std::string_view name) override;

    [[nodiscard]] virtual std::expected<file_descriptor_t, FsError> openByInode(
        inode_index_t inode, OpenMode mode) override;

    [[nodiscard]] virtual std::expected<inode_index_t, FsError> createWithParentInode(
        std::string_view name, inode_index_t parent) override;

    [[nodiscard]] virtual std::expected<void, FsError> removeByNameAndParent(
        inode_index_t inode, std::string_view name, bool recursive = false) override;

    [[nodiscard]] virtual std::expected<void, FsError> truncate(
        inode_index_t inode, size_t new_size) override;

private:
    [[nodiscard]] std::expected<FileAttributes, FsError> _unprotectedGetAttributes(
        inode_index_t inode_index);

    [[nodiscard]] std::expected<inode_index_t, FsError> _unprotectedLookup(
        inode_index_t parent_index, std::string_view name);

    [[nodiscard]] std::expected<void, FsError>
    _unprotectedGetDirectoryEntries(inode_index_t inode, static_vector<DirectoryEntry>& buf);

    [[nodiscard]] std::expected<inode_index_t, FsError> _unprotectedCreateDirectoryByParent(
        inode_index_t parent, std::string_view name);

    [[nodiscard]] std::expected<file_descriptor_t, FsError> _unprotectedOpenByInode(
        inode_index_t inode, OpenMode mode);

    [[nodiscard]] std::expected<inode_index_t, FsError> _unprotectedCreateWithParentInode(
        std::string_view name, inode_index_t parent);

    [[nodiscard]] std::expected<void, FsError> _unprotectedRemoveByNameAndParent(
        inode_index_t inode, std::string_view name, bool recursive);

    [[nodiscard]] std::expected<void, FsError> _unprotectedTruncate(
        inode_index_t inode, size_t new_size);
};