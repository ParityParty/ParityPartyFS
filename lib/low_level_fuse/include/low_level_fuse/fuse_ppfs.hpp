#include "filesystem/ppfs.hpp"
#include "low_level_fuse/fuse_wrapper.h"

class FusePpFS : public Fuse<FusePPFS> {
private:
    PpFS* ppfs = nullptr;

    int _get_stats(fuse_ino_t ino, struct stat* stbuf);
    int _map_fs_error_to_errno(FsError err);

public:
    FusePpFS(IDisk& disk);
    ~FusePpFS();

    static void getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info* fi);
    static void lookup(fuse_req_t req, fuse_ino_t parent, const char* name);
};