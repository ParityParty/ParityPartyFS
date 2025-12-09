#pragma once

#include <expected>

#include "common/types.hpp"

template <typename T, typename Func>
std::expected<T, FsError> mutex_wrapper(PpFSMutex& mutex, Func f)
{
    auto lock = mutex.lock();
    if (!lock.has_value()) {
        if (lock.error() == FsError::Mutex_NotInitialized) {
            return std::unexpected(FsError::PpFS_NotInitialized);
        }
        return std::unexpected(lock.error());
    }
    auto ret = f();
    auto unlock = mutex.unlock();
    if (!unlock.has_value()) {
        return std::unexpected(unlock.error());
    }
    return ret;
};