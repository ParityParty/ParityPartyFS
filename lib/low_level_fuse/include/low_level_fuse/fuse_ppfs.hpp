#pragma once

#include "filesystem/ppfs_low_level.hpp"
#include "low_level_fuse/fuse_wrapper.hpp"

using namespace low_level_fuse;

class FusePpFS : public FuseWrapper<FusePpFS> {
private:
    PpFSLowLevel& _ppfs;

    int _get_stats(fuse_ino_t ino, struct stat* stbuf);
    static int _map_fs_error_to_errno(FsError err);

public:
    FusePpFS(PpFSLowLevel& ppfs);
    ~FusePpFS() = default;

    static void getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info* fi);
    static void read(
        fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info* fi);
    static void lookup(fuse_req_t req, fuse_ino_t parent, const char* name);
    static void readdir(
        fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info* fi);
    static void mkdir(fuse_req_t req, fuse_ino_t parent, const char* name, mode_t mode);
    static void open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info* fi);
    static void write(fuse_req_t req, fuse_ino_t ino, const char* buf, size_t size, off_t off,
        struct fuse_file_info* fi);
    static void release(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info* fi);
    static void mknod(fuse_req_t req, fuse_ino_t parent, const char* name, mode_t mode, dev_t rdev);
    static void unlink(fuse_req_t req, fuse_ino_t parent, const char* name);
    static void rmdir(fuse_req_t req, fuse_ino_t parent, const char* name);
};