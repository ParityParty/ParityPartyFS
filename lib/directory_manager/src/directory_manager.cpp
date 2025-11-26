#include "directory_manager/directory_manager.hpp"

#include <cstring>

DirectoryManager::DirectoryManager(
    IBlockDevice& block_device, IInodeManager& inode_manager, FileIO& file_io)
    : _block_device(block_device)
    , _inode_manager(inode_manager)
    , _file_io(file_io)
{
}

std::expected<std::vector<DirectoryEntry>, FsError> DirectoryManager::getEntries(
    inode_index_t inode)
{
    auto inode_result = _inode_manager.get(inode);
    if (!inode_result.has_value()) {
        return std::unexpected(inode_result.error());
    }

    Inode dir_inode = inode_result.value();

    return _readDirectoryData(dir_inode);
}

std::expected<void, FsError> DirectoryManager::addEntry(
    inode_index_t directory, DirectoryEntry entry)
{
    auto inode_result = _inode_manager.get(directory);
    if (!inode_result.has_value()) {
        return std::unexpected(inode_result.error());
    }

    Inode dir_inode = inode_result.value();

    auto read_res = _readDirectoryData(dir_inode);

    if (!read_res.has_value()) {
        return std::unexpected(read_res.error());
    }

    std::vector<DirectoryEntry> entries = read_res.value();

    int index = _findEntryIndexByName(entries, entry.name.data());
    if (index != -1) {
        return std::unexpected(FsError::NameTaken);
    }

    auto write_res = _file_io.writeFile(dir_inode, dir_inode.file_size,
        std::vector<uint8_t>(reinterpret_cast<uint8_t*>(&entry),
            reinterpret_cast<uint8_t*>(&entry) + sizeof(DirectoryEntry)));

    if (!write_res.has_value())
        return std::unexpected(write_res.error());
    return {};
}

std::expected<void, FsError> DirectoryManager::removeEntry(
    inode_index_t directory, inode_index_t entry)
{
    auto inode_result = _inode_manager.get(directory);
    if (!inode_result.has_value()) {
        return std::unexpected(inode_result.error());
    }

    Inode dir_inode = inode_result.value();

    auto read_res = _readDirectoryData(dir_inode);

    if (!read_res.has_value()) {
        return std::unexpected(read_res.error());
    }

    std::vector<DirectoryEntry> entries = read_res.value();

    int index = _findEntryIndexByInode(entries, entry);
    if (index == -1) {
        return std::unexpected(FsError::NotFound);
    }

    entries.erase(entries.begin() + index);

    std::vector<uint8_t> new_data;
    for (const auto& entry : entries) {
        const uint8_t* entry_ptr = reinterpret_cast<const uint8_t*>(&entry);
        new_data.insert(new_data.end(), entry_ptr, entry_ptr + sizeof(DirectoryEntry));
    }

    auto write_res = _file_io.writeFile(dir_inode, 0, new_data);
    if (!write_res.has_value()) {
        return std::unexpected(write_res.error());
    }

    return {};
}

std::expected<std::vector<DirectoryEntry>, FsError> DirectoryManager::_readDirectoryData(
    Inode& dir_inode)
{
    auto data_result = _file_io.readFile(dir_inode, 0, dir_inode.file_size);
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
        if (std::strncmp(entries[i].name.data(), name, std::strlen(name)) == 0) {
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
