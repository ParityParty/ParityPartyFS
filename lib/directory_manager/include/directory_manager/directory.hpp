#pragma once
#include "common/types.hpp"
#include <array>

struct DirectoryEntry {
    inode_index_t inode;
    // TODO: do something more clever
    std::array<char, 128 - sizeof(inode_index_t)> name;
};
