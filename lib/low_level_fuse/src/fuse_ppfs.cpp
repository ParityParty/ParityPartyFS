#include "low_level_fuse/fuse_ppfs.hpp"

template <class T> low_level_fuse::t_init low_level_fuse::Fuse<T>::init = nullptr;
template <class T> low_level_fuse::t_destroy low_level_fuse::Fuse<T>::destroy = nullptr;
template <class T> low_level_fuse::t_lookup low_level_fuse::Fuse<T>::lookup = nullptr;
template <class T> low_level_fuse::t_forget low_level_fuse::Fuse<T>::forget = nullptr;
template <class T> low_level_fuse::t_getattr low_level_fuse::Fuse<T>::getattr = nullptr;
template <class T> low_level_fuse::t_setattr low_level_fuse::Fuse<T>::setattr = nullptr;
template <class T> low_level_fuse::t_readlink low_level_fuse::Fuse<T>::readlink = nullptr;
template <class T> low_level_fuse::t_mknod low_level_fuse::Fuse<T>::mknod = nullptr;
template <class T> low_level_fuse::t_mkdir low_level_fuse::Fuse<T>::mkdir = nullptr;
template <class T> low_level_fuse::t_unlink low_level_fuse::Fuse<T>::unlink = nullptr;
template <class T> low_level_fuse::t_rmdir low_level_fuse::Fuse<T>::rmdir = nullptr;
template <class T> low_level_fuse::t_symlink low_level_fuse::Fuse<T>::symlink = nullptr;
template <class T> low_level_fuse::t_rename low_level_fuse::Fuse<T>::rename = nullptr;
template <class T> low_level_fuse::t_link low_level_fuse::Fuse<T>::link = nullptr;
template <class T> low_level_fuse::t_open low_level_fuse::Fuse<T>::open = nullptr;
template <class T> low_level_fuse::t_read low_level_fuse::Fuse<T>::read = nullptr;
template <class T> low_level_fuse::t_write low_level_fuse::Fuse<T>::write = nullptr;
template <class T> low_level_fuse::t_flush low_level_fuse::Fuse<T>::flush = nullptr;
template <class T> low_level_fuse::t_release low_level_fuse::Fuse<T>::release = nullptr;
template <class T> low_level_fuse::t_fsync low_level_fuse::Fuse<T>::fsync = nullptr;
template <class T> low_level_fuse::t_opendir low_level_fuse::Fuse<T>::opendir = nullptr;
template <class T> low_level_fuse::t_readdir low_level_fuse::Fuse<T>::readdir = nullptr;
template <class T> low_level_fuse::t_releasedir low_level_fuse::Fuse<T>::releasedir = nullptr;
template <class T> low_level_fuse::t_fsyncdir low_level_fuse::Fuse<T>::fsyncdir = nullptr;
template <class T> low_level_fuse::t_statfs low_level_fuse::Fuse<T>::statfs = nullptr;
template <class T> low_level_fuse::t_setxattr low_level_fuse::Fuse<T>::setxattr = nullptr;
template <class T> low_level_fuse::t_getxattr low_level_fuse::Fuse<T>::getxattr = nullptr;
template <class T> low_level_fuse::t_listxattr low_level_fuse::Fuse<T>::listxattr = nullptr;
template <class T> low_level_fuse::t_removexattr low_level_fuse::Fuse<T>::removexattr = nullptr;
template <class T> low_level_fuse::t_access low_level_fuse::Fuse<T>::access = nullptr;
template <class T> low_level_fuse::t_create low_level_fuse::Fuse<T>::create = nullptr;
template <class T> low_level_fuse::t_getlk low_level_fuse::Fuse<T>::getlk = nullptr;
template <class T> low_level_fuse::t_setlk low_level_fuse::Fuse<T>::setlk = nullptr;
template <class T> low_level_fuse::t_bmap low_level_fuse::Fuse<T>::bmap = nullptr;
template <class T> low_level_fuse::t_ioctl low_level_fuse::Fuse<T>::ioctl = nullptr;
template <class T> low_level_fuse::t_poll low_level_fuse::Fuse<T>::poll = nullptr;
template <class T> low_level_fuse::t_write_buf low_level_fuse::Fuse<T>::write_buf = nullptr;
template <class T>
low_level_fuse::t_retrieve_reply low_level_fuse::Fuse<T>::retrieve_reply = nullptr;
template <class T> low_level_fuse::t_forget_multi low_level_fuse::Fuse<T>::forget_multi = nullptr;
template <class T> low_level_fuse::t_flock low_level_fuse::Fuse<T>::flock = nullptr;
template <class T> low_level_fuse::t_fallocate low_level_fuse::Fuse<T>::fallocate = nullptr;
template <class T> low_level_fuse::t_readdirplus low_level_fuse::Fuse<T>::readdirplus = nullptr;
template <class T>
low_level_fuse::t_copy_file_range low_level_fuse::Fuse<T>::copy_file_range = nullptr;
template <class T> low_level_fuse::t_lseek low_level_fuse::Fuse<T>::_lseek = nullptr;

FusePpFS::FusePpFS(IDisk& disk)
{
    ppfs = new PpFSLinux(disk);
    ppfs->init();
}

FusePpFS::~FusePpFS() { delete ppfs; }

static void FusePpFS::getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info* fi)
{
    const auto ptr = this_();

    struct stat stbuf;
    (void)fi;
    memset(&stbuf, 0, sizeof(stbuf));
    if (ptr->_get_stats(ino, &stbuf) == -1)
        fuse_reply_err(req, errno);
    else
        fuse_reply_attr(req, &stbuf, 1.0);
}

static void FusePpFS::lookup(fuse_req_t req, fuse_ino_t parent, const char* name)
{
    const auto ptr = this_();

    if (name == nullptr || name[0] == '\0') {
        fuse_reply_err(req, EINVAL);
        return;
    }

    auto lookup_res = g_fs_instance->lookup(parent, name);

    if (!lookup_res.has_value()) {
        int posix_err = ptr->_map_fs_error_to_errno(lookup_res.error());
        fuse_reply_err(req, posix_err);
        return;
    }

    fuse_ino_t found_ino = lookup_res.value();

    struct fuse_entry_param e;
    std::memset(&e, 0, sizeof(e));

    e.ino = found_ino;

    e.attr_timeout = 5.0;
    e.entry_timeout = 5.0;

    if (ptr->_get_stats(e.ino, &e.attr) == -1) {
        fuse_reply_err(req, EIO);
        return;
    }

    fuse_reply_entry(req, &e);
}

static void FusePpFS::readdir(
    fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info* fi)
{
    const auto ptr = this_();

    char* buf = (char*)malloc(size);
    if (!buf) {
        fuse_reply_err(req, ENOMEM);
        return;
    }

    auto entries_res = g_fs_instance.getEntries(ino);

    if (!entries_res.has_value()) {
        int posix_err = ptr->ppfs.map_fs_error_to_errno(entries_res.error());
        fuse_reply_err(req, posix_err);
        free(buf);
        return;
    }

    std::vector<DirectoryEntry> entries = std::move(entries_res.value());

    size_t total_size = 0;

    size_t current_vector_index = (size_t)offset;

    for (size_t i = current_vector_index; i < entries.size(); ++i) {
        if (total_size >= size)
            break;

        DirectoryEntry& entry = entries[i];
        struct stat entry_st;

        if (ptr->ppfs._get_stats(entry.inode, &entry_st) == -1) {
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

static void FusePpFS::mkdir(fuse_req_t req, fuse_ino_t parent, const char* name, mode_t mode)
{
    const auto ptr = this_();

    if (name == nullptr || name[0] == '\0') {
        fuse_reply_err(req, EINVAL);
        return;
    }

    auto create_res = ptr->ppfs.createDirectoryByParent(parent, name, mode);

    if (!create_res.has_value()) {
        int posix_err = _map_fs_error_to_errno(create_res.error());

        fuse_reply_err(req, posix_err);
        return;
    }

    fuse_ino_t new_ino = create_res.value();

    struct fuse_entry_param e;
    std::memset(&e, 0, sizeof(e));

    e.ino = new_ino;
    e.attr_timeout = 1.0;
    e.entry_timeout = 1.0;

    if (g_fs_instance._get_stats(e.ino, &e.attr) == -1) {
        fuse_reply_err(req, EIO);
        return;
    }

    e.attr.st_mode = (e.attr.st_mode & ~S_IFMT) | S_IFDIR;
    e.attr.st_nlink = 2;
    fuse_reply_entry(req, &e);
}

int FusePpFS::_get_stats(fuse_ino_t ino, struct stat* stbuf)
{
    auto attr_res = PpFSLinux::getAttributes(ino);

    if (!attr_res.has_value()) {
        FsError err = attr_res.error();

        switch (err) {
        case FsError::InodeManager_NotFound:
            errno = ENOENT;
            break;
        default:
            errno = EIO;
        }
        return -1;
    }

    const auto& attributes = attr_res.value();

    stbuf->st_size = attributes.size;

    stbuf->st_ino = ino;

    mode_t mode;

    if (attributes.type == FileType::File) {
        mode = S_IFREG;
        mode |= 0644;
        stbuf->st_nlink = 1;
    } else if (attributes.type == FileType::Directory) {
        mode = S_IFDIR;
        mode |= 0755;
        stbuf->st_nlink = 2;
    }
    stbuf->st_mode = mode;

    stbuf->st_blksize = attributes.block_size;
    stbuf->st_blocks = (stbuf->st_size + attributes.block_size - 1) / attributes.block_size;

    return 0;
}

static int FusePpFS _map_fs_error_to_errno(FsError err)
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

    case FsError::PPFS_FAT_InvalidRequest:
    case FsError::DirectoryManager_InvalidRequest:
    case FsError::FileIO_InvalidRequest:
    case FsError::PpFS_InvalidRequest:
    case FsError::SuperBlockManager_InvalidRequest:
    case FsError::PpFS_InvalidPath:
        return EINVAL; // Invalid argument

    case FsError::PPFS_FAT_OutOfBounds:
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
