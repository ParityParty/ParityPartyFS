#include "disk/file_disk.hpp"

#include <cerrno>
#include <cstring>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

FileDisk::FileDisk(const char* path, size_t size)
    : _size(size)
{
    _fd = ::open(path, O_RDWR | O_CREAT, 0644);
    if (_fd < 0) {
        std::abort();
    }

    struct stat st {};
    if (::fstat(_fd, &st) != 0) {
        std::abort();
    }

    if (static_cast<size_t>(st.st_size) < _size) {
        if (::ftruncate(_fd, static_cast<off_t>(_size)) != 0) {
            std::abort();
        }
    }
}

FileDisk::~FileDisk()
{
    if (_fd >= 0) {
        ::close(_fd);
    }
}

size_t FileDisk::size() { return _size; }

std::expected<void, FsError> FileDisk::read(
    size_t address, size_t size, static_vector<uint8_t>& data)
{
    if (address + size > _size) {
        return std::unexpected(FsError::Disk_OutOfBounds);
    }

    if (data.capacity() < size) {
        return std::unexpected(FsError::Disk_InvalidRequest);
    }

    data.resize(size);

    ssize_t r = ::pread(_fd, data.data(), size, static_cast<off_t>(address));

    if (r != static_cast<ssize_t>(size)) {
        return std::unexpected(FsError::Disk_InvalidRequest);
    }

    return {};
}

std::expected<size_t, FsError> FileDisk::write(size_t address, const static_vector<uint8_t>& data)
{
    if (address + data.size() > _size) {
        return std::unexpected(FsError::Disk_OutOfBounds);
    }

    ssize_t w = ::pwrite(_fd, data.data(), data.size(), static_cast<off_t>(address));

    if (w != static_cast<ssize_t>(data.size())) {
        return std::unexpected(FsError::Disk_InvalidRequest);
    }

    return data.size();
}
