#include "filesystem/ppfs_linux.hpp"
#include "mutex_wrapper.hpp"

#include <cstring>

PpFSLinux::PpFSLinux(IDisk& disk)
    : PpFS(disk)
{
}

std::expected<FileAttributes, FsError> PpFSLinux::getAttributes(inode_index_t inode_index)
{
    return mutex_wrapper<FileAttributes>(
        _mutex, [&]() { return _unprotectedGetAttributes(inode_index); });
}

std::expected<inode_index_t, FsError> PpFSLinux::lookup(
    inode_index_t parent_index, std::string_view name)
{
    return mutex_wrapper<inode_index_t>(
        _mutex, [&]() { return _unprotectedLookup(parent_index, name); });
}

std::expected<std::vector<DirectoryEntry>, FsError> PpFSLinux::getDirectoryEntries(
    inode_index_t inode)
{
    return mutex_wrapper<std::vector<DirectoryEntry>>(
        _mutex, [&]() { return _unprotectedGetDirectoryEntries(inode); });
}

std::expected<inode_index_t, FsError> PpFSLinux::createDirectoryByParent(
    inode_index_t parent, std::string_view name)
{
    return mutex_wrapper<inode_index_t>(
        _mutex, [&]() { return _unprotectedCreateDirectoryByParent(parent, name); });
}

std::expected<file_descriptor_t, FsError> PpFSLinux::openByInode(inode_index_t inode, OpenMode mode)
{
    return mutex_wrapper<file_descriptor_t>(
        _mutex, [&]() { return _unprotectedOpenByInode(inode, mode); });
}

std::expected<inode_index_t, FsError> PpFSLinux::createWithParentInode(
    std::string_view name, inode_index_t parent)
{
    return mutex_wrapper<file_descriptor_t>(
        _mutex, [&]() { return _unprotectedCreateWithParentInode(name, parent); });
}

std::expected<FileAttributes, FsError> PpFSLinux::_unprotectedGetAttributes(
    inode_index_t inode_index)
{
    std::cout << "Getattrs for inode: " << inode_index << std::endl;
    if (!isInitialized()) {
        return std::unexpected(FsError::PpFS_NotInitialized);
    }

    auto inode_res = _inodeManager->get(inode_index);

    if (!inode_res.has_value())
        return std::unexpected(inode_res.error());

    return FileAttributes {
        .size = inode_res.value().file_size,
        .block_size = _blockDevice->dataSize(),
        .type = inode_res.value().type,
    };
}

std::expected<inode_index_t, FsError> PpFSLinux::_unprotectedLookup(
    inode_index_t parent_index, std::string_view name)
{
    std::cout << "Lookup for inode: " << parent_index << std::endl;
    std::cout << "Name : " << name << std::endl;
    if (!isInitialized())
        return std::unexpected(FsError::PpFS_NotInitialized);
    auto lookup_res = _getInodeFromParent(parent_index, name);

    return lookup_res;
}

std::expected<inode_index_t, FsError> PpFSLinux::_unprotectedCreateDirectoryByParent(
    inode_index_t parent, std::string_view name)
{
    std::cout << "Trying to create file named: " << name << " in parent: " << parent << std::endl;
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
    std::cout << "Great success" << std::endl;
    return new_inode_index;
}

std::expected<std::vector<DirectoryEntry>, FsError> PpFSLinux::_unprotectedGetDirectoryEntries(
    inode_index_t inode)
{
    if (!isInitialized()) {
        return std::unexpected(FsError::PpFS_NotInitialized);
    }

    return _directoryManager->getEntries(inode);
}

std::expected<file_descriptor_t, FsError> PpFSLinux::_unprotectedOpenByInode(
    inode_index_t inode, OpenMode mode)
{
    if (!isInitialized()) {
        return std::unexpected(FsError::PpFS_NotInitialized);
    }
    auto open_res = _openFilesTable.open(inode, mode);
    if (!open_res.has_value()) {
        return std::unexpected(open_res.error());
    }

    return open_res.value();
}

std::expected<inode_index_t, FsError> PpFSLinux::_unprotectedCreateWithParentInode(
    std::string_view name, inode_index_t parent)
{
    if (!isInitialized()) {
        return std::unexpected(FsError::PpFS_NotInitialized);
    }

    auto unique_res = _directoryManager->checkNameUnique(parent, name.data());
    if (!unique_res.has_value()) {
        return std::unexpected(unique_res.error());
    }

    // Create new inode
    Inode new_inode { .type = InodeType::File };
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

    return new_inode_index;
}
