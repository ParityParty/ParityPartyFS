#include "ppfs/data_structures.hpp"
#include "ppfs/ppfs.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>

SuperBlock::SuperBlock(unsigned int num_blocks, unsigned int block_size,
    unsigned int fat_block_start, unsigned int fat_size)
    : num_blocks(num_blocks)
    , block_size(block_size)
    , fat_block_start(fat_block_start)
    , fat_size(fat_size)
{
}

void appendBytes(std::vector<std::byte>& out, const void* src, size_t size)
{
    auto p = reinterpret_cast<const std::byte*>(src);
    out.insert(out.end(), p, p + size);
}

std::vector<std::byte> SuperBlock::toBytes() const
{
    std::vector<std::byte> out;
    appendBytes(out, &num_blocks, sizeof(num_blocks));
    appendBytes(out, &block_size, sizeof(block_size));
    appendBytes(out, &fat_block_start, sizeof(fat_block_start));
    appendBytes(out, &fat_size, sizeof(fat_size));

    return out;
}

SuperBlock SuperBlock::fromBytes(const std::vector<std::byte>& bytes)
{

    size_t offset = 0;

    unsigned int num_blocks;
    std::memcpy(&num_blocks, bytes.data() + offset, sizeof(num_blocks));
    offset += sizeof(num_blocks);

    unsigned int block_size;
    std::memcpy(&block_size, bytes.data() + offset, sizeof(block_size));
    offset += sizeof(block_size);

    unsigned int fat_block_start;
    std::memcpy(&fat_block_start, bytes.data() + offset, sizeof(fat_block_start));
    offset += sizeof(fat_block_start);

    unsigned int fat_size;
    std::memcpy(&fat_size, bytes.data() + offset, sizeof(fat_size));
    // offset += sizeof(fat_size);

    return { num_blocks, block_size, fat_block_start, fat_size };
}
FileAllocationTable::FileAllocationTable(std::vector<int> fat, std::vector<bool> dirty_entries)
    : _fat(std::move(fat))
    , _dirty_entries(std::move(dirty_entries))
{
}
FileAllocationTable::FileAllocationTable(std::vector<int> fat)
    : _fat(std::move(fat))
    , _dirty_entries(_fat.size(), false)
{
}

FileAllocationTable FileAllocationTable::fromBytes(const std::vector<std::byte>& bytes)
{
    const int entries = bytes.size() / sizeof(int);

    auto fat = std::vector<int>(entries);
    auto dirty_entries = std::vector(entries, false);

    std::memcpy(fat.data(), bytes.data(), bytes.size());

    // move so there is no copying data
    return { std::move(fat), std::move(dirty_entries) };
}

std::vector<std::byte> FileAllocationTable::toBytes() const
{
    const size_t bytes_count = _fat.size() * sizeof(int);
    std::vector<std::byte> out(bytes_count);

    std::memcpy(out.data(), _fat.data(), bytes_count);
    return out;
}

std::expected<void, DiskError> FileAllocationTable::updateFat(
    IBlockDevice& block_device, const size_t fat_start_address)
{
    auto bytes = std::vector<std::byte>(sizeof(int));
    for (int i = 0; i < _dirty_entries.size(); i++) {
        if (!_dirty_entries[i])
            continue;
        std::memcpy(bytes.data(), &_fat[i], sizeof(int));
        auto ret = block_device.writeBlock(bytes, fat_start_address 
            / block_device.rawBlockSize(), i * sizeof(int));
        if (!ret.has_value()) {
            return std::unexpected(DiskError::IOError);
        }
        _dirty_entries[i] = false;
    }
    return {};
}
bool FileAllocationTable::operator==(const FileAllocationTable& other) const
{
    return _dirty_entries == other._dirty_entries && _fat == other._fat;
}

void FileAllocationTable::setValue(int index, int value)
{
    _fat[index] = value;
    _dirty_entries[index] = true;
}

int FileAllocationTable::operator[](const int index) const { return _fat[index]; }

std::expected<block_index_t, FatError> FileAllocationTable::findFreeBlock() const
{
    block_index_t i = 0;
    while (++i < _fat.size() && _fat[i] != FREE_BLOCK)
        ;
    if (i == _fat.size()) {
        return std::unexpected(FatError::OutOfDiskSpace);
    }
    return i;
}
std::expected<void, FatError> FileAllocationTable::freeBlocksFrom(block_index_t first)
{
    auto block = first;

    // in case there is a loop in fat
    size_t max_iters = _fat.size();
    size_t iters = 0;

    block_index_t next;

    while (iters++ < max_iters && _fat[block] != LAST_BLOCK) {
        next = _fat[block];
        _fat[block] = FREE_BLOCK;
        _dirty_entries[block] = true;
        block = next;
    }
    if (_fat[block] != LAST_BLOCK) {
        std::cerr << "Error while freeing blocks. Probably a loop in fat";
        return std::unexpected(FatError::Internal);
    }
    _fat[block] = FREE_BLOCK;
    _dirty_entries[block] = true;
    return {};
}

DirectoryEntry DirectoryEntry::fromBytes(const std::array<std::byte, Layout::DIR_ENTRY_SIZE>& bytes)
{
    file_name_t file_name;
    size_t offset = 0;
    std::memcpy(&file_name, bytes.data(), Layout::DIR_ENTRY_NAME_SIZE);

    offset += Layout::DIR_ENTRY_NAME_SIZE;
    block_index_t start_block;
    std::memcpy(&start_block, bytes.data() + offset, sizeof(start_block));

    size_t file_size;
    offset += Layout::DIR_ENTRY_START_BLOCK_INDEX_SIZE;
    std::memcpy(&file_size, bytes.data() + offset, sizeof(file_size));

    return { file_name, start_block, file_size };
}

std::array<std::byte, Layout::DIR_ENTRY_SIZE> DirectoryEntry::toBytes() const
{
    std::array<std::byte, Layout::DIR_ENTRY_SIZE> out {};

    // Write name including /0
    size_t offset = 0;
    std::memcpy(out.data(), file_name.data(), std::strlen(file_name.data()) + 1);
    offset += Layout::DIR_ENTRY_NAME_SIZE;
    std::memcpy(out.data() + offset, &start_block, sizeof(start_block));
    offset += Layout::DIR_ENTRY_START_BLOCK_INDEX_SIZE;
    std::memcpy(out.data() + offset, &file_size, sizeof(file_size));

    return out;
}

DirectoryEntry::DirectoryEntry(const std::string& name, int start_block, size_t file_size)
    : start_block(start_block)
    , file_size(file_size)
{
    const size_t name_end = std::min(Layout::DIR_ENTRY_NAME_SIZE - 1, name.size());
    std::memcpy(file_name.data(), name.data(), name_end);

    // cut the name to size
    file_name[name_end] = '\0';
}

DirectoryEntry::DirectoryEntry(
    const std::array<char, Layout::DIR_ENTRY_NAME_SIZE>& name, int start_block, size_t file_size)
    : file_name(name)
    , start_block(start_block)
    , file_size(file_size)
{
}

DirectoryEntry::DirectoryEntry()
    : start_block(FileAllocationTable::FREE_BLOCK)
    , file_size(0)
{
}

bool DirectoryEntry::operator==(const DirectoryEntry& other) const
{
    return std::strcmp(file_name.data(), other.file_name.data()) == 0
        && start_block == other.start_block && file_size == other.file_size;
}
std::string DirectoryEntry::fileNameStr() const { return { file_name.data() }; }

Directory::Directory(
    const std::array<char, Layout::DIR_NAME_SIZE>& name, const std::vector<DirectoryEntry>& entries)
    : name(name)
    , entries(entries)
{
}

Directory::Directory(const std::string& name, const std::vector<DirectoryEntry>& entries)
    : entries(entries)
{
    const size_t name_end = std::min(Layout::DIR_ENTRY_NAME_SIZE - 1, name.size());
    std::memcpy(this->name.data(), name.data(), name_end);

    this->name[name_end] = '\0';
}

Directory Directory::fromBytes(const std::vector<std::byte>& bytes)
{
    std::array<char, Layout::DIR_NAME_SIZE> name;
    std::memcpy(name.data(), bytes.data(), Layout::DIR_NAME_SIZE);

    num_entries_t num_entries;
    std::memcpy(&num_entries, bytes.data() + Layout::DIR_NAME_SIZE, sizeof(num_entries));

    std::vector<DirectoryEntry> entries(num_entries);

    std::array<std::byte, Layout::DIR_ENTRY_SIZE> entry_bytes;
    for (int i = 0; i < num_entries; i++) {
        auto entry_begin = bytes.data() + Layout::DIR_HEADER_SIZE + i * Layout::DIR_ENTRY_SIZE;
        std::memcpy(entry_bytes.data(), entry_begin, Layout::DIR_ENTRY_SIZE);

        entries[i] = DirectoryEntry::fromBytes(entry_bytes);
    }

    return { name, entries };
}

std::vector<std::byte> Directory::toBytes() const
{
    std::vector<std::byte> bytes(Layout::DIR_HEADER_SIZE + entries.size() * Layout::DIR_ENTRY_SIZE);

    num_entries_t num_entries = entries.size();
    // copy header
    std::memcpy(bytes.data(), name.data(), std::strlen(name.data()) + 1);
    std::memcpy(bytes.data() + Layout::DIR_NAME_SIZE, &num_entries, sizeof(int));

    // copy entries
    for (int i = 0; i < entries.size(); i++) {
        std::memcpy(bytes.data() + Layout::DIR_HEADER_SIZE + i * Layout::DIR_ENTRY_SIZE,
            entries[i].toBytes().data(), Layout::DIR_ENTRY_SIZE);
    }

    return bytes;
}
std::optional<std::reference_wrapper<DirectoryEntry>> Directory::findFile(const std::string& name)
{
    int i;
    for (i = 0; i < entries.size(); i++) {
        if (entries[i].fileNameStr() == name) {
            return std::ref(entries[i]);
        }
    }
    return std::nullopt;
}
void Directory::addEntry(const DirectoryEntry& entry) { entries.push_back(entry); }

void Directory::removeEntry(const DirectoryEntry& entry)
{
    entries.erase(std::find(entries.begin(), entries.end(), entry));
}
void Directory::changeEntry(const DirectoryEntry& entry, const DirectoryEntry& new_entry)
{
    removeEntry(entry);
    addEntry(new_entry);
}
