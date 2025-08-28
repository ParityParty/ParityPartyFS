#include <string>
#include <vector>
#include "disk/idisk.hpp"

struct SuperBlock {
    unsigned int num_blocks;
    unsigned int block_size;
    unsigned int fat_block_start;
    unsigned int fat_size;

    SuperBlock(unsigned int num_blocks, unsigned int block_size, unsigned int fat_block_start, unsigned int fat_num_blocks);

    std::vector<std::byte> toBytes() const;

    static SuperBlock fromBytes(const std::vector<std::byte>& bytes);
};

struct DirectoryEntry {
    std::string file_name;
    int start_block;
};

class FileAllocationTable {
    std::vector<int> _fat;
    std::vector<bool> _dirty_entries;

public:
    FileAllocationTable(std::vector<int> fat, std::vector<bool> dirty_entries);
    FileAllocationTable(std::vector<int> fat);

    static FileAllocationTable fromBytes(const std::vector<std::byte> &bytes);
    std::vector<std::byte> toBytes() const;
    std::expected<void, DiskError> updateFat(IDisk &disk, size_t fat_start_address);

    bool operator==(const FileAllocationTable &other) const;
    void setValue(int index, int value);
    int operator[](int index) const;

    static constexpr int FREE_BLOCK = 0;
    static constexpr int LAST_BLOCK = -1;
};