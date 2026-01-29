/**
 * @file main.cpp
 * @brief Example FUSE filesystem using low-level FUSE wrapper.
 *
 * I have added this file to demonstrate how to use the low-level FUSE wrapper
 * and to see if it works (we can remove it later).
 *
 * The functionality is copied from the hello_ll.c example from the FUSE
 * repository, release 3.10.5. I have decided to use this release, because
 * this release is available on most Ubuntu / Debian systems (like mine xd).
 */

#include "ppfs/low_level_fuse/fuse_wrapper.hpp"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace low_level_fuse;

#define min(x, y) ((x) < (y) ? (x) : (y))

struct dirbuf {
    char* p;
    size_t size;
};

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

template <class T> struct fuse_lowlevel_ops low_level_fuse::Fuse<T>::_operations;

class HelloFS : public FuseWrapper<HelloFS> {
private:
    static constexpr const char* _file_name = "hello.txt";
    static constexpr const char* _file_content = "Hello, World!\n";

    static int _get_stats(fuse_ino_t ino, struct stat* stbuf)
    {
        stbuf->st_ino = ino;
        switch (ino) {
        case 1:
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_nlink = 2;
            break;

        case 2:
            stbuf->st_mode = S_IFREG | 0444;
            stbuf->st_nlink = 1;
            stbuf->st_size = strlen(_file_content);
            break;

        default:
            return -1;
        }
        return 0;
    }

    static void _dirbuf_add(fuse_req_t req, struct dirbuf* b, const char* name, fuse_ino_t ino)
    {
        struct stat stbuf;
        size_t oldsize = b->size;

        b->size += fuse_add_direntry(req, NULL, 0, name, NULL, 0);
        b->p = (char*)realloc(b->p, b->size);
        memset(&stbuf, 0, sizeof(stbuf));
        stbuf.st_ino = ino;

        fuse_add_direntry(req, b->p + oldsize, b->size - oldsize, name, &stbuf, b->size);
    }

public:
    static void lookup(fuse_req_t req, fuse_ino_t parent, const char* name)
    {
        if (parent != FUSE_ROOT_ID || std::strcmp(name, _file_name) != 0) {
            fuse_reply_err(req, ENOENT);
            return;
        }

        struct fuse_entry_param e;
        memset(&e, 0, sizeof(e));
        e.ino = 2;
        e.attr_timeout = 1.0;
        e.entry_timeout = 1.0;
        _get_stats(e.ino, &e.attr);
        fuse_reply_entry(req, &e);
    }

    static void getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info* fi)
    {
        struct stat stbuf;
        (void)fi;
        memset(&stbuf, 0, sizeof(stbuf));
        if (_get_stats(ino, &stbuf) == -1)
            fuse_reply_err(req, ENOENT);
        else
            fuse_reply_attr(req, &stbuf, 1.0);
    }

    static void readdir(
        fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info* fi)
    {
        (void)fi;

        if (ino != 1)
            fuse_reply_err(req, ENOTDIR);
        else {
            struct dirbuf b;

            memset(&b, 0, sizeof(b));
            _dirbuf_add(req, &b, ".", 1);
            _dirbuf_add(req, &b, "..", 1);
            _dirbuf_add(req, &b, _file_name, 2);
            if (off < b.size)
                fuse_reply_buf(req, b.p + off, min(b.size - off, size));
            else
                fuse_reply_buf(req, NULL, 0);
            free(b.p);
        }
    }

    static void open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info* fi)
    {
        if (ino != 2)
            fuse_reply_err(req, EISDIR);
        else if ((fi->flags & O_ACCMODE) != O_RDONLY)
            fuse_reply_err(req, EACCES);
        else
            fuse_reply_open(req, fi);
    }

    static void read(
        fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info* fi)
    {
        (void)fi;

        if (ino != 2)
            fuse_reply_err(req, EISDIR);
        int file_size = strlen(_file_content);
        if (off < file_size)
            fuse_reply_buf(req, _file_content + off, min(file_size - off, file_size));
        else
            fuse_reply_buf(req, NULL, 0);
    }
};

int main(int argc, char** argv)
{
    HelloFS fs;
    return fs.Run(argc, argv);
}
