#pragma once

#include "ppfs/blockdevice/iblock_device.hpp"
#include "ppfs/common/static_vector.hpp"
#include "ppfs/directory_manager/idirectory_manager.hpp"
#include "ppfs/file_io/file_io.hpp"
#include <optional>

/**
 * Manages directory entries and operations.
 */
class DirectoryManager : public IDirectoryManager {
    IBlockDevice& _block_device;
    IInodeManager& _inode_manager;
    FileIO& _file_io;

    [[nodiscard]] std::expected<void, FsError> _readDirectoryData(inode_index_t inode_index,
        Inode& dir_inode, static_vector<DirectoryEntry>& buf, size_t offset, size_t size);
    std::optional<std::pair<size_t, DirectoryEntry>> _findEntryByName(
        const static_vector<DirectoryEntry>& entries, char const* name);
    std::optional<std::pair<size_t, DirectoryEntry>> _findEntryByInode(
        const static_vector<DirectoryEntry>& entries, inode_index_t inode);
    [[nodiscard]] std::expected<Inode, FsError> _getDirectoryInode(inode_index_t inode_index);

public:
    DirectoryManager(IBlockDevice& block_device, IInodeManager& inode_manager, FileIO& file_io);

    [[nodiscard]] std::expected<void, FsError> getEntries(inode_index_t inode,
        std::uint32_t elements, std::uint32_t offset, static_vector<DirectoryEntry>& buf) override;

    [[nodiscard]] virtual std::expected<void, FsError> addEntry(
        inode_index_t directory, DirectoryEntry entry) override;

    [[nodiscard]] virtual std::expected<void, FsError> removeEntry(
        inode_index_t directory, inode_index_t entry) override;

    [[nodiscard]] virtual std::expected<void, FsError> checkNameUnique(
        inode_index_t directory, const char* name) override;

    [[nodiscard]] virtual std::expected<inode_index_t, FsError> getInodeByName(
        inode_index_t directory, const char* name) override;
};