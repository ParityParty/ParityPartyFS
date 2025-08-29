#include "ppfs/ppfs.hpp"
#include "ppfs/data_structures.hpp"

template <class T> Fusepp::t_getattr Fusepp::Fuse<T>::getattr = nullptr;
template <class T> Fusepp::t_readlink Fusepp::Fuse<T>::readlink = nullptr;
template <class T> Fusepp::t_mknod Fusepp::Fuse<T>::mknod = nullptr;
template <class T> Fusepp::t_mkdir Fusepp::Fuse<T>::mkdir = nullptr;
template <class T> Fusepp::t_unlink Fusepp::Fuse<T>::unlink = nullptr;
template <class T> Fusepp::t_rmdir Fusepp::Fuse<T>::rmdir = nullptr;
template <class T> Fusepp::t_symlink Fusepp::Fuse<T>::symlink = nullptr;
template <class T> Fusepp::t_rename Fusepp::Fuse<T>::rename = nullptr;
template <class T> Fusepp::t_link Fusepp::Fuse<T>::link = nullptr;
template <class T> Fusepp::t_chmod Fusepp::Fuse<T>::chmod = nullptr;
template <class T> Fusepp::t_chown Fusepp::Fuse<T>::chown = nullptr;
template <class T> Fusepp::t_truncate Fusepp::Fuse<T>::truncate = nullptr;
template <class T> Fusepp::t_open Fusepp::Fuse<T>::open = nullptr;
template <class T> Fusepp::t_read Fusepp::Fuse<T>::read = nullptr;
template <class T> Fusepp::t_write Fusepp::Fuse<T>::write = nullptr;
template <class T> Fusepp::t_statfs Fusepp::Fuse<T>::statfs = nullptr;
template <class T> Fusepp::t_flush Fusepp::Fuse<T>::flush = nullptr;
template <class T> Fusepp::t_release Fusepp::Fuse<T>::release = nullptr;
template <class T> Fusepp::t_fsync Fusepp::Fuse<T>::fsync = nullptr;
template <class T> Fusepp::t_setxattr Fusepp::Fuse<T>::setxattr = nullptr;
template <class T> Fusepp::t_getxattr Fusepp::Fuse<T>::getxattr = nullptr;
template <class T> Fusepp::t_listxattr Fusepp::Fuse<T>::listxattr = nullptr;
template <class T> Fusepp::t_removexattr Fusepp::Fuse<T>::removexattr = nullptr;
template <class T> Fusepp::t_opendir Fusepp::Fuse<T>::opendir = nullptr;
template <class T> Fusepp::t_readdir Fusepp::Fuse<T>::readdir = nullptr;
template <class T> Fusepp::t_releasedir Fusepp::Fuse<T>::releasedir = nullptr;
template <class T> Fusepp::t_fsyncdir Fusepp::Fuse<T>::fsyncdir = nullptr;
template <class T> Fusepp::t_init Fusepp::Fuse<T>::init = nullptr;
template <class T> Fusepp::t_destroy Fusepp::Fuse<T>::destroy = nullptr;
template <class T> Fusepp::t_access Fusepp::Fuse<T>::access = nullptr;
template <class T> Fusepp::t_create Fusepp::Fuse<T>::create = nullptr;
template <class T> Fusepp::t_lock Fusepp::Fuse<T>::lock = nullptr;
template <class T> Fusepp::t_utimens Fusepp::Fuse<T>::utimens = nullptr;
template <class T> Fusepp::t_bmap Fusepp::Fuse<T>::bmap = nullptr;
template <class T> Fusepp::t_ioctl Fusepp::Fuse<T>::ioctl = nullptr;
template <class T> Fusepp::t_poll Fusepp::Fuse<T>::poll = nullptr;
template <class T> Fusepp::t_write_buf Fusepp::Fuse<T>::write_buf = nullptr;
template <class T> Fusepp::t_read_buf Fusepp::Fuse<T>::read_buf = nullptr;
template <class T> Fusepp::t_flock Fusepp::Fuse<T>::flock = nullptr;
template <class T> Fusepp::t_fallocate Fusepp::Fuse<T>::fallocate = nullptr;

template <class T> struct fuse_operations Fusepp::Fuse<T>::operations_;

PpFS::PpFS(IDisk& disk):
    _disk(disk)
{
    // Write something to file to help with testing
    _disk.write(0, {reinterpret_cast<const std::byte*>(_hello_str.c_str()),reinterpret_cast<const std::byte*>(_hello_str.c_str()) + _hello_str.size()});
    _data_length = _hello_str.size();
}

int PpFS::getattr(const char* path, struct stat* stBuffer, struct fuse_file_info*)
{
    const auto ptr = this_();

    int res = 0;

    memset(stBuffer, 0, sizeof(struct stat));
    if (path == ptr->rootPath()) {
        stBuffer->st_mode = S_IFDIR | 0755;
        stBuffer->st_nlink = 2;
    } else if (path == ptr->helloPath()) {
        stBuffer->st_mode = S_IFREG | 0666;
        stBuffer->st_nlink = 1;
        stBuffer->st_size = static_cast<off_t>(ptr->helloStr().length());
    } else
        res = -ENOENT;

    return res;
}

int PpFS::readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t,
    struct fuse_file_info*, enum fuse_readdir_flags)
{
    const auto ptr = this_();

    if (path != ptr->rootPath())
        return -ENOENT;

    filler(buf, ".", nullptr, 0, FUSE_FILL_DIR_PLUS);
    filler(buf, "..", nullptr, 0, FUSE_FILL_DIR_PLUS);
    filler(buf, ptr->helloPath().c_str() + 1, nullptr, 0, FUSE_FILL_DIR_PLUS);

    return 0;
}

int PpFS::open(const char* path, struct fuse_file_info* fi)
{
    if (path != this_()->helloPath())
        return -ENOENT;

    if ((fi->flags & 3) != O_RDONLY)
        return -EACCES;

    return 0;
}

int PpFS::read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info*)
{
    const auto ptr = this_();

    if (path != ptr->helloPath())
        return -ENOENT;

    const auto len = ptr->_data_length;
    if (static_cast<size_t>(offset) >= len)
        size = 0;
    else {
        if (offset + size > len)
            size = len - offset;
        auto res = ptr->_disk.read(offset, size);
        if (!res.has_value()) {
            return -EIO;
        }
        memcpy(buf, res.value().data(), size);
    }

    return static_cast<int>(size);
}

int PpFS::write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info*)
{
    const auto ptr = this_();
    if (path != ptr->helloPath())
        return -ENOENT;
    if (offset + size > ptr->_disk.size()) {
        return -ENOSPC;
    }
    auto res = ptr->_disk.write(offset, {reinterpret_cast<const std::byte*>(buf), reinterpret_cast<const std::byte*>(buf + size)});
    if (!res.has_value()) {
        return -EIO;
    }
    ptr->_data_length = size;
    return static_cast<int>(size);
}

std::expected<void, DiskError> PpFS::formatDisk(const unsigned int block_size)
{
    this->block_size = block_size;

    if (block_size < sizeof(SuperBlock)) {
        return std::unexpected(DiskError::InvalidRequest);
    }
    unsigned int num_blocks = this->_disk.size()/block_size;
    unsigned int fat_block_start = 1;
    unsigned int fat_num_blocks = num_blocks * sizeof(unsigned int);

    auto sb = SuperBlock(num_blocks, block_size, fat_block_start, fat_num_blocks);

    auto ret = _disk.write(0, sb.toBytes());
    if (!ret.has_value()) {
        return std::unexpected(DiskError::IOError);
    }

    auto fat_table = std::vector(num_blocks, 0);
    auto fat = FileAllocationTable(std::move(fat_table));

    auto ret1 = _disk.write(0, sb.toBytes());
    auto ret2 = _disk.write(block_size, fat.toBytes());
    if (!ret1.has_value() || !ret2.has_value()) {
        return std::unexpected(DiskError::IOError);
    }

    // TODO: add root directory with header with number of files, name etc.

    return {};
}
