#pragma once
#include "common/types.hpp"
#include <array>

/**
 * Represents a directory entry containing inode reference and filename.
 */
struct DirectoryEntry {
    inode_index_t inode; ///< Index of the inode this entry refers to
    // TODO: do something more clever
    std::array<char, 128 - sizeof(inode_index_t)> name; ///< Null-terminated filename
};
