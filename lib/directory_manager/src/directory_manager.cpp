#include "directory_manager/directory_manager.hpp"

#include <cstring>

DirectoryManager::DirectoryManager(
    IBlockDevice& block_device, IInodeManager& inode_manager, FileIO& file_io)
    : _block_device(block_device)
    , _inode_manager(inode_manager)
    , _file_io(file_io)
{
}

std::expected<Inode, FsError> DirectoryManager::checkNameUnique(
    inode_index_t directory, const char* name)
{
    auto inode_result = _getDirectoryInode(directory);
    if (!inode_result.has_value()) {
        return std::unexpected(inode_result.error());
    }
    Inode dir_inode = inode_result.value();

    auto read_res = _readDirectoryData(directory, dir_inode);

    if (!read_res.has_value()) {
        return std::unexpected(read_res.error());
    }

    std::vector<DirectoryEntry> entries = read_res.value();

    int index = _findEntryIndexByName(entries, name);
    if (index != -1) {
        return std::unexpected(FsError::DirectoryManager_NameTaken);
    }
    return dir_inode;
}

std::expected<std::vector<DirectoryEntry>, FsError> DirectoryManager::getEntries(
    inode_index_t inode, std::uint32_t elements, std::uint32_t offset)
{
    auto inode_result = _getDirectoryInode(inode);
    if (!inode_result.has_value()) {
        return std::unexpected(inode_result.error());
    }

    Inode dir_inode = inode_result.value();

    return _readDirectoryData(inode, dir_inode, elements, offset);
}

std::expected<void, FsError> DirectoryManager::addEntry(
    inode_index_t directory, DirectoryEntry entry)
{
    auto inode_result = checkNameUnique(directory, entry.name.data());
    if (!inode_result.has_value()) {
        return std::unexpected(inode_result.error());
    }
    Inode dir_inode = inode_result.value();

    auto write_res = _file_io.writeFile(directory, dir_inode, dir_inode.file_size,
        std::vector<uint8_t>(reinterpret_cast<uint8_t*>(&entry),
            reinterpret_cast<uint8_t*>(&entry) + sizeof(DirectoryEntry)));

    if (!write_res.has_value()) {
        return std::unexpected(write_res.error());
    }
    return {};
}

std::expected<void, FsError> DirectoryManager::removeEntry(
    inode_index_t directory, inode_index_t entry)
{
    auto inode_result = _getDirectoryInode(directory);
    if (!inode_result.has_value()) {
        return std::unexpected(inode_result.error());
    }

    Inode dir_inode = inode_result.value();

    auto read_res = _readDirectoryData(directory, dir_inode);

    if (!read_res.has_value()) {
        return std::unexpected(read_res.error());
    }

    std::vector<DirectoryEntry> entries = read_res.value();
    int index = _findEntryIndexByInode(entries, entry);
    if (index == -1) {
        return std::unexpected(FsError::DirectoryManager_NotFound);
    }

    auto new_dir_size = dir_inode.file_size - sizeof(DirectoryEntry);
    if (index * sizeof(DirectoryEntry) != new_dir_size) {
        // move last entry to deleted entry position
        auto write_res = _file_io.writeFile(directory, dir_inode, index * sizeof(DirectoryEntry),
            std::vector<uint8_t>(reinterpret_cast<uint8_t*>(&entries.back()),
                reinterpret_cast<uint8_t*>(&entries.back()) + sizeof(DirectoryEntry)));

        if (!write_res.has_value()) {
            return std::unexpected(write_res.error());
        }
    }
    auto resize_res = _file_io.resizeFile(directory, dir_inode, new_dir_size);
    if (!resize_res.has_value()) {
        return std::unexpected(resize_res.error());
    }
    return {};
}

std::expected<std::vector<DirectoryEntry>, FsError> DirectoryManager::_readDirectoryData(
    inode_index_t inode_index, Inode& dir_inode, std::uint32_t elements, std::uint32_t offset) const
{
    auto bytes_to_read = elements * sizeof(DirectoryEntry);
    if (elements == 0)
        bytes_to_read = dir_inode.file_size;

    auto data_result
        = _file_io.readFile(inode_index, dir_inode, offset * sizeof(DirectoryEntry), bytes_to_read);
    if (!data_result.has_value()) {
        return std::unexpected(data_result.error());
    }
    std::vector<uint8_t> dir_data = data_result.value();

    size_t count = dir_data.size() / sizeof(DirectoryEntry);
    auto raw_ptr = reinterpret_cast<const DirectoryEntry*>(dir_data.data());
    std::vector<DirectoryEntry> entries;
    entries.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        entries.push_back(raw_ptr[i]);
    }

    return entries;
}

int DirectoryManager::_findEntryIndexByName(
    const std::vector<DirectoryEntry>& entries, char const* name)
{
    for (size_t i = 0; i < entries.size(); ++i) {
        if (std::strlen(name) == std::strlen(entries[i].name.data())
            && std::strncmp(entries[i].name.data(), name, std::strlen(name)) == 0) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int DirectoryManager::_findEntryIndexByInode(
    const std::vector<DirectoryEntry>& entries, inode_index_t inode)
{
    for (size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].inode == inode) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

std::expected<Inode, FsError> DirectoryManager::_getDirectoryInode(inode_index_t inode_index)
{
    auto inode_result = _inode_manager.get(inode_index);
    if (!inode_result.has_value()) {
        return std::unexpected(inode_result.error());
    }

    if (inode_result->type != InodeType::Directory)
        return std::unexpected(FsError::DirectoryManager_InvalidRequest);

    return *inode_result;
}
