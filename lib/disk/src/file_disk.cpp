#include "ppfs/disk/file_disk.hpp"

#include <cstring>
#include <filesystem>
#include <fstream>

FileDisk::FileDisk() = default;

FileDisk::~FileDisk()
{
    if (_file.is_open()) {
        _file.close();
    }
}

std::expected<void, FsError> FileDisk::open(std::string_view path)
{
    if (_file.is_open()) {
        _file.close();
    }

    _file.open(path.data(), std::ios::binary | std::ios::in | std::ios::out);

    if (!_file.is_open()) {
        return std::unexpected(FsError::Disk_IOError);
    }

    _file.seekg(0, std::ios::end);
    _size = static_cast<size_t>(_file.tellg());
    _file.seekg(0, std::ios::beg);

    return {};
}

std::expected<void, FsError> FileDisk::create(std::string_view path, size_t size)
{
    if (_file.is_open())
        return std::unexpected(FsError::Disk_InvalidRequest);

    std::ofstream ofs(path.data(), std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        return std::unexpected(FsError::Disk_IOError);
    }

    if (size > 0) {
        ofs.seekp(size - 1);
        ofs.put(0);
    }

    ofs.close();
    return open(path);
}

size_t FileDisk::size() { return _size; }

std::expected<void, FsError> FileDisk::read(
    size_t address, size_t size, static_vector<uint8_t>& data)
{
    if (!_file.is_open()) {
        return std::unexpected(FsError::Disk_IOError);
    }

    if (address + size > _size) {
        return std::unexpected(FsError::Disk_OutOfBounds);
    }

    if (data.capacity() < size) {
        return std::unexpected(FsError::Disk_InvalidRequest);
    }

    data.resize(size);

    _file.seekg(address, std::ios::beg);
    _file.read(reinterpret_cast<char*>(data.data()), size);

    if (!_file.good()) {
        return std::unexpected(FsError::Disk_IOError);
    }

    return {};
}

std::expected<size_t, FsError> FileDisk::write(size_t address, const static_vector<uint8_t>& data)
{
    if (!_file.is_open()) {
        return std::unexpected(FsError::Disk_IOError);
    }

    if (address + data.size() > _size) {
        return std::unexpected(FsError::Disk_OutOfBounds);
    }

    _file.seekp(address, std::ios::beg);
    _file.write(reinterpret_cast<const char*>(data.data()), data.size());

    if (!_file.good()) {
        return std::unexpected(FsError::Disk_IOError);
    }

    return data.size();
}
