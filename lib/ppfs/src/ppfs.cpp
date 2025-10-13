#include "ppfs/ppfs.hpp"
#include "ppfs/data_structures.hpp"

#include <iostream>
#include <ostream>

template <class T> Fusepp::t_getattr Fusepp::Fuse<T>::getattr = nullptr;
template <class T> Fusepp::t_readlink Fusepp::Fuse<T>::readlink = nullptr;
template <class T> Fusepp::t_mknod Fusepp::Fuse<T>::mknod = nullptr;
template <class T> Fusepp::t_mkdir Fusepp::Fuse<T>::mkdir = nullptr;
template <class T> Fusepp::t_unlink Fusepp::Fuse<T>::unlink = nullptr;
template <class T> Fusepp::t_rmdir Fusepp::Fuse<T>::rmdir = nullptr;
template <class T> Fusepp::t_symlink Fusepp::Fuse<T>::symlink = nullptr;
template <class T> Fusepp::t_rename Fusepp::Fuse<T>::rename = nullptr;
template <class T> Fusepp::t_link Fusepp::Fuse<T>::link = nullptr;
template <class T> Fusepp::t_chmod Fusepp::Fuse<T>::chmod = nullptr;
template <class T> Fusepp::t_chown Fusepp::Fuse<T>::chown = nullptr;
template <class T> Fusepp::t_truncate Fusepp::Fuse<T>::truncate = nullptr;
template <class T> Fusepp::t_open Fusepp::Fuse<T>::open = nullptr;
template <class T> Fusepp::t_read Fusepp::Fuse<T>::read = nullptr;
template <class T> Fusepp::t_write Fusepp::Fuse<T>::write = nullptr;
template <class T> Fusepp::t_statfs Fusepp::Fuse<T>::statfs = nullptr;
template <class T> Fusepp::t_flush Fusepp::Fuse<T>::flush = nullptr;
template <class T> Fusepp::t_release Fusepp::Fuse<T>::release = nullptr;
template <class T> Fusepp::t_fsync Fusepp::Fuse<T>::fsync = nullptr;
template <class T> Fusepp::t_setxattr Fusepp::Fuse<T>::setxattr = nullptr;
template <class T> Fusepp::t_getxattr Fusepp::Fuse<T>::getxattr = nullptr;
template <class T> Fusepp::t_listxattr Fusepp::Fuse<T>::listxattr = nullptr;
template <class T> Fusepp::t_removexattr Fusepp::Fuse<T>::removexattr = nullptr;
template <class T> Fusepp::t_opendir Fusepp::Fuse<T>::opendir = nullptr;
template <class T> Fusepp::t_readdir Fusepp::Fuse<T>::readdir = nullptr;
template <class T> Fusepp::t_releasedir Fusepp::Fuse<T>::releasedir = nullptr;
template <class T> Fusepp::t_fsyncdir Fusepp::Fuse<T>::fsyncdir = nullptr;
template <class T> Fusepp::t_init Fusepp::Fuse<T>::init = nullptr;
template <class T> Fusepp::t_destroy Fusepp::Fuse<T>::destroy = nullptr;
template <class T> Fusepp::t_access Fusepp::Fuse<T>::access = nullptr;
template <class T> Fusepp::t_create Fusepp::Fuse<T>::create = nullptr;
template <class T> Fusepp::t_lock Fusepp::Fuse<T>::lock = nullptr;
template <class T> Fusepp::t_utimens Fusepp::Fuse<T>::utimens = nullptr;
template <class T> Fusepp::t_bmap Fusepp::Fuse<T>::bmap = nullptr;
template <class T> Fusepp::t_ioctl Fusepp::Fuse<T>::ioctl = nullptr;
template <class T> Fusepp::t_poll Fusepp::Fuse<T>::poll = nullptr;
template <class T> Fusepp::t_write_buf Fusepp::Fuse<T>::write_buf = nullptr;
template <class T> Fusepp::t_read_buf Fusepp::Fuse<T>::read_buf = nullptr;
template <class T> Fusepp::t_flock Fusepp::Fuse<T>::flock = nullptr;
template <class T> Fusepp::t_fallocate Fusepp::Fuse<T>::fallocate = nullptr;

template <class T> struct fuse_operations Fusepp::Fuse<T>::operations_;

PpFS::PpFS(IDisk& disk)
    : _disk(disk)
{
    // Always format disk for now
    if (const auto ret = formatDisk(512); !ret.has_value()) {
        throw std::runtime_error("PpFS::formatDisk failed");
    }
}

int PpFS::getattr(const char* path, struct stat* stBuffer, struct fuse_file_info*)
{
    const auto ptr = this_();
    const std::string str_path(path);
    int res = 0;

    memset(stBuffer, 0, sizeof(struct stat));
    if (path == ptr->rootPath()) {
        stBuffer->st_mode = S_IFDIR | 0755;
        stBuffer->st_nlink = 2;
    } else if (str_path.substr(0, ptr->rootPath().length()) == ptr->rootPath()) {
        std::string file_name = str_path.substr(ptr->rootPath().length(), str_path.length());
        auto dir_entry_opt = ptr->_root_dir.findFile(file_name);
        if (dir_entry_opt.has_value()) {
            stBuffer->st_mode = S_IFREG | 0666;
            stBuffer->st_nlink = 1;
            stBuffer->st_size = dir_entry_opt.value().get().file_size;
        } else {
            res = -ENOENT;
        }
    } else
        res = -ENOENT;

    return res;
}

int PpFS::readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t,
    struct fuse_file_info*, enum fuse_readdir_flags)
{
    const auto ptr = this_();

    if (path != ptr->rootPath())
        return -ENOENT;

    filler(buf, ".", nullptr, 0, FUSE_FILL_DIR_PLUS);
    filler(buf, "..", nullptr, 0, FUSE_FILL_DIR_PLUS);
    for (auto entry : ptr->_root_dir.entries) {
        filler(buf, entry.fileNameStr().c_str(), nullptr, 0, FUSE_FILL_DIR_PLUS);
    }

    return 0;
}

int PpFS::open(const char* path, struct fuse_file_info* fi)
{
    const auto ptr = this_();

    std::string str_path(path);
    if (str_path.substr(0, ptr->rootPath().length()) != ptr->rootPath()) {
        return -ENOENT;
    }

    if (str_path == ptr->rootPath()) {
        return -EISDIR;
    }

    std::string file = str_path.substr(ptr->rootPath().length());
    auto ret = ptr->_root_dir.findFile(file);
    if (!ret.has_value()) {
        return -ENOENT;
    }
    return 0;
}

int PpFS::read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info*)
{
    const auto ptr = this_();

    std::string str_path(path);
    if (str_path.substr(0, ptr->rootPath().length()) != ptr->rootPath()) {
        return -ENOENT;
    }

    auto file_opt = ptr->_root_dir.findFile(str_path.substr(ptr->rootPath().size()));
    if (!file_opt.has_value()) {
        return -ENOENT;
    }

    auto ret = ptr->_readFile(file_opt.value(), size, offset);
    if (!ret.has_value()) {
        std::cerr << "Error while reading file: " << toString(ret.error()) << std::endl;
        return -EIO;
    }

    std::memcpy(buf, ret.value().data(), ret.value().size());

    return ret.value().size();
}

int PpFS::write(
    const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info*)
{
    const auto ptr = this_();
    std::string str_path(path);
    if (str_path.substr(0, ptr->rootPath().length()) != ptr->rootPath()) {
        return -ENOENT;
    }
    auto ret = ptr->_root_dir.findFile(str_path.substr(ptr->rootPath().length()));
    if (!ret.has_value()) {
        return -ENOENT;
    }

    std::vector<std::byte> data(size);
    std::memcpy(data.data(), buf, size);
    auto ret_write = ptr->_writeFile(ret.value(), data, offset);
    if (!ret_write.has_value()) {
        return -EIO;
    }

    auto ret_flush = ptr->_flushChanges();
    if (!ret_flush.has_value()) {
        return -EIO;
    }

    return size;
}

int PpFS::create(const char* path, mode_t mode, fuse_file_info* fi) { return mknod(path, mode, 0); }

int PpFS::mknod(const char* path, mode_t mode, dev_t dev)
{
    const auto ptr = this_();

    auto path_str = std::string(path);
    if (ptr->rootPath() != path_str.substr(0, ptr->rootPath().length())) {
        return -ENOENT;
    }

    std::string file_name = path_str.substr(ptr->rootPath().length());
    auto dir_entry_opt = ptr->_root_dir.findFile(file_name);
    if (dir_entry_opt.has_value()) {
        // TODO: open file
        return 0;
    }

    auto ret = ptr->_createFile(file_name);
    if (!ret.has_value()) {
        return -EIO;
    }

    ret = ptr->_flushChanges();

    return ret.has_value() ? 0 : -EIO;
}
int PpFS::unlink(const char* path)
{
    auto ptr = this_();
    if (path == ptr->_root_path) {
        return -EISDIR;
    }

    auto file_opt = ptr->_root_dir.findFile(std::string(path).substr(ptr->_root_path.length()));
    if (!file_opt.has_value()) {
        return -ENOENT;
    }

    if (!ptr->_removeFile(file_opt.value()).has_value()) {
        return -EIO;
    }

    auto ret = ptr->_flushChanges();
    if (!ret.has_value()) {
        return -EIO;
    }

    return 0;
}
int PpFS::truncate(const char* path, off_t offset, fuse_file_info* fi)
{
    const auto ptr = this_();
    std::string str_path(path);
    if (str_path.substr(0, ptr->rootPath().length()) != ptr->rootPath()) {
        return -ENOENT;
    }
    auto ret = ptr->_root_dir.findFile(str_path.substr(ptr->rootPath().length()));
    if (!ret.has_value()) {
        return -ENOENT;
    }

    auto trunc_ret = ptr->_truncateFile(ret.value(), offset);
    if (!trunc_ret.has_value()) {
        return -EIO;
    }

    auto flush_ret = ptr->_flushChanges();
    if (!flush_ret.has_value()) {
        return -EIO;
    }

    return 0;
}

std::expected<void, DiskError> PpFS::formatDisk(const unsigned int block_size)
{
    this->_block_size = block_size;

    if (block_size < sizeof(SuperBlock)) {
        return std::unexpected(DiskError::InvalidRequest);
    }
    unsigned int num_blocks = this->_disk.size() / block_size;
    constexpr unsigned int fat_block_start = Layout::SUPERBLOCK_NUM_BLOCKS;
    const unsigned int fat_size = num_blocks * sizeof(block_index_t);

    _super_block = SuperBlock(num_blocks, block_size, fat_block_start, fat_size);

    auto ret = _disk.write(0, _super_block.toBytes());
    if (!ret.has_value()) {
        return std::unexpected(DiskError::IOError);
    }

    auto fat_table = std::vector(num_blocks, FileAllocationTable::FREE_BLOCK);

    _root_dir = Directory(_root_path, {});
    _root_dir_block = Layout::SUPERBLOCK_NUM_BLOCKS + fat_size / block_size + 1;

    fat_table[0] = FileAllocationTable::LAST_BLOCK;
    for (block_index_t i = 1; i < _root_dir_block; i++) {
        fat_table[i] = i + 1;
    }
    fat_table[_root_dir_block - 1] = FileAllocationTable::LAST_BLOCK;
    fat_table[_root_dir_block] = FileAllocationTable::LAST_BLOCK;

    _fat = FileAllocationTable(std::move(fat_table));

    const auto ret1 = _disk.write(0, _super_block.toBytes());
    const auto ret2 = _disk.write(block_size, _fat.toBytes());
    if (!ret1.has_value() || !ret2.has_value()) {
        return std::unexpected(DiskError::IOError);
    }

    const auto root_ret = _disk.write(_root_dir_block * block_size, _root_dir.toBytes());
    if (!root_ret.has_value()) {
        return std::unexpected(DiskError::IOError);
    }

    return {};
}

std::expected<void, DiskError> PpFS::_createFile(const std::string& name)
{
    auto ret = _fat.findFreeBlock();
    if (!ret.has_value()) {
        if (ret.error() == FatError::OutOfDiskSpace)
            return std::unexpected(DiskError::OutOfMemory);
        return std::unexpected(DiskError::InternalError);
    }

    block_index_t block_index = ret.value();

    _root_dir.addEntry({ name, block_index, 0 });
    _fat.setValue(block_index, FileAllocationTable::LAST_BLOCK);

    return {};
}
std::expected<void, DiskError> PpFS::_removeFile(const DirectoryEntry& entry)
{
    auto ret = _fat.freeBlocksFrom(entry.start_block);
    if (!ret.has_value()) {
        std::cerr << "Error in _removeFile: couldn't free blocks" << std::endl;
        return std::unexpected(DiskError::InternalError);
    }
    _root_dir.removeEntry(entry);
    return {};
}
std::expected<std::vector<std::byte>, DiskError> PpFS::_readFile(
    const DirectoryEntry& entry, size_t size, size_t offset)
{
    std::vector<std::byte> data;
    auto ret_find = _findFileOffset(entry, offset);
    if (!ret_find.has_value()) {
        std::cerr << "Error in _readFile: _findFileOffset failed:" << toString(ret_find.error())
                  << std::endl;
        return std::unexpected(ret_find.error());
    }

    size_t data_start = ret_find.value();
    size_t to_read = std::min(_block_size - offset % _block_size, size);
    block_index_t block_ind = data_start / _block_size;

    auto ret = _disk.read(data_start, to_read);
    if (!ret.has_value()) {
        std::cerr << "Error in _readFile: couldn't read from disk:" << toString(ret.error())
                  << std::endl;
        return std::unexpected(ret.error());
    }

    data.insert(data.begin(), ret.value().begin(), ret.value().end());
    size_t read = to_read;

    while (read < size) {
        block_ind = _fat[block_ind];
        if (block_ind == FileAllocationTable::LAST_BLOCK) {
            return data;
        }
        if (block_ind == FileAllocationTable::FREE_BLOCK) {
            std::cerr << "Error in _readFile: fat pointed to free block" << std::endl;
            return std::unexpected(DiskError::InternalError);
        }
        to_read = std::min(static_cast<size_t>(_block_size), size - read);
        auto bytes_ex = _disk.read(
            block_ind * _block_size, std::min(static_cast<size_t>(_block_size), to_read));
        if (!bytes_ex.has_value()) {
            std::cerr << "Error in _readFile: couldn't read from disk" << std::endl;
            return std::unexpected(bytes_ex.error());
        }
        auto bytes = bytes_ex.value();
        data.insert(data.end(), bytes.begin(), bytes.end());
        read += to_read;
    }
    return data;
}
std::expected<void, DiskError> PpFS::_writeFile(
    DirectoryEntry& entry, const std::vector<std::byte>& data, size_t offset)
{
    auto ret = _findFileOffset(entry, offset);
    if (!ret.has_value()) {
        return std::unexpected(ret.error());
    }

    auto ret_write = _writeBytes(data, ret.value());
    if (!ret_write.has_value()) {
        return std::unexpected(ret_write.error());
    }

    size_t new_size = offset + data.size();
    if (new_size > entry.file_size) {
        entry.file_size = new_size;
    }
    return {};
}
std::expected<void, DiskError> PpFS::_writeBytes(const std::vector<std::byte>& data, size_t address)
{
    size_t written = 0;

    block_index_t block = address / _block_size;

    size_t to_write
        = std::min(static_cast<size_t>(_block_size) - address % _block_size, data.size());
    auto ret = _disk.write(address, { data.begin(), data.begin() + to_write });
    if (!ret.has_value()) {
        return std::unexpected(ret.error());
    }

    written = to_write;

    while (written < data.size() && _fat[block] != FileAllocationTable::LAST_BLOCK) {
        auto next_block = _fat[block];
        _fat.setValue(block, next_block);

        to_write = std::min(static_cast<size_t>(_block_size), data.size() - written);
        auto write_ret = _disk.write(next_block * _block_size,
            { data.begin() + written, data.begin() + written + to_write });
        if (!write_ret.has_value()) {
            return std::unexpected(write_ret.error());
        }
        written += to_write;
        block = next_block;
    }

    while (written < data.size()) {
        auto next_block_ex = _fat.findFreeBlock();
        if (!next_block_ex.has_value()) {
            return std::unexpected(DiskError::OutOfMemory);
        }
        auto next_block = next_block_ex.value();
        _fat.setValue(block, next_block);

        to_write = std::min(static_cast<size_t>(_block_size), data.size() - written);
        auto write_ret = _disk.write(next_block * _block_size,
            { data.begin() + written, data.begin() + written + to_write });
        if (!write_ret.has_value()) {
            return std::unexpected(write_ret.error());
        }
        written += to_write;
        block = next_block;
    }
    _fat.setValue(block, FileAllocationTable::LAST_BLOCK);
    return {};
}
std::expected<size_t, DiskError> PpFS::_findFileOffset(const DirectoryEntry& entry, size_t offset)
{
    block_index_t num_blocks = offset / _block_size;
    block_index_t block_ind = entry.start_block;
    block_index_t i = 0;
    while (i++ < num_blocks) {
        block_ind = _fat[block_ind];
        if (block_ind == FileAllocationTable::LAST_BLOCK) {
            return std::unexpected(DiskError::OutOfBounds);
        }
        if (block_ind == FileAllocationTable::FREE_BLOCK) {
            return std::unexpected(DiskError::InternalError);
        }
    }

    return offset % _block_size + block_ind * _block_size;
}
std::expected<void, DiskError> PpFS::_truncateFile(DirectoryEntry& entry, size_t size)
{
    auto offset_ret = _findFileOffset(entry, size);
    if (!offset_ret.has_value()) {
        return std::unexpected(offset_ret.error());
    }

    auto next_block = _fat[offset_ret.value() / _block_size];
    if (next_block != FileAllocationTable::LAST_BLOCK
        && next_block != FileAllocationTable::FREE_BLOCK) {
        auto free_ret = _fat.freeBlocksFrom(next_block);
        if (!free_ret.has_value()) {
            return std::unexpected(DiskError::InternalError);
        }
    }
    entry.file_size = size;
    return {};
}
std::expected<void, DiskError> PpFS::_flushChanges()
{
    auto dir_bytes = _root_dir.toBytes();
    auto ret = _writeBytes(dir_bytes, _root_dir_block * _block_size);
    if (!ret.has_value()) {
        return std::unexpected(ret.error());
    }

    ret = _fat.updateFat(_disk, Layout::FAT_START_BLOCK * _block_size);
    if (!ret.has_value()) {
        return std::unexpected(ret.error());
    }
    return {};
}
