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
    struct stat stbuf;
    (void)fi;
    memset(&stbuf, 0, sizeof(stbuf));
    if (_get_stats(ino, &stbuf) == -1)
        fuse_reply_err(req, errno);
    else
        fuse_reply_attr(req, &stbuf, 1.0);
}

static void FusePpFS::lookup(fuse_req_t req, fuse_ino_t parent, const char* name) { }

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
