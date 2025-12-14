#include "directory_manager/directory_manager.hpp"
#include "blockdevice/iblock_device.hpp"
#include <array>
#include <cstring>
#include <vector>

// number of entries we read in one batch to find given entry
#define ENTRY_BATCH_SIZE 256

DirectoryManager::DirectoryManager(
    IBlockDevice& block_device, IInodeManager& inode_manager, FileIO& file_io)
    : _block_device(block_device)
    , _inode_manager(inode_manager)
    , _file_io(file_io)
{
}

[[nodiscard]] std::expected<void, FsError> DirectoryManager::getEntries(inode_index_t inode,
    std::uint32_t elements, std::uint32_t offset, static_vector<DirectoryEntry>& buf)
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
    auto read_res = _file_io.readFile(
        inode, dir_inode, offset * sizeof(DirectoryEntry), bytes_to_read, read_data);
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
    auto inode_result = _getDirectoryInode(directory);
    if (!inode_result.has_value()) {
        return std::unexpected(inode_result.error());
    }
    Inode dir_inode = inode_result.value();

    std::array<uint8_t, sizeof(DirectoryEntry)> entry_buffer;
    std::memcpy(entry_buffer.data(), &entry, sizeof(DirectoryEntry));
    static_vector<uint8_t> entry_data(
        entry_buffer.data(), sizeof(DirectoryEntry), sizeof(DirectoryEntry));
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

    size_t num_entries = dir_inode.file_size / sizeof(DirectoryEntry);
    size_t checked = 0;
    std::optional<std::pair<size_t, DirectoryEntry>> found_entry;
 
    while (checked < num_entries) {
     std::array<DirectoryEntry, ENTRY_BATCH_SIZE> entries_buffer;
     static_vector<DirectoryEntry> entries(entries_buffer.data(), entries_buffer.size());
     auto read_res = _readDirectoryData(directory, dir_inode, entries, checked, ENTRY_BATCH_SIZE);
     if (!read_res.has_value()) 
         return std::unexpected(read_res.error());
 
     found_entry = _findEntryByInode(entries, entry);
 
     if (found_entry.has_value()){
        found_entry.value().first += checked;
        break;
     }
 
     checked += entries.size();
    }

    if (!found_entry.has_value())
        return std::unexpected(FsError::DirectoryManager_NotFound);

    auto new_dir_size = dir_inode.file_size - sizeof(DirectoryEntry);
    if (found_entry.value().first * sizeof(DirectoryEntry) != new_dir_size) {
        // move last entry to deleted entry position
        size_t last_entry_index = (dir_inode.file_size / sizeof(DirectoryEntry)) - 1;
        std::array<DirectoryEntry, 1> last_entry_buffer;
        static_vector<DirectoryEntry> last_entry(last_entry_buffer.data(), 1);
        auto read_last_res = _readDirectoryData(directory, dir_inode, last_entry, last_entry_index, 1);
        if (!read_last_res.has_value()) {
            return std::unexpected(read_last_res.error());
        }
        
        std::array<uint8_t, sizeof(DirectoryEntry)> temp_buffer;
        std::memcpy(temp_buffer.data(), &last_entry[0], sizeof(DirectoryEntry));
        static_vector<uint8_t> temp(
            temp_buffer.data(), sizeof(DirectoryEntry), sizeof(DirectoryEntry));
        auto write_res
            = _file_io.writeFile(directory, dir_inode, found_entry.value().first * sizeof(DirectoryEntry), temp);

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

std::expected<void, FsError> DirectoryManager::checkNameUnique(
    inode_index_t directory, const char* name)
{
    auto unique_res = getInodeByName(directory, name);
    if (!unique_res.has_value() && unique_res.error() != FsError::PpFS_NotFound) {
        return std::unexpected(unique_res.error());
    }
    if (unique_res.has_value()) {
        return std::unexpected(FsError::DirectoryManager_NameTaken);
    }
    return {};
}

std::expected<inode_index_t, FsError> DirectoryManager::getInodeByName(
    inode_index_t directory, const char* name)
{
    auto inode_result = _getDirectoryInode(directory);
    if (!inode_result.has_value()) {
        return std::unexpected(inode_result.error());
    }
    Inode dir_inode = inode_result.value();

   size_t num_entries = dir_inode.file_size / sizeof(DirectoryEntry);
   size_t checked = 0;

   while (checked < num_entries) {
    std::array<DirectoryEntry, ENTRY_BATCH_SIZE> entries_buffer;
    static_vector<DirectoryEntry> entries(entries_buffer.data(), entries_buffer.size());
    auto read_res = _readDirectoryData(directory, dir_inode, entries, checked, ENTRY_BATCH_SIZE);
    if (!read_res.has_value()) 
        return std::unexpected(read_res.error());

    auto unique_res = _findEntryByName(entries, name);

    if (unique_res.has_value())
        return unique_res.value().second.inode;

    checked += entries.size();
}
    return std::unexpected(FsError::PpFS_NotFound);
}

std::expected<void, FsError> DirectoryManager::_readDirectoryData(inode_index_t inode_index,
    Inode& dir_inode, static_vector<DirectoryEntry>& buf, size_t offset, size_t size)
{
    size_t max_entries = dir_inode.file_size / sizeof(DirectoryEntry);
    if (offset >= max_entries) {
        buf.resize(0);
        return {};
    }
    size = std::min(size, max_entries - offset);
    if (buf.capacity() < size)
        return std::unexpected(FsError::DirectoryManager_InvalidRequest);

    static_vector<uint8_t> read_data((uint8_t*)buf.data(), buf.capacity() * sizeof(DirectoryEntry));
    auto read_res = _file_io.readFile(inode_index, dir_inode, offset * sizeof(DirectoryEntry), size * sizeof(DirectoryEntry), read_data);
    if (!read_res.has_value()) {
        return std::unexpected(read_res.error());
    }

    buf.resize(read_data.size() / sizeof(DirectoryEntry));

    return {};
}

std::optional<std::pair<size_t, DirectoryEntry>> DirectoryManager::_findEntryByName(
    const static_vector<DirectoryEntry>& entries, char const* name)
{
    for (size_t i = 0; i < entries.size(); ++i) {
        if (std::strlen(name) == std::strlen(entries[i].name.data())
            && std::strncmp(entries[i].name.data(), name, std::strlen(name)) == 0) {
            return std::make_pair(i, entries[i]);
        }
    }
    return {};
}

 std::optional<std::pair<size_t, DirectoryEntry>> DirectoryManager::_findEntryByInode(
    const static_vector<DirectoryEntry>& entries, inode_index_t inode)
{
    for (size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].inode == inode) {
            return std::make_pair(i, entries[i]);
        }
    }
    return {};
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
