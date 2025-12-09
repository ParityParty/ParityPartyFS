#include "filesystem/ppfs_linux.hpp"

#include <cstring>

PpFSLinux::PpFSLinux(IDisk& disk)
    : PpFS(disk)
{
}

std::expected<FileAttributes, FsError> PpFSLinux::getAttributes(inode_index_t inode_index)
{
    if (!isInitialized())
        return std::unexpected(FsError::PpFS_NotInitialized);

    auto inode_res = _inodeManager->get(inode_index);
    if (!inode_res.has_value())
        return std::unexpected(inode_res.error());

    return FileAttributes { .size = inode_res.value().file_size, .type = inode_res.value().type };
}

std::expected<inode_index_t, FsError> PpFSLinux::lookup(
    inode_index_t parent_index, std::string_view name)
{
    if (!isInitialized())
        return std::unexpected(FsError::PpFS_NotInitialized);
    auto lookup_res = _getInodeFromParent(parent_index, name);

    return lookup_res;
}

std::expected<inode_index_t, FsError> PpFSLinux::createDirectoryByParent(
    inode_index_t parent, std::string_view name)
{
    if (!isInitialized()) {
        return std::unexpected(FsError::PpFS_NotInitialized);
    }

    Inode new_inode { .type = InodeType::Directory };
    auto create_inode_res = _inodeManager->create(new_inode);
    if (!create_inode_res.has_value()) {
        return std::unexpected(create_inode_res.error());
    }
    inode_index_t new_inode_index = create_inode_res.value();

    // Add entry to parent directory
    DirectoryEntry new_entry { .inode = new_inode_index };
    std::strncpy(new_entry.name.data(), name.data(), new_entry.name.size() - 1);
    auto add_entry_res = _directoryManager->addEntry(parent, new_entry);
    if (!add_entry_res.has_value()) {
        return std::unexpected(add_entry_res.error());
    }

    return {};
}