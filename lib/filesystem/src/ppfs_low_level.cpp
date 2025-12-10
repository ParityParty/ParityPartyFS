#include "filesystem/ppfs_low_level.hpp"
#include "filesystem/mutex_wrapper.hpp"

#include <cstring>

PpFSLowLevel::PpFSLowLevel(IDisk& disk)
    : PpFS(disk)
{
}

std::expected<FileAttributes, FsError> PpFSLowLevel::getAttributes(inode_index_t inode_index)
{
    return mutex_wrapper<FileAttributes>(
        _mutex, [&]() { return _unprotectedGetAttributes(inode_index); });
}

std::expected<inode_index_t, FsError> PpFSLowLevel::lookup(
    inode_index_t parent_index, std::string_view name)
{
    return mutex_wrapper<inode_index_t>(
        _mutex, [&]() { return _unprotectedLookup(parent_index, name); });
}

std::expected<std::vector<DirectoryEntry>, FsError> PpFSLowLevel::getDirectoryEntries(
    inode_index_t inode)
{
    return mutex_wrapper<std::vector<DirectoryEntry>>(
        _mutex, [&]() { return _unprotectedGetDirectoryEntries(inode); });
}

std::expected<inode_index_t, FsError> PpFSLowLevel::createDirectoryByParent(
    inode_index_t parent, std::string_view name)
{
    return mutex_wrapper<inode_index_t>(
        _mutex, [&]() { return _unprotectedCreateDirectoryByParent(parent, name); });
}

std::expected<void, FsError> PpFSLowLevel::removeByNameAndParent(
    inode_index_t parent, std::string_view name, bool recursive)
{
    return mutex_wrapper<void>(
        _mutex, [&]() { return _unprotectedRemoveByNameAndParent(parent, name, recursive); });
}

std::expected<file_descriptor_t, FsError> PpFSLowLevel::openByInode(
    inode_index_t inode, OpenMode mode)
{
    return mutex_wrapper<file_descriptor_t>(
        _mutex, [&]() { return _unprotectedOpenByInode(inode, mode); });
}

std::expected<inode_index_t, FsError> PpFSLowLevel::createWithParentInode(
    std::string_view name, inode_index_t parent)
{
    return mutex_wrapper<file_descriptor_t>(
        _mutex, [&]() { return _unprotectedCreateWithParentInode(name, parent); });
}

std::expected<FileAttributes, FsError> PpFSLowLevel::_unprotectedGetAttributes(
    inode_index_t inode_index)
{
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

std::expected<inode_index_t, FsError> PpFSLowLevel::_unprotectedLookup(
    inode_index_t parent_index, std::string_view name)
{
    if (!isInitialized())
        return std::unexpected(FsError::PpFS_NotInitialized);
    auto lookup_res = _getInodeFromParent(parent_index, name);

    return lookup_res;
}

std::expected<inode_index_t, FsError> PpFSLowLevel::_unprotectedCreateDirectoryByParent(
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
    return new_inode_index;
}

std::expected<std::vector<DirectoryEntry>, FsError> PpFSLowLevel::_unprotectedGetDirectoryEntries(
    inode_index_t inode)
{
    if (!isInitialized()) {
        return std::unexpected(FsError::PpFS_NotInitialized);
    }

    return _directoryManager->getEntries(inode);
}

std::expected<file_descriptor_t, FsError> PpFSLowLevel::_unprotectedOpenByInode(
    inode_index_t inode, OpenMode mode)
{
    if (!isInitialized()) {
        return std::unexpected(FsError::PpFS_NotInitialized);
    }

    auto inode_res = _inodeManager->get(inode);
    if (!inode_res.has_value())
        return std::unexpected(inode_res.error());

    if (inode_res.value().type != InodeType::File)
        return std::unexpected(FsError::PpFS_InvalidRequest);

    auto open_res = _openFilesTable.open(inode, mode);
    if (!open_res.has_value()) {
        return std::unexpected(open_res.error());
    }

    return open_res.value();
}

std::expected<inode_index_t, FsError> PpFSLowLevel::_unprotectedCreateWithParentInode(
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

std::expected<void, FsError> PpFSLowLevel::_unprotectedRemoveByNameAndParent(
    inode_index_t parent, std::string_view name, bool recursive)
{
    auto inode_res = _getInodeFromParent(parent, name);
    if (!inode_res.has_value()) {
        return std::unexpected(inode_res.error());
    }
    inode_index_t inode = inode_res.value();

    if (!recursive) {
        // If directory, check if empty
        auto inode_data_res = _inodeManager->get(inode);
        if (!inode_data_res.has_value()) {
            return std::unexpected(inode_data_res.error());
        }
        Inode inode_data = inode_data_res.value();
        if (inode_data.type == InodeType::Directory && inode_data.file_size > 0) {
            return std::unexpected(FsError::PpFS_DirectoryNotEmpty);
        }
    }

    auto check_in_use_res = _checkIfInUseRecursive(inode);
    if (!check_in_use_res.has_value()) {
        return std::unexpected(check_in_use_res.error());
    }

    auto remove_res = _removeRecursive(parent, inode);
    if (!remove_res.has_value()) {
        return std::unexpected(remove_res.error());
    }

    return {};
}
