#include "filesystem/ppfs_linux.hpp"

PpFSLinux::PpFSLinux(IDisk& disk)
    : PpFS(disk)
{
}

std::expected<FileAttributes, FsError> PpFSLinux::getAttributes(inode_index_t inode_index)
{
    if (!isInitialized())
        return std::unexpected(FsError::PpFS_NotInitialized);

    auto inode_res = _inodeManager->get(inode_index);
    if (!inode_res.has_value())
        return std::unexpected(inode_res.error());

    return FileAttributes { .size = inode_res.value().file_size, .type = inode_res.value().type };
}

std::expected<inode_index_t, FsError> PpFSLinux::lookup(
    inode_index_t parent_index, const char* name)
{
}
