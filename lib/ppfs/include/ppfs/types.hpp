#pragma once

typedef int num_entries_t;
typedef int block_index_t;

// Memory layout of the file system
namespace Layout {
// First is the superblock
constexpr size_t SUPERBLOCK_START_BLOCK = 0;
constexpr size_t SUPERBLOCK_NUM_BLOCKS = 1;

// Then file allocation table
constexpr size_t FAT_START_BLOCK = SUPERBLOCK_START_BLOCK + SUPERBLOCK_NUM_BLOCKS;

// Then root directory with entries
constexpr size_t DIR_HEADER_SIZE = 128;
constexpr size_t DIR_NUM_ENTRIES_SIZE = sizeof(num_entries_t);
constexpr size_t DIR_NAME_SIZE = DIR_HEADER_SIZE - DIR_NUM_ENTRIES_SIZE;

constexpr size_t DIR_ENTRY_SIZE = 128;
constexpr size_t DIR_ENTRY_FILE_TYPE_SIZE = sizeof(bool);
constexpr size_t DIR_ENTRY_START_BLOCK_INDEX_SIZE = sizeof(block_index_t);
constexpr size_t DIR_ENTRY_FILE_SIZE_SIZE = sizeof(size_t);
constexpr size_t DIR_ENTRY_NAME_SIZE = DIR_ENTRY_SIZE - DIR_ENTRY_FILE_TYPE_SIZE
    - DIR_ENTRY_START_BLOCK_INDEX_SIZE - DIR_ENTRY_FILE_SIZE_SIZE;
}

typedef std::array<char, Layout::DIR_ENTRY_NAME_SIZE> file_name_t;

enum class FatError {
    OutOfDiskSpace,
    Internal,
};