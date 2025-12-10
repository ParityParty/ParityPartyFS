#pragma once
#include "common/types.hpp"
#include "filesystem/types.hpp"

#include <array>
#include <expected>
#include <optional>

struct OpenFile {
    inode_index_t inode;
    std::size_t position;
    OpenMode mode;
};

template <std::size_t MAX> class OpenFilesTable {
    std::array<std::optional<OpenFile>, MAX> _table;

public:
    std::optional<OpenFile*> get(file_descriptor_t fd)
    {
        if (fd >= MAX) {
            return { };
        }
        auto& entry = _table[fd];
        if (entry.has_value()) {
            return &entry.value();
        }
        return { };
    }

    std::optional<OpenFile*> get(inode_index_t inode)
    {
        for (int i = 0; i < MAX; ++i) {
            auto& entry = _table[i];
            if (entry.has_value() && entry->inode == inode) {
                return &entry.value();
            }
        }
        return { };
    }

    [[nodiscard]] std::expected<file_descriptor_t, FsError> open(inode_index_t inode, OpenMode mode)
    {
        // Check if already open in exclusive/protected mode and save a free spot
        std::optional<int> free_spot;
        for (int i = 0; i < MAX; ++i) {
            auto& entry = _table[i];
            if (entry.has_value() && entry->inode == inode) {
                // Check exclusivity
                if (mode & OpenMode::Exclusive || entry->mode & OpenMode::Exclusive) {
                    return std::unexpected(FsError::PpFS_AlreadyOpen);
                }
                // If protected, check if no other non-protected OpenFiles exist
                if (mode & OpenMode::Protected && !(entry->mode & OpenMode::Protected)) {
                    return std::unexpected(FsError::PpFS_AlreadyOpen);
                }
                // If non-protected, check if no other protected OpenFiles exist
                if (!(mode & OpenMode::Protected) && entry->mode & OpenMode::Protected) {
                    return std::unexpected(FsError::PpFS_AlreadyOpen);
                }
            }
            if (!entry.has_value() && !free_spot.has_value()) {
                free_spot = i;
            }
        }

        if (free_spot.has_value()) {
            _table[free_spot.value()] = OpenFile { inode, 0, mode };
            return free_spot.value();
        }
        return std::unexpected(FsError::PpFS_OpenFilesTableFull);
    }

    [[nodiscard]] std::expected<void, FsError> close(file_descriptor_t fd)
    {
        if (fd >= MAX) {
            return std::unexpected(FsError::PpFS_OutOfBounds);
        }
        auto& entry = _table[fd];
        if (entry.has_value()) {
            entry.reset();
            return { };
        }
        return std::unexpected(FsError::PpFS_NotFound);
    }
};
