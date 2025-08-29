#pragma once
#include "disk/idisk.hpp"
#include "ppfs/layout.hpp"
#include <string>
#include <vector>

#include <array>

struct SuperBlock {
    unsigned int num_blocks;
    unsigned int block_size;
    unsigned int fat_block_start;
    unsigned int fat_size;

    SuperBlock(unsigned int num_blocks, unsigned int block_size, unsigned int fat_block_start,
        unsigned int fat_num_blocks);

    std::vector<std::byte> toBytes() const;

    static SuperBlock fromBytes(const std::vector<std::byte>& bytes);
};

class FileAllocationTable {
    std::vector<int> _fat;
    std::vector<bool> _dirty_entries;

public:
    FileAllocationTable(std::vector<int> fat, std::vector<bool> dirty_entries);
    FileAllocationTable(std::vector<int> fat);

    static FileAllocationTable fromBytes(const std::vector<std::byte>& bytes);
    std::vector<std::byte> toBytes() const;
    std::expected<void, DiskError> updateFat(IDisk& disk, size_t fat_start_address);

    bool operator==(const FileAllocationTable& other) const;
    void setValue(int index, int value);
    int operator[](int index) const;

    static constexpr int FREE_BLOCK = 0;
    static constexpr int LAST_BLOCK = -1;
};

struct DirectoryEntry {
    std::array<char, Layout::DIR_ENTRY_NAME_SIZE> file_name {};
    int start_block;

    static DirectoryEntry fromBytes(const std::array<std::byte, Layout::DIR_ENTRY_SIZE>& bytes);
    std::array<std::byte, Layout::DIR_ENTRY_SIZE> toBytes() const;

    DirectoryEntry(const std::string& name, int start_block);
    DirectoryEntry(const std::array<char, Layout::DIR_ENTRY_NAME_SIZE>& name, int start_block);
    DirectoryEntry();

    bool operator==(const DirectoryEntry& other) const;
};

struct Directory {
    std::array<char, Layout::DIR_NAME_SIZE> name;
    std::vector<DirectoryEntry> entries;

    Directory(const std::array<char, Layout::DIR_NAME_SIZE>& name,
        const std::vector<DirectoryEntry>& entries);
    Directory(const std::string& name, const std::vector<DirectoryEntry>& entries);
    static Directory fromBytes(const std::vector<std::byte>& bytes);
    std::vector<std::byte> toBytes() const;
};
