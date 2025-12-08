#pragma once

#include "blockdevice/iblock_device.hpp"
#include "directory_manager/idirectory_manager.hpp"
#include "file_io/file_io.hpp"

class DirectoryManager : public IDirectoryManager {
    IBlockDevice& _block_device;
    IInodeManager& _inode_manager;
    FileIO& _file_io;

    [[nodiscard]] std::expected<std::vector<DirectoryEntry>, FsError> _readDirectoryData(
        inode_index_t inode_index, Inode& dir_inode);
    int _findEntryIndexByName(const std::vector<DirectoryEntry>& entries, char const* name);
    int _findEntryIndexByInode(const std::vector<DirectoryEntry>& entries, inode_index_t inode);
    [[nodiscard]] std::expected<Inode, FsError> _getDirectoryInode(inode_index_t inode_index);

public:
    DirectoryManager(IBlockDevice& block_device, IInodeManager& inode_manager, FileIO& file_io);

    [[nodiscard]] virtual std::expected<std::vector<DirectoryEntry>, FsError> getEntries(
        inode_index_t inode) override;

    [[nodiscard]] virtual std::expected<void, FsError> addEntry(
        inode_index_t directory, DirectoryEntry entry) override;

    [[nodiscard]] virtual std::expected<void, FsError> removeEntry(
        inode_index_t directory, inode_index_t entry) override;

    [[nodiscard]] virtual std::expected<Inode, FsError> checkNameUnique(
        inode_index_t directory, const char* name) override;
};