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
        : _is_initialized(false)
    {
#ifdef PPFS_USE_FREERTOS
        _mutex_handle = nullptr;
#endif
    }

    ~PpFSMutex() { (void)deinit(); }

    [[nodiscard]] std::expected<void, FsError> init()
    {
        if (_is_initialized) {
            return std::unexpected(FsError::Mutex_AlreadyInitialized);
        }

#ifdef PPFS_USE_FREERTOS
        _mutex_handle = xSemaphoreCreateMutex();
        if (_mutex_handle == nullptr) {
            return std::unexpected(FsError::Mutex_InitFailed);
        }
#else
        if (pthread_mutex_init(&_mutex_handle, nullptr) != 0) {
            return std::unexpected(FsError::Mutex_InitFailed);
        }
#endif

        _is_initialized = true;
        return { };
    }

    [[nodiscard]] std::expected<void, FsError> deinit()
    {
        if (!_is_initialized) {
            return std::unexpected(FsError::Mutex_NotInitialized);
        }

#ifdef PPFS_USE_FREERTOS
        if (_mutex_handle != nullptr) {
            vSemaphoreDelete(_mutex_handle);
            _mutex_handle = nullptr;
        }
#else
        if (pthread_mutex_destroy(&_mutex_handle) != 0) {
            return std::unexpected(FsError::Mutex_InternalError);
        }
#endif

        _is_initialized = false;
        return { };
    }

    [[nodiscard]] std::expected<void, FsError> lock()
    {
        if (!_is_initialized) {
            return std::unexpected(FsError::Mutex_NotInitialized);
        }

#ifdef PPFS_USE_FREERTOS
        if (xSemaphoreTake(_mutex_handle, portMAX_DELAY) != pdTRUE) {
            return std::unexpected(FsError::Mutex_LockFailed);
        }
#else
        if (pthread_mutex_lock(&_mutex_handle) != 0) {
            return std::unexpected(FsError::Mutex_LockFailed);
        }
#endif
        return { };
    }

    [[nodiscard]] std::expected<void, FsError> unlock()
    {
        if (!_is_initialized) {
            return std::unexpected(FsError::Mutex_NotInitialized);
        }

#ifdef PPFS_USE_FREERTOS
        if (xSemaphoreGive(_mutex_handle) != pdTRUE) {
            return std::unexpected(FsError::Mutex_UnlockFailed);
        }
#else
        if (pthread_mutex_unlock(&_mutex_handle) != 0) {
            return std::unexpected(FsError::Mutex_UnlockFailed);
        }
#endif
        return { };
    }

    bool isInitialized() const { return _is_initialized; }

    // Disable copying
    PpFSMutex(const PpFSMutex&) = delete;
    PpFSMutex& operator=(const PpFSMutex&) = delete;

private:
    bool _is_initialized;

#ifdef PPFS_USE_FREERTOS
    SemaphoreHandle_t _mutex_handle;
#else
    pthread_mutex_t _mutex_handle;
#endif
};