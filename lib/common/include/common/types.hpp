#pragma once
#include <cstdint>

#include <string_view>

typedef std::uint32_t num_entries_t;
typedef std::uint32_t block_index_t;
typedef std::uint32_t inode_index_t;
typedef std::int32_t file_descriptor_t;

enum class FsError : uint8_t {
    // Bitmap errors
    Bitmap_IndexOutOfRange,
    Bitmap_NotFound,

    // Block manager errors
    BlockManager_AlreadyTaken,
    BlockManager_AlreadyFree,
    BlockManager_NoMoreFreeBlocks,

    // Block device errors
    BlockDevice_CorrectionError,

    // Directory Manager errors
    DirectoryManager_NameTaken,
    DirectoryManager_NotFound,
    DirectoryManager_InvalidRequest,

    // Disk errors
    Disk_OutOfBounds,

    // FileIO errors
    FileIO_OutOfBounds,
    FileIO_InternalError,
    FileIO_InvalidRequest,

    // Filesystem errors
    PpFS_DiskNotFormatted,
    PpFS_InvalidRequest,
    PpFS_NotInitialized,
    PpFS_InvalidPath,
    PpFS_NotFound,
    PpFS_FileInUse,
    PpFS_DirectoryNotEmpty,
    PpFS_OutOfBounds,
    PpFS_OpenFilesTableFull,
    PpFS_AlreadyOpen,

    // Inode Manager errors
    InodeManager_AlreadyTaken,
    InodeManager_NotFound,
    InodeManager_AlreadyFree,
    InodeManager_NoMoreFreeInodes,

    // Mutex errors
    Mutex_InitFailed,
    Mutex_LockFailed,
    Mutex_UnlockFailed,
    Mutex_NotInitialized,
    Mutex_AlreadyInitialized,
    Mutex_InternalError,

    // SuperBlock Manager errors
    SuperBlockManager_InvalidRequest,

    // Generic error
    NotImplemented,
};

inline std::string_view toString(FsError err)
{
    switch (err) {
    case FsError::Bitmap_IndexOutOfRange:
        return "Bitmap_IndexOutOfRange";
    case FsError::Bitmap_NotFound:
        return "Bitmap_NotFound";

    case FsError::BlockManager_AlreadyTaken:
        return "BlockManager_AlreadyTaken";
    case FsError::BlockManager_AlreadyFree:
        return "BlockManager_AlreadyFree";
    case FsError::BlockManager_NoMoreFreeBlocks:
        return "BlockManager_NoMoreFreeBlocks";

    case FsError::BlockDevice_CorrectionError:
        return "BlockDevice_CorrectionError";

    case FsError::DirectoryManager_NameTaken:
        return "DirectoryManager_NameTaken";
    case FsError::DirectoryManager_NotFound:
        return "DirectoryManager_NotFound";
    case FsError::DirectoryManager_InvalidRequest:
        return "DirectoryManager_InvalidRequest";

    case FsError::Disk_OutOfBounds:
        return "Disk_OutOfBounds";

    case FsError::FileIO_OutOfBounds:
        return "FileIO_OutOfBounds";
    case FsError::FileIO_InternalError:
        return "FileIO_InternalError";
    case FsError::FileIO_InvalidRequest:
        return "FileIO_InvalidRequest";

    case FsError::PpFS_DiskNotFormatted:
        return "PpFS_DiskNotFormatted";
    case FsError::PpFS_InvalidRequest:
        return "PpFS_InvalidRequest";
    case FsError::PpFS_NotInitialized:
        return "PpFS_NotInitialized";
    case FsError::PpFS_InvalidPath:
        return "PpFS_InvalidPath";
    case FsError::PpFS_NotFound:
        return "PpFS_NotFound";
    case FsError::PpFS_FileInUse:
        return "PpFS_FileInUse";
    case FsError::PpFS_DirectoryNotEmpty:
        return "PpFS_DirectoryNotEmpty";
    case FsError::PpFS_OutOfBounds:
        return "PpFS_OutOfBounds";
    case FsError::PpFS_OpenFilesTableFull:
        return "PpFS_OpenFilesTableFull";
    case FsError::PpFS_AlreadyOpen:
        return "PpFS_AlreadyOpen";

    case FsError::InodeManager_AlreadyTaken:
        return "InodeManager_AlreadyTaken";
    case FsError::InodeManager_NotFound:
        return "InodeManager_NotFound";
    case FsError::InodeManager_AlreadyFree:
        return "InodeManager_AlreadyFree";
    case FsError::InodeManager_NoMoreFreeInodes:
        return "InodeManager_NoMoreFreeInodes";

    case FsError::Mutex_InitFailed:
        return "Mutex_InitFailed";
    case FsError::Mutex_LockFailed:
        return "Mutex_LockFailed";
    case FsError::Mutex_UnlockFailed:
        return "Mutex_UnlockFailed";
    case FsError::Mutex_NotInitialized:
        return "Mutex_NotInitialized";
    case FsError::Mutex_AlreadyInitialized:
        return "Mutex_AlreadyInitialized";
    case FsError::Mutex_InternalError:
        return "Mutex_InternalError";

    case FsError::SuperBlockManager_InvalidRequest:
        return "SuperBlockManager_InvalidRequest";

    case FsError::NotImplemented:
        return "NotImplemented";

    default:
        return "UnknownError";
    }
}
