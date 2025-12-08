#pragma once

#include "blockdevice/iblock_device.hpp"
#include "common/static_vector.hpp"
#include "directory_manager/idirectory_manager.hpp"
#include "file_io/file_io.hpp"

class DirectoryManager : public IDirectoryManager {
    IBlockDevice& _block_device;
    IInodeManager& _inode_manager;
    FileIO& _file_io;

    std::expected<void, FsError> _readDirectoryData(
        inode_index_t inode_index, Inode& dir_inode, buffer<DirectoryEntry>& buf);
    int _findEntryIndexByName(const buffer<DirectoryEntry>& entries, char const* name);
    int _findEntryIndexByInode(const buffer<DirectoryEntry>& entries, inode_index_t inode);
    std::expected<Inode, FsError> _getDirectoryInode(inode_index_t inode_index);

public:
    DirectoryManager(IBlockDevice& block_device, IInodeManager& inode_manager, FileIO& file_io);

    std::expected<void, FsError> getEntries(
        inode_index_t inode, buffer<DirectoryEntry>& buf) override;

    std::expected<void, FsError> addEntry(inode_index_t directory, DirectoryEntry entry) override;

    std::expected<void, FsError> removeEntry(inode_index_t directory, inode_index_t entry) override;
};