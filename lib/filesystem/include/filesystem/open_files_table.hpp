#pragma once
#include "common/types.hpp"

#include <array>
#include <optional>

struct OpenFile {
    inode_index_t inode;
    size_t position;
};

template <std::size_t N = 32> class OpenFilesTable {
    std::array<std::optional<OpenFile>, N> _table;

public:
    std::optional<OpenFile>& get(inode_index_t inode)
    {
        for (auto& entry : _table) {
            if (entry.has_value() && entry->inode == inode) {
                return entry;
            }
        }
        return {};
    }
};
