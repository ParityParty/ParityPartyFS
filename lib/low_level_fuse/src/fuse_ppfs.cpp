#include "low_level_fuse/fuse_ppfs.hpp"

#include <cstring>
#include <iostream>

#define HANDLE_EXPECTED_ERROR(req, result_expr)                                                    \
    do {                                                                                           \
        if (!(result_expr).has_value()) {                                                          \
            int _err = _map_fs_error_to_errno((result_expr).error());                              \
            fuse_reply_err((req), _err);                                                           \
            return;                                                                                \
        }                                                                                          \
    } while (0)

template <class T> low_level_fuse::t_init low_level_fuse::FuseWrapper<T>::init = nullptr;
template <class T> low_level_fuse::t_destroy low_level_fuse::FuseWrapper<T>::destroy = nullptr;
template <class T> low_level_fuse::t_lookup low_level_fuse::FuseWrapper<T>::lookup = nullptr;
template <class T> low_level_fuse::t_forget low_level_fuse::FuseWrapper<T>::forget = nullptr;
template <class T> low_level_fuse::t_getattr low_level_fuse::FuseWrapper<T>::getattr = nullptr;
template <class T> low_level_fuse::t_setattr low_level_fuse::FuseWrapper<T>::setattr = nullptr;
template <class T> low_level_fuse::t_readlink low_level_fuse::FuseWrapper<T>::readlink = nullptr;
template <class T> low_level_fuse::t_mknod low_level_fuse::FuseWrapper<T>::mknod = nullptr;
template <class T> low_level_fuse::t_mkdir low_level_fuse::FuseWrapper<T>::mkdir = nullptr;
template <class T> low_level_fuse::t_unlink low_level_fuse::FuseWrapper<T>::unlink = nullptr;
template <class T> low_level_fuse::t_rmdir low_level_fuse::FuseWrapper<T>::rmdir = nullptr;
template <class T> low_level_fuse::t_symlink low_level_fuse::FuseWrapper<T>::symlink = nullptr;
template <class T> low_level_fuse::t_rename low_level_fuse::FuseWrapper<T>::rename = nullptr;
template <class T> low_level_fuse::t_link low_level_fuse::FuseWrapper<T>::link = nullptr;
template <class T> low_level_fuse::t_open low_level_fuse::FuseWrapper<T>::open = nullptr;
template <class T> low_level_fuse::t_read low_level_fuse::FuseWrapper<T>::read = nullptr;
template <class T> low_level_fuse::t_write low_level_fuse::FuseWrapper<T>::write = nullptr;
template <class T> low_level_fuse::t_flush low_level_fuse::FuseWrapper<T>::flush = nullptr;
template <class T> low_level_fuse::t_release low_level_fuse::FuseWrapper<T>::release = nullptr;
template <class T> low_level_fuse::t_fsync low_level_fuse::FuseWrapper<T>::fsync = nullptr;
template <class T> low_level_fuse::t_opendir low_level_fuse::FuseWrapper<T>::opendir = nullptr;
template <class T> low_level_fuse::t_readdir low_level_fuse::FuseWrapper<T>::readdir = nullptr;
template <class T>
low_level_fuse::t_releasedir low_level_fuse::FuseWrapper<T>::releasedir = nullptr;
template <class T> low_level_fuse::t_fsyncdir low_level_fuse::FuseWrapper<T>::fsyncdir = nullptr;
template <class T> low_level_fuse::t_statfs low_level_fuse::FuseWrapper<T>::statfs = nullptr;
template <class T> low_level_fuse::t_setxattr low_level_fuse::FuseWrapper<T>::setxattr = nullptr;
template <class T> low_level_fuse::t_getxattr low_level_fuse::FuseWrapper<T>::getxattr = nullptr;
template <class T> low_level_fuse::t_listxattr low_level_fuse::FuseWrapper<T>::listxattr = nullptr;
template <class T>
low_level_fuse::t_removexattr low_level_fuse::FuseWrapper<T>::removexattr = nullptr;
template <class T> low_level_fuse::t_access low_level_fuse::FuseWrapper<T>::access = nullptr;
template <class T> low_level_fuse::t_create low_level_fuse::FuseWrapper<T>::create = nullptr;
template <class T> low_level_fuse::t_getlk low_level_fuse::FuseWrapper<T>::getlk = nullptr;
template <class T> low_level_fuse::t_setlk low_level_fuse::FuseWrapper<T>::setlk = nullptr;
template <class T> low_level_fuse::t_bmap low_level_fuse::FuseWrapper<T>::bmap = nullptr;
template <class T> low_level_fuse::t_ioctl low_level_fuse::FuseWrapper<T>::ioctl = nullptr;
template <class T> low_level_fuse::t_poll low_level_fuse::FuseWrapper<T>::poll = nullptr;
template <class T> low_level_fuse::t_write_buf low_level_fuse::FuseWrapper<T>::write_buf = nullptr;
template <class T>
low_level_fuse::t_retrieve_reply low_level_fuse::FuseWrapper<T>::retrieve_reply = nullptr;
template <class T>
low_level_fuse::t_forget_multi low_level_fuse::FuseWrapper<T>::forget_multi = nullptr;
template <class T> low_level_fuse::t_flock low_level_fuse::FuseWrapper<T>::flock = nullptr;
template <class T> low_level_fuse::t_fallocate low_level_fuse::FuseWrapper<T>::fallocate = nullptr;
template <class T>
low_level_fuse::t_readdirplus low_level_fuse::FuseWrapper<T>::readdirplus = nullptr;
template <class T>
low_level_fuse::t_copy_file_range low_level_fuse::FuseWrapper<T>::copy_file_range = nullptr;
template <class T> low_level_fuse::t_lseek low_level_fuse::FuseWrapper<T>::_lseek = nullptr;

template <class T> struct fuse_lowlevel_ops low_level_fuse::FuseWrapper<T>::_operations;

FusePpFS::FusePpFS(PpFSLowLevel& ppfs)
    : _ppfs(ppfs)
{
}

void FusePpFS::getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info* fi)
{
    const auto ptr = this_(req);

    struct stat stbuf;
    (void)fi;
    std::memset(&stbuf, 0, sizeof(stbuf));
    auto stats_res = ptr->_get_stats(ino, &stbuf);
    HANDLE_EXPECTED_ERROR(req, stats_res);

    fuse_reply_attr(req, &stbuf, 1.0);
}

void FusePpFS::lookup(fuse_req_t req, fuse_ino_t parent, const char* name)
{
    const auto ptr = this_(req);

    if (name == nullptr || name[0] == '\0') {
        fuse_reply_err(req, EINVAL);
        return;
    }

    inode_index_t ppfs_inode = parent - 1;
    auto lookup_res = ptr->_ppfs.lookup(ppfs_inode, name);
    HANDLE_EXPECTED_ERROR(req, lookup_res);

    fuse_ino_t found_ino = lookup_res.value();

    struct fuse_entry_param e;
    std::memset(&e, 0, sizeof(e));

    e.ino = found_ino + 1;

    e.attr_timeout = 5.0;
    e.entry_timeout = 5.0;

    auto stats_res = ptr->_get_stats(e.ino, &e.attr);
    HANDLE_EXPECTED_ERROR(req, stats_res);

    fuse_reply_entry(req, &e);
}

void FusePpFS::readdir(
    fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info* fi)
{
    const auto ptr = this_(req);

    char* buf = (char*)malloc(size);
    if (!buf) {
        fuse_reply_err(req, ENOMEM);
        return;
    }

    inode_index_t ppfs_inode = ino - 1;
    auto entries_res = ptr->_ppfs.getDirectoryEntries(ppfs_inode);
    HANDLE_EXPECTED_ERROR(req, entries_res);

    std::vector<DirectoryEntry> entries = std::move(entries_res.value());

    size_t total_size = 0;

    size_t current_vector_index = (size_t)off;

    for (size_t i = current_vector_index; i < entries.size(); ++i) {
        if (total_size >= size)
            break;

        DirectoryEntry& entry = entries[i];
        struct stat entry_st;

        if (!(ptr->_get_stats(entry.inode + 1, &entry_st)).has_value()) {
            continue;
        }

        off_t next_offset = (off_t)i + 1;

        size_t entry_size = fuse_add_direntry(
            req, buf + total_size, size - total_size, entry.name.data(), &entry_st, next_offset);

        if (entry_size > size - total_size) {
            break;
        }

        total_size += entry_size;
    }

end:
    fuse_reply_buf(req, buf, total_size);
    free(buf);
}

void FusePpFS::mkdir(fuse_req_t req, fuse_ino_t parent, const char* name, mode_t mode)
{
    const auto ptr = this_(req);

    if (name == nullptr || name[0] == '\0') {
        fuse_reply_err(req, EINVAL);
        return;
    }

    inode_index_t ppfs_parent = parent - 1;
    auto create_res = ptr->_ppfs.createDirectoryByParent(ppfs_parent, name);

    if (!create_res.has_value()) {
        std::cerr << "Error: " << toString(create_res.error()) << std::endl;
        int posix_err = _map_fs_error_to_errno(create_res.error());

        fuse_reply_err(req, posix_err);
        return;
    }

    fuse_ino_t new_ino = create_res.value() + 1;

    struct fuse_entry_param e;
    std::memset(&e, 0, sizeof(e));

    e.ino = new_ino;
    e.attr_timeout = 1.0;
    e.entry_timeout = 1.0;

    auto stats_res = ptr->_get_stats(e.ino, &e.attr);
    HANDLE_EXPECTED_ERROR(req, stats_res);

    e.attr.st_mode = (e.attr.st_mode & ~S_IFMT) | S_IFDIR;
    e.attr.st_nlink = 2;

    fuse_reply_entry(req, &e);
}

void FusePpFS::open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info* fi)
{
    const auto ptr = this_(req);

    OpenMode mode = OpenMode::Normal;

    if (fi->flags & O_APPEND) {
        mode = static_cast<OpenMode>(
            static_cast<std::uint8_t>(mode) | static_cast<std::uint8_t>(OpenMode::Append));
    }

    if (fi->flags & O_TRUNC) {
        mode = static_cast<OpenMode>(
            static_cast<std::uint8_t>(mode) | static_cast<std::uint8_t>(OpenMode::Truncate));
    }

    auto open_res = ptr->_ppfs.openByInode(ino - 1, mode);
    HANDLE_EXPECTED_ERROR(req, open_res);

    fi->fh = open_res.value();
    fuse_reply_open(req, fi);
}

void FusePpFS::write(fuse_req_t req, fuse_ino_t ino, const char* buf, size_t size, off_t off,
    struct fuse_file_info* fi)
{
    const auto ptr = this_(req);

    if (!(fi->flags & O_APPEND)) {
        auto seek_res = ptr->_ppfs.seek(fi->fh, off);
        HANDLE_EXPECTED_ERROR(req, seek_res);
    }
    std::vector<uint8_t> to_write(size);
    std::memcpy(to_write.data(), buf, size);
    auto write_res = ptr->_ppfs.write(fi->fh, to_write);
    HANDLE_EXPECTED_ERROR(req, write_res);

    fuse_reply_write(req, write_res.value());
}

void FusePpFS::read(
    fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info* fi)
{
    std::cerr << "Requested to read file with size: " << size << " adn offset: " << off
              << std::endl;

    const auto ptr = this_(req);

    auto seek_res = ptr->_ppfs.seek(fi->fh, off);
    HANDLE_EXPECTED_ERROR(req, seek_res);

    auto read_res = ptr->_ppfs.read(fi->fh, size);
    HANDLE_EXPECTED_ERROR(req, read_res);

    auto buffer = read_res.value();

    fuse_reply_buf(req, reinterpret_cast<const char*>(buffer.data()), buffer.size());
}

void FusePpFS::release(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info* fi)
{
    const auto ptr = this_(req);

    auto close_res = ptr->_ppfs.close(fi->fh);
    HANDLE_EXPECTED_ERROR(req, close_res);

    fuse_reply_err(req, 0);
}

void FusePpFS::mknod(fuse_req_t req, fuse_ino_t parent, const char* name, mode_t mode, dev_t rdev)
{
    const auto ptr = this_(req);

    if (name == nullptr || name[0] == '\0') {
        fuse_reply_err(req, EINVAL);
        return;
    }
    inode_index_t ppfs_parent = parent - 1;

    // We only have regular files in ppfs
    if ((mode & S_IFMT) != S_IFREG) {
        fuse_reply_err(req, ENOTSUP);
        return;
    }
    auto create_res = ptr->_ppfs.createWithParentInode(name, ppfs_parent);

    if (!create_res.has_value()) {
        int posix_err = _map_fs_error_to_errno(create_res.error());
        fuse_reply_err(req, posix_err);
        return;
    }

    fuse_ino_t new_ino = create_res.value() + 1;

    struct fuse_entry_param e;
    std::memset(&e, 0, sizeof(e));

    e.ino = new_ino;
    e.attr_timeout = 1.0;
    e.entry_timeout = 1.0;

    auto stats_res = ptr->_get_stats(e.ino, &e.attr);
    HANDLE_EXPECTED_ERROR(req, stats_res);

    fuse_reply_entry(req, &e);
}

void FusePpFS::unlink(fuse_req_t req, fuse_ino_t parent, const char* name)
{
    const auto ptr = this_(req);

    if (name == nullptr || name[0] == '\0') {
        fuse_reply_err(req, EINVAL);
        return;
    }
    inode_index_t ppfs_parent = parent - 1;

    auto remove_res = ptr->_ppfs.removeByNameAndParent(ppfs_parent, name);
    HANDLE_EXPECTED_ERROR(req, remove_res);

    fuse_reply_err(req, 0);
}

void FusePpFS::rmdir(fuse_req_t req, fuse_ino_t parent, const char* name)
{
    const auto ptr = this_(req);

    if (name == nullptr || name[0] == '\0') {
        fuse_reply_err(req, EINVAL);
        return;
    }
    inode_index_t ppfs_parent = parent - 1;

    auto remove_res = ptr->_ppfs.removeByNameAndParent(ppfs_parent, name);

    HANDLE_EXPECTED_ERROR(req, remove_res);

    fuse_reply_err(req, 0);
}

void FusePpFS::truncate(fuse_req_t req, fuse_ino_t ino, off_t new_size, struct fuse_file_info* fi)
{
    const auto ptr = this_(req);

    inode_index_t ppfs_ino = ino - 1;
    /*
        auto truncate_res = ptr->_ppfs.truncateFile(internal_ino, new_size);

        if (!truncate_res.has_value()) {
            int posix_err = _map_fs_error_to_errno(truncate_res.error());
            fuse_reply_err(req, posix_err);
            return;
        }
    */

    fuse_reply_err(req, 0);
}

std::expected<void, FsError> FusePpFS::_get_stats(fuse_ino_t ino, struct stat* stbuf)
{
    inode_index_t ppfs_inode = ino - 1;
    auto attr_res = _ppfs.getAttributes(ppfs_inode);

    if (!attr_res.has_value())
        return std::unexpected(attr_res.error());

    const auto& attributes = attr_res.value();
    stbuf->st_size = attributes.size;

    stbuf->st_ino = ino;

    mode_t mode;

    if (attributes.type == InodeType::File) {
        mode = S_IFREG;
        mode |= 0644;
        stbuf->st_nlink = 1;
    } else if (attributes.type == InodeType::Directory) {
        mode = S_IFDIR;
        mode |= 0755;
        stbuf->st_nlink = 2;
    }
    stbuf->st_mode = mode;
    stbuf->st_blksize = attributes.block_size;
    stbuf->st_blocks = (stbuf->st_size + attributes.block_size - 1) / attributes.block_size;
    return {};
}

int FusePpFS::_map_fs_error_to_errno(FsError err)
{
    switch (err) {
    case FsError::Bitmap_NotFound:
    case FsError::DirectoryManager_NotFound:
    case FsError::PpFS_NotFound:
    case FsError::InodeManager_NotFound:
        return ENOENT; // No such file or directory

    case FsError::BlockManager_AlreadyTaken:
    case FsError::InodeManager_AlreadyTaken:
    case FsError::DirectoryManager_NameTaken:
    case FsError::PpFS_AlreadyOpen:
        return EEXIST; // File exists

    case FsError::PpFS_FileInUse:
        return EBUSY; // Device or resource busy
    case FsError::PpFS_DirectoryNotEmpty:
        return ENOTEMPTY; // Directory not empty

    case FsError::DirectoryManager_InvalidRequest:
    case FsError::FileIO_InvalidRequest:
    case FsError::PpFS_InvalidRequest:
    case FsError::SuperBlockManager_InvalidRequest:
    case FsError::PpFS_InvalidPath:
        return EINVAL; // Invalid argument

    case FsError::Bitmap_IndexOutOfRange:
    case FsError::Disk_OutOfBounds:
    case FsError::FileIO_OutOfBounds:
    case FsError::PpFS_OutOfBounds:
        return EFBIG; // File too large
    case FsError::PpFS_OpenFilesTableFull:
        return ENFILE; // File table overflow

    case FsError::NotImplemented:
        return ENOSYS; // Function not implemented

    default:
        return EIO; // Input/output error
    }
}
