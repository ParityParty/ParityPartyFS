#include "directory_manager/directory_manager.hpp"
#include "blockdevice/iblock_device.hpp"
#include <array>
#include <cstring>
#include <vector>

DirectoryManager::DirectoryManager(
    IBlockDevice& block_device, IInodeManager& inode_manager, FileIO& file_io)
    : _block_device(block_device)
    , _inode_manager(inode_manager)
    , _file_io(file_io)
{
}

[[nodiscard]] std::expected<void, FsError> DirectoryManager::getEntries(
    inode_index_t inode, std::uint32_t elements, std::uint32_t offset, static_vector<DirectoryEntry>& buf)
{
    auto inode_result = _getDirectoryInode(inode);
    if (!inode_result.has_value()) {
        return std::unexpected(inode_result.error());
    }

    Inode dir_inode = inode_result.value();
    
    size_t bytes_to_read = elements * sizeof(DirectoryEntry);
    if (elements == 0)
        bytes_to_read = dir_inode.file_size;
    
    // Check capacity
    size_t expected_count = bytes_to_read / sizeof(DirectoryEntry);
    if (buf.capacity() < expected_count) {
        return std::unexpected(FsError::DirectoryManager_InvalidRequest);
    }
    
    // Read raw bytes
    std::array<uint8_t, MAX_BLOCK_SIZE * 10> read_buffer; // Large enough for directory data
    static_vector<uint8_t> read_data(read_buffer.data(), read_buffer.size());
    auto read_res = _file_io.readFile(inode, dir_inode, offset * sizeof(DirectoryEntry), bytes_to_read, read_data);
    if (!read_res.has_value()) {
        return std::unexpected(read_res.error());
    }
    
    // Convert to DirectoryEntry and fill buf
    size_t count = read_data.size() / sizeof(DirectoryEntry);
    buf.resize(count);
    std::memcpy(buf.data(), read_data.data(), read_data.size());
    
    return {};
}

std::expected<void, FsError> DirectoryManager::addEntry(
    inode_index_t directory, DirectoryEntry entry)
{
    auto inode_result = checkNameUnique(directory, entry.name.data());
    if (!inode_result.has_value()) {
        return std::unexpected(inode_result.error());
    }
    Inode dir_inode = inode_result.value();

    std::array<uint8_t, sizeof(DirectoryEntry)> entry_buffer;
    std::memcpy(entry_buffer.data(), &entry, sizeof(DirectoryEntry));
    static_vector<uint8_t> entry_data(entry_buffer.data(), sizeof(DirectoryEntry), sizeof(DirectoryEntry));
    auto write_res = _file_io.writeFile(directory, dir_inode, dir_inode.file_size, entry_data);

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
    
    // Read directory entries
    std::array<uint8_t, MAX_BLOCK_SIZE * 10> read_buffer;
    static_vector<uint8_t> read_data(read_buffer.data(), read_buffer.size());
    auto read_res = _file_io.readFile(directory, dir_inode, 0, dir_inode.file_size, read_data);
    if (!read_res.has_value()) {
        return std::unexpected(read_res.error());
    }
    
    // Convert to DirectoryEntry view
    size_t count = read_data.size() / sizeof(DirectoryEntry);
    std::array<DirectoryEntry, MAX_BLOCK_SIZE * 10 / sizeof(DirectoryEntry)> entries_buffer;
    static_vector<DirectoryEntry> entries(entries_buffer.data(), entries_buffer.size());
    entries.resize(count);
    std::memcpy(entries.data(), read_data.data(), read_data.size());
    
    int index = _findEntryIndexByInode(entries, entry);
    if (index == -1) {
        return std::unexpected(FsError::DirectoryManager_NotFound);
    }

    auto new_dir_size = dir_inode.file_size - sizeof(DirectoryEntry);
    if (index * sizeof(DirectoryEntry) != new_dir_size) {
        // move last entry to deleted entry position
        std::array<uint8_t, sizeof(DirectoryEntry)> temp_buffer;
        std::memcpy(temp_buffer.data(), &entries[count - 1], sizeof(DirectoryEntry));
        static_vector<uint8_t> temp(temp_buffer.data(), sizeof(DirectoryEntry), sizeof(DirectoryEntry));
        auto write_res
            = _file_io.writeFile(directory, dir_inode, index * sizeof(DirectoryEntry), temp);

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

std::expected<Inode, FsError> DirectoryManager::checkNameUnique(
    inode_index_t directory, const char* name)
{
    auto inode_result = _getDirectoryInode(directory);
    if (!inode_result.has_value()) {
        return std::unexpected(inode_result.error());
    }
    Inode dir_inode = inode_result.value();
    
    // Read directory entries
    std::array<uint8_t, MAX_BLOCK_SIZE * 10> read_buffer;
    static_vector<uint8_t> read_data(read_buffer.data(), read_buffer.size());
    auto read_res = _file_io.readFile(directory, dir_inode, 0, dir_inode.file_size, read_data);
    if (!read_res.has_value()) {
        return std::unexpected(read_res.error());
    }
    
    // Convert to DirectoryEntry view
    size_t count = read_data.size() / sizeof(DirectoryEntry);
    std::array<DirectoryEntry, MAX_BLOCK_SIZE * 10 / sizeof(DirectoryEntry)> entries_buffer;
    static_vector<DirectoryEntry> entries(entries_buffer.data(), entries_buffer.size());
    entries.resize(count);
    std::memcpy(entries.data(), read_data.data(), read_data.size());
    
    int index = _findEntryIndexByName(entries, name);
    if (index != -1) {
        return std::unexpected(FsError::DirectoryManager_NameTaken);
    }
    return dir_inode;
}

std::expected<void, FsError> DirectoryManager::_readDirectoryData(
    inode_index_t inode_index, Inode& dir_inode, static_vector<DirectoryEntry>& buf)
{
    // Read raw bytes
    std::array<uint8_t, MAX_BLOCK_SIZE * 10> read_buffer;
    static_vector<uint8_t> read_data(read_buffer.data(), read_buffer.size());
    auto read_res = _file_io.readFile(inode_index, dir_inode, 0, dir_inode.file_size, read_data);
    if (!read_res.has_value()) {
        return std::unexpected(read_res.error());
    }
    
    // Convert to DirectoryEntry and fill buf
    size_t count = read_data.size() / sizeof(DirectoryEntry);
    buf.resize(count);
    std::memcpy(buf.data(), read_data.data(), read_data.size());
    
    return {};
}

int DirectoryManager::_findEntryIndexByName(const static_vector<DirectoryEntry>& entries, char const* name)
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
    const static_vector<DirectoryEntry>& entries, inode_index_t inode)
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
