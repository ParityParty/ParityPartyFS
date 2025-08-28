#include "ppfs/data_structures.hpp"

#include <cstring>

SuperBlock::SuperBlock(unsigned int num_blocks, unsigned int block_size, unsigned int fat_block_start, unsigned int fat_size):
num_blocks(num_blocks),
block_size(block_size),
fat_block_start(fat_block_start),
fat_size(fat_size){}


void appendBytes(std::vector<std::byte>& out, const void* src, size_t size) {
    auto p = reinterpret_cast<const std::byte*>(src);
    out.insert(out.end(), p, p + size);
}


std::vector<std::byte> SuperBlock::toBytes() const{
    std::vector<std::byte> out;
    appendBytes(out, &num_blocks, sizeof(num_blocks));
    appendBytes(out, &block_size, sizeof(block_size));
    appendBytes(out, &fat_block_start, sizeof(fat_block_start));
    appendBytes(out, &fat_size, sizeof(fat_size));

    return out;
}

SuperBlock SuperBlock::fromBytes(const std::vector<std::byte>& bytes){

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

    return {num_blocks, block_size, fat_block_start, fat_size};
}
FileAllocationTable::FileAllocationTable(
    std::vector<int> fat, std::vector<bool> dirty_entries):_fat(std::move(fat)), _dirty_entries(std::move(dirty_entries)) {
}
FileAllocationTable::FileAllocationTable(std::vector<int> fat): _fat(std::move(fat)), _dirty_entries(fat.size(), false) { }

FileAllocationTable FileAllocationTable::fromBytes(const std::vector<std::byte>& bytes)
{
    const int entries = bytes.size()/sizeof(int);

    auto fat = std::vector<int>(entries);
    auto dirty_entries = std::vector(entries, false);

    std::memcpy(fat.data(), bytes.data(), bytes.size());

    // move so there is no copying data
    return {std::move(fat), std::move(dirty_entries)};
}

std::vector<std::byte> FileAllocationTable::toBytes() const
{
    const size_t bytes_count = _fat.size()*sizeof(int);
    std::vector<std::byte> out(bytes_count);

    std::memcpy(out.data(), _fat.data(), bytes_count);
    return out;
}

std::expected<void, DiskError> FileAllocationTable::updateFat(IDisk& disk, const size_t fat_start_address)
{
    auto bytes = std::vector<std::byte>(sizeof(int));
    for (int i = 0; i < _dirty_entries.size(); i++) {
        if (!_dirty_entries[i]) continue;
        std::memcpy(bytes.data(), &_fat[i], sizeof(int));
        auto ret = disk.write(fat_start_address + i*sizeof(int), bytes);
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
int FileAllocationTable::operator[](const int index) const {
    return _fat[index];
}

