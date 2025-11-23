#pragma once
#include <cstdint>

typedef int num_entries_t;
typedef int block_index_t;
typedef int inode_index_t;

enum class FsError {
    IOError,
    OutOfBounds,
    InvalidRequest,
    InternalError,
    OutOfMemory,
    CorrectionError,
    IndexOutOfRange,
    NotFound,
    AlreadyTaken,
    AlreadyFree,
    NotImplemented,
};
