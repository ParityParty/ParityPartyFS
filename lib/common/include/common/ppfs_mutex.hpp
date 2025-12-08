#pragma once

#include "common/types.hpp"
#include <cstdint>
#include <expected>

#ifdef PPFS_USE_FREERTOS
#    include "FreeRTOS.h"
#    include "semphr.h"
#else
#    include <pthread.h>
#endif

class PpFSMutex {
public:
    PpFSMutex()
        : isInitialized_(false)
    {
#ifdef PPFS_USE_FREERTOS
        mutexHandle_ = nullptr;
#endif
    }

    ~PpFSMutex() { deinit(); }

    [[nodiscard]] std::expected<void, FsError> init()
    {
        if (isInitialized_) {
            return std::unexpected(FsError::MutexAlreadyInitialized);
        }

#ifdef PPFS_USE_FREERTOS
        mutexHandle_ = xSemaphoreCreateMutex();
        if (mutexHandle_ == nullptr) {
            return std::unexpected(FsError::MutexInitFailed);
        }
#else
        if (pthread_mutex_init(&mutexHandle_, nullptr) != 0) {
            return std::unexpected(FsError::MutexInitFailed);
        }
#endif

        isInitialized_ = true;
        return {};
    }

    std::expected<void, FsError> deinit()
    {
        if (!isInitialized_) {
            return std::unexpected(FsError::MutexNotInitialized);
        }

#ifdef PPFS_USE_FREERTOS
        if (mutexHandle_ != nullptr) {
            vSemaphoreDelete(mutexHandle_);
            mutexHandle_ = nullptr;
        }
#else
        if (pthread_mutex_destroy(&mutexHandle_) != 0) {
            return std::unexpected(FsError::InternalError);
        }
#endif

        isInitialized_ = false;
        return {};
    }

    [[nodiscard]] std::expected<void, FsError> lock()
    {
        if (!isInitialized_) {
            return std::unexpected(FsError::MutexNotInitialized);
        }

#ifdef PPFS_USE_FREERTOS
        if (xSemaphoreTake(mutexHandle_, portMAX_DELAY) != pdTRUE) {
            return std::unexpected(FsError::MutexLockFailed);
        }
#else
        if (pthread_mutex_lock(&mutexHandle_) != 0) {
            return std::unexpected(FsError::MutexLockFailed);
        }
#endif
        return {};
    }

    [[nodiscard]] std::expected<void, FsError> unlock()
    {
        if (!isInitialized_) {
            return std::unexpected(FsError::MutexNotInitialized);
        }

#ifdef PPFS_USE_FREERTOS
        if (xSemaphoreGive(mutexHandle_) != pdTRUE) {
            return std::unexpected(FsError::MutexUnlockFailed);
        }
#else
        if (pthread_mutex_unlock(&mutexHandle_) != 0) {
            return std::unexpected(FsError::MutexUnlockFailed);
        }
#endif
        return {};
    }

    // Disable copying
    PpFSMutex(const PpFSMutex&) = delete;
    PpFSMutex& operator=(const PpFSMutex&) = delete;

private:
    bool isInitialized_;

#ifdef PPFS_USE_FREERTOS
    SemaphoreHandle_t mutexHandle_;
#else
    pthread_mutex_t mutexHandle_;
#endif
};