#pragma once
namespace Layout {
    constexpr size_t SUPERBLOCK_NUM_BLOCKS_OFFSET = 0;
    constexpr size_t DIR_NAME_SIZE = 124;
    constexpr size_t DIR_NUM_ENTRIES_SIZE = 4;
    constexpr size_t DIR_HEADER_SIZE = DIR_NAME_SIZE + DIR_NUM_ENTRIES_SIZE;
    constexpr size_t DIR_ENTRY_NAME_SIZE = 124;
    constexpr size_t DIR_ENTRY_NUM_ENTRIES_SIZE = 4;
    constexpr size_t DIR_ENTRY_SIZE = DIR_ENTRY_NAME_SIZE + DIR_ENTRY_NUM_ENTRIES_SIZE;
}