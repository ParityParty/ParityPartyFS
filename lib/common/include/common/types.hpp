#pragma once
#include <cstdint>

#include <string_view>

typedef int num_entries_t;
typedef int block_index_t;
typedef int inode_index_t;
typedef unsigned int file_descriptor_t;

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
    NameTaken,
    NotImplemented,
    DiskNotFormatted,
    NotInitialized,
    InvalidPath,

    MutexInitFailed,
    MutexLockFailed,
    MutexUnlockFailed,
    MutexNotInitialized,
    MutexAlreadyInitialized,

    OpenFilesTableFull,
    AlreadyOpen,
    FileInUse,
    DirectoryNotEmpty,
};

inline std::string_view toString(FsError err)
{
    switch (err) {
    case FsError::IOError:
        return "IOError";
    case FsError::OutOfBounds:
        return "OutOfBounds";
    case FsError::InvalidRequest:
        return "InvalidRequest";
    case FsError::InternalError:
        return "InternalError";
    case FsError::OutOfMemory:
        return "OutOfMemory";
    case FsError::CorrectionError:
        return "CorrectionError";
    case FsError::IndexOutOfRange:
        return "IndexOutOfRange";
    case FsError::NotFound:
        return "NotFound";
    case FsError::AlreadyTaken:
        return "AlreadyTaken";
    case FsError::AlreadyFree:
        return "AlreadyFree";
    case FsError::NotImplemented:
        return "NotImplemented";
    case FsError::NameTaken:
        return "NameTaken";
    case FsError::DiskNotFormatted:
        return "DiskNotFormatted";
    case FsError::NotInitialized:
        return "NotInitialized";
    case FsError::InvalidPath:
        return "InvalidPath";

    case FsError::MutexInitFailed:
        return "MutexInitFailed";
    case FsError::MutexLockFailed:
        return "MutexLockFailed";
    case FsError::MutexUnlockFailed:
        return "MutexUnlockFailed";
    case FsError::MutexNotInitialized:
        return "MutexNotInitialized";
    case FsError::MutexAlreadyInitialized:
        return "MutexAlreadyInitialized";

    case FsError::OpenFilesTableFull:
        return "OpenFilesTableFull";
    case FsError::AlreadyOpen:
        return "AlreadyOpen";
    case FsError::FileInUse:
        return "FileInUse";
    case FsError::DirectoryNotEmpty:
        return "DirectoryNotEmpty";

    default:
        return "UnknownError";
    }
}
