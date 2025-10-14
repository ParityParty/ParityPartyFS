#pragma once
#include "disk/idisk.hpp"
#include "ppfs/types.hpp"
#include <array>
#include <optional>
#include <string>
#include <vector>

struct SuperBlock {
    unsigned int num_blocks;
    unsigned int block_size;
    unsigned int fat_block_start;
    unsigned int fat_size;

    SuperBlock(unsigned int num_blocks, unsigned int block_size, unsigned int fat_block_start,
        unsigned int fat_size);
    SuperBlock() = default;

    std::vector<std::byte> toBytes() const;

    static SuperBlock fromBytes(const std::vector<std::byte>& bytes);
};

class FileAllocationTable {
    std::vector<int> _fat;
    std::vector<bool> _dirty_entries;

public:
    FileAllocationTable(std::vector<block_index_t> fat, std::vector<bool> dirty_entries);
    FileAllocationTable(std::vector<block_index_t> fat);
    FileAllocationTable() = default;

    static FileAllocationTable fromBytes(const std::vector<std::byte>& bytes);
    std::vector<std::byte> toBytes() const;
    std::expected<void, DiskError> updateFat(IDisk& disk, size_t fat_start_address);

    bool operator==(const FileAllocationTable& other) const;
    void setValue(int index, int value);
    int operator[](int index) const;

    static constexpr block_index_t FREE_BLOCK = 0;
    static constexpr block_index_t LAST_BLOCK = -1;

    std::expected<block_index_t, FatError> findFreeBlock() const;

    /**
     * Frees blocks starting from first up to LAST_BLOCK
     *
     * @param first first block
     * @return void on success, FatError on error
     */
    std::expected<void, FatError> freeBlocksFrom(block_index_t first);
};

struct DirectoryEntry {
    bool is_directory;
    file_name_t file_name {};
    block_index_t start_block;
    size_t file_size;

    static DirectoryEntry fromBytes(const std::array<std::byte, Layout::DIR_ENTRY_SIZE>& bytes);
    std::array<std::byte, Layout::DIR_ENTRY_SIZE> toBytes() const;

    DirectoryEntry(
        bool is_directory, const std::string& name, block_index_t start_block, size_t file_size);
    DirectoryEntry(
        bool is_directory, const file_name_t& name, block_index_t start_block, size_t file_size);
    DirectoryEntry();

    bool operator==(const DirectoryEntry& other) const;

    std::string fileNameStr() const;
};

struct Directory {
    std::array<char, Layout::DIR_NAME_SIZE> name;
    std::vector<DirectoryEntry> entries;

    Directory(const std::array<char, Layout::DIR_NAME_SIZE>& name,
        const std::vector<DirectoryEntry>& entries);
    Directory(const std::string& name, const std::vector<DirectoryEntry>& entries);
    Directory() = default;

    static Directory fromBytes(const std::vector<std::byte>& bytes);
    std::vector<std::byte> toBytes() const;
    std::optional<std::reference_wrapper<DirectoryEntry>> findFile(const std::string& name);

    void addEntry(const DirectoryEntry& entry);
    void removeEntry(const DirectoryEntry& entry);
    void changeEntry(const DirectoryEntry& entry, const DirectoryEntry& new_entry);
};
