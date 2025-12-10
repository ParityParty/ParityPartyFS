#include "filesystem/ppfs.hpp"
#include "common/math_helpers.hpp"
#include "common/ppfs_mutex.hpp"
#include <cstring>
#include <numeric>

#include "blockdevice/crc_block_device.hpp"
#include "blockdevice/hamming_block_device.hpp"
#include "blockdevice/iblock_device.hpp"
#include "blockdevice/parity_block_device.hpp"
#include "blockdevice/raw_block_device.hpp"
#include "blockdevice/rs_block_device.hpp"

#include <mutex>

PpFS::PpFS(IDisk& disk, std::shared_ptr<Logger> logger)
    : _disk(disk)
    , _logger(logger)
{
}

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

bool PpFS::isInitialized() const
{
    return _blockDevice && _inodeManager && _blockManager && _directoryManager && _fileIO
        && _superBlockManager && _mutex.isInitialized();
}
std::expected<std::size_t, FsError> PpFS::getFileCount()
{
    return mutex_wrapper<std::size_t>(_mutex, [&]() { return _unprotectedGetFileCount(); });
}

std::expected<void, FsError> PpFS::_createAppropriateBlockDevice(
    size_t block_size, ECCType eccType, std::uint64_t polynomial, std::uint32_t correctable_bytes)
{
    switch (eccType) {
    case ECCType::None: {
        _blockDeviceStorage.emplace<RawBlockDevice>(block_size, _disk);
        _blockDevice = &std::get<RawBlockDevice>(_blockDeviceStorage);
        break;
    }
    case ECCType::Parity: {
        _blockDeviceStorage.emplace<ParityBlockDevice>(block_size, _disk, _logger);
        _blockDevice = &std::get<ParityBlockDevice>(_blockDeviceStorage);
        break;
    }
    case ECCType::Crc: {
        auto crc_polynomial = CrcPolynomial::MsgExplicit(polynomial);
        _blockDeviceStorage.emplace<CrcBlockDevice>(crc_polynomial, _disk, block_size, _logger);
        _blockDevice = &std::get<CrcBlockDevice>(_blockDeviceStorage);
        break;
    }
    case ECCType::Hamming: {
        _blockDeviceStorage.emplace<HammingBlockDevice>(binLog(block_size), _disk, _logger);
        _blockDevice = &std::get<HammingBlockDevice>(_blockDeviceStorage);
        break;
    }
    case ECCType::ReedSolomon: {
        _blockDeviceStorage.emplace<ReedSolomonBlockDevice>(
            _disk, block_size, correctable_bytes, _logger);
        _blockDevice = &std::get<ReedSolomonBlockDevice>(_blockDeviceStorage);
        break;
    }
    default:
        return std::unexpected(FsError::PpFS_InvalidRequest);
    }
    return {};
}

std::expected<void, FsError> PpFS::init()
{
    // Create superblock manager
    _superBlockManagerStorage.emplace<SuperBlockManager>(_disk);
    _superBlockManager = &std::get<SuperBlockManager>(_superBlockManagerStorage);
    auto sb_res = _superBlockManager->get();
    if (!sb_res.has_value()) {
        return std::unexpected(sb_res.error());
    }
    _superBlock = sb_res.value();
    auto block_size = _superBlock.block_size;

    // Create block device with appropriate ECC
    auto bd_res = _createAppropriateBlockDevice(block_size, _superBlock.ecc_type,
        _superBlock.crc_polynomial, _superBlock.rs_correctable_bytes);
    if (!bd_res.has_value()) {
        return std::unexpected(bd_res.error());
    }

    // Create inode manager
    _inodeManagerStorage.emplace<InodeManager>(*_blockDevice, _superBlock);
    _inodeManager = &std::get<InodeManager>(_inodeManagerStorage);

    // Create block manager
    _blockManagerStorage.emplace<BlockManager>(_superBlock, *_blockDevice);
    _blockManager = &std::get<BlockManager>(_blockManagerStorage);

    // Create file IO
    _fileIOStorage.emplace<FileIO>(*_blockDevice, *_blockManager, *_inodeManager);
    _fileIO = &std::get<FileIO>(_fileIOStorage);

    // Create directory manager
    _directoryManagerStorage.emplace<DirectoryManager>(*_blockDevice, *_inodeManager, *_fileIO);
    _directoryManager = &std::get<DirectoryManager>(_directoryManagerStorage);

    auto mutex_init = _mutex.init();
    if (!mutex_init.has_value()) {
        return mutex_init;
    }

    return {};
}

std::expected<void, FsError> PpFS::format(FsConfig options)
{
    // Check if parameters were set
    if (options.total_size == 0 || options.block_size == 0 || options.average_file_size == 0) {
        return std::unexpected(FsError::PpFS_InvalidRequest);
    }
    // Check if total size is a multiple of block size
    if (options.total_size % options.block_size != 0) {
        return std::unexpected(FsError::PpFS_InvalidRequest);
    }
    // Check if block size is a power of two
    if ((options.block_size & (options.block_size - 1)) != 0) {
        return std::unexpected(FsError::PpFS_InvalidRequest);
    }

    // Create block device with appropriate ECC
    auto bd_res = _createAppropriateBlockDevice(options.block_size, options.ecc_type,
        options.crc_polynomial.getExplicitPolynomial(), options.rs_correctable_bytes);
    if (!bd_res.has_value()) {
        return std::unexpected(bd_res.error());
    }
    auto data_block_size = _blockDevice->dataSize();

    // Create superblock
    SuperBlock sb {};
    sb.total_blocks = options.total_size / options.block_size;
    sb.total_inodes = options.total_size / options.average_file_size;
    sb.inode_bitmap_address = divCeil(sizeof(SuperBlock) * 2, data_block_size);
    sb.inode_table_address
        = sb.inode_bitmap_address + divCeil((size_t)divCeil(sb.total_inodes, 8U), data_block_size);

    if (options.use_journal) {
        return std::unexpected(FsError::NotImplemented);
    }

    sb.block_bitmap_address
        = sb.inode_table_address + divCeil((sb.total_inodes * sizeof(Inode)), data_block_size);
    sb.first_data_blocks_address = sb.block_bitmap_address
        + divCeil(divCeil((size_t)(sb.total_blocks), 8UL), data_block_size);
    sb.last_data_block_address = sb.total_blocks - divCeil(sizeof(SuperBlock), data_block_size);
    sb.block_size = options.block_size;
    sb.ecc_type = options.ecc_type;
    if (sb.ecc_type == ECCType::Crc)
        sb.crc_polynomial = options.crc_polynomial.getExplicitPolynomial();
    if (sb.ecc_type == ECCType::ReedSolomon)
        sb.rs_correctable_bytes = options.rs_correctable_bytes;

    // Validate superblock
    if (sb.total_blocks == 0 || sb.total_inodes == 0) {
        return std::unexpected(FsError::PpFS_InvalidRequest);
    }
    if (sb.first_data_blocks_address >= sb.last_data_block_address) {
        return std::unexpected(FsError::PpFS_InvalidRequest);
    }
    if (sb.last_data_block_address >= sb.total_blocks) {
        return std::unexpected(FsError::PpFS_InvalidRequest);
    }

    // Write superblock to disk
    _superBlockManagerStorage.emplace<SuperBlockManager>(_disk);
    _superBlockManager = &std::get<SuperBlockManager>(_superBlockManagerStorage);
    auto put_res = _superBlockManager->put(sb);
    if (!put_res.has_value()) {
        return std::unexpected(put_res.error());
    }
    _superBlock = sb;

    // Create and format inode manager
    _inodeManagerStorage.emplace<InodeManager>(*_blockDevice, _superBlock);
    _inodeManager = &std::get<InodeManager>(_inodeManagerStorage);
    auto format_inode_res = _inodeManager->format();
    if (!format_inode_res.has_value()) {
        return std::unexpected(format_inode_res.error());
    }

    // Create and format block manager
    _blockManagerStorage.emplace<BlockManager>(_superBlock, *_blockDevice);
    _blockManager = &std::get<BlockManager>(_blockManagerStorage);
    auto format_block_res = _blockManager->format();
    if (!format_block_res.has_value()) {
        return std::unexpected(format_block_res.error());
    }

    // Create file IO
    _fileIOStorage.emplace<FileIO>(*_blockDevice, *_blockManager, *_inodeManager);
    _fileIO = &std::get<FileIO>(_fileIOStorage);

    // Create directory manager
    _directoryManagerStorage.emplace<DirectoryManager>(*_blockDevice, *_inodeManager, *_fileIO);
    _directoryManager = &std::get<DirectoryManager>(_directoryManagerStorage);

    auto mutex_init = _mutex.init();
    if (!mutex_init.has_value()) {
        return mutex_init;
    }

    return {};
}
std::expected<void, FsError> PpFS::create(std::string_view path)
{
    return mutex_wrapper<void>(_mutex, [&]() { return _unprotectedCreate(path); });
}
std::expected<file_descriptor_t, FsError> PpFS::open(std::string_view path, OpenMode mode)
{
    return mutex_wrapper<file_descriptor_t>(_mutex, [&]() { return _unprotectedOpen(path, mode); });
}
std::expected<void, FsError> PpFS::close(file_descriptor_t fd)
{
    return mutex_wrapper<void>(_mutex, [&]() { return _unprotectedClose(fd); });
}
std::expected<void, FsError> PpFS::remove(std::string_view path, bool recursive)
{
    return mutex_wrapper<void>(_mutex, [&]() { return _unprotectedRemove(path, recursive); });
}
std::expected<std::vector<std::uint8_t>, FsError> PpFS::read(
    file_descriptor_t fd, std::size_t bytes_to_read)
{
    return mutex_wrapper<std::vector<std::uint8_t>>(
        _mutex, [&]() { return _unprotectedRead(fd, bytes_to_read); });
}
std::expected<void, FsError> PpFS::write(file_descriptor_t fd, std::vector<std::uint8_t> buffer)
{
    return mutex_wrapper<void>(_mutex, [&]() { return _unprotectedWrite(fd, buffer); });
}
std::expected<void, FsError> PpFS::seek(file_descriptor_t fd, size_t position)
{
    return mutex_wrapper<void>(_mutex, [&]() { return _unprotectedSeek(fd, position); });
}
std::expected<void, FsError> PpFS::createDirectory(std::string_view path)
{
    return mutex_wrapper<void>(_mutex, [&]() { return _unprotectedCreateDirectory(path); });
}
std::expected<std::vector<std::string>, FsError> PpFS::readDirectory(std::string_view path)
{
    return mutex_wrapper<std::vector<std::string>>(
        _mutex, [&]() { return _unprotectedReadDirectory(path); });
}

std::expected<void, FsError> PpFS::_unprotectedCreate(std::string_view path)
{
    if (!isInitialized()) {
        return std::unexpected(FsError::PpFS_NotInitialized);
    }

    if (!_isPathValid(path)) {
        return std::unexpected(FsError::PpFS_InvalidPath);
    }

    // Get parent inode
    auto parent_inode_res = _getParentInodeFromPath(path);
    if (!parent_inode_res.has_value()) {
        return std::unexpected(parent_inode_res.error());
    }
    inode_index_t parent_inode = parent_inode_res.value();

    // Check if parent inode does not already contain an entry with the same name
    size_t last_slash = path.find_last_of('/');
    std::string_view filename = path.substr(last_slash + 1);
    auto unique_res = _directoryManager->checkNameUnique(parent_inode, filename.data());
    if (!unique_res.has_value()) {
        return std::unexpected(unique_res.error());
    }

    // Create new inode
    Inode new_inode { .type = InodeType::File };
    auto create_inode_res = _inodeManager->create(new_inode);
    if (!create_inode_res.has_value()) {
        return std::unexpected(create_inode_res.error());
    }
    inode_index_t new_inode_index = create_inode_res.value();

    // Add entry to parent directory
    DirectoryEntry new_entry { .inode = new_inode_index };
    std::strncpy(new_entry.name.data(), filename.data(), new_entry.name.size() - 1);
    auto add_entry_res = _directoryManager->addEntry(parent_inode, new_entry);
    if (!add_entry_res.has_value()) {
        return std::unexpected(add_entry_res.error());
    }

    return {};
}

bool PpFS::_isPathValid(std::string_view path)
{
    if (path.empty() || path.front() != '/') {
        return false;
    }
    // Check if path does not contain double slashes
    for (size_t i = 1; i < path.size(); ++i) {
        if (path[i] == '/' && path[i - 1] == '/') {
            return false;
        }
    }
    return true;
}

std::expected<inode_index_t, FsError> PpFS::_getParentInodeFromPath(std::string_view path)
{
    inode_index_t current_inode = _root;

    while (true) {
        size_t next_slash = path.find('/', 1);
        if (next_slash == std::string_view::npos) {
            return current_inode;
        }
        std::string_view next_name = path.substr(1, next_slash - 1);

        auto entry_res = _directoryManager->getEntries(current_inode);
        if (!entry_res.has_value()) {
            return std::unexpected(entry_res.error());
        }
        const auto& entries = entry_res.value();

        bool found = false;
        for (const auto& entry : entries) {
            if (next_name != entry.name.data())
                continue;
            current_inode = entry.inode;
            found = true;
            break;
        }

        if (!found) {
            return std::unexpected(FsError::PpFS_NotFound);
        }
        path = path.substr(next_slash);
    }
}

std::expected<inode_index_t, FsError> PpFS::_getInodeFromParent(
    inode_index_t parent_inode, std::string_view path)
{
    size_t last_slash = path.find_last_of('/');
    std::string_view name = path.substr(last_slash + 1);

    auto entries_res = _directoryManager->getEntries(parent_inode);
    if (!entries_res.has_value()) {
        return std::unexpected(entries_res.error());
    }
    const auto& entries = entries_res.value();

    for (const auto& entry : entries) {
        if (name == entry.name.data()) {
            return entry.inode;
        }
    }
    return std::unexpected(FsError::PpFS_NotFound);
}

std::expected<inode_index_t, FsError> PpFS::_getInodeFromPath(std::string_view path)
{
    if (path == "/") {
        return _root;
    }

    auto parent_inode_res = _getParentInodeFromPath(path);
    if (!parent_inode_res.has_value()) {
        return std::unexpected(parent_inode_res.error());
    }
    inode_index_t parent_inode = parent_inode_res.value();

    return _getInodeFromParent(parent_inode, path);
}

std::expected<file_descriptor_t, FsError> PpFS::_unprotectedOpen(
    std::string_view path, OpenMode mode)
{
    if (!isInitialized()) {
        return std::unexpected(FsError::PpFS_NotInitialized);
    }
    if (!_isPathValid(path)) {
        return std::unexpected(FsError::PpFS_InvalidPath);
    }

    auto inode_res = _getInodeFromPath(path);
    if (!inode_res.has_value()) {
        return std::unexpected(inode_res.error());
    }
    inode_index_t inode = inode_res.value();

    auto open_res = _openFilesTable.open(inode, mode);
    if (!open_res.has_value()) {
        return std::unexpected(open_res.error());
    }

    if (mode & OpenMode::Truncate) {
        auto inode_data_res = _inodeManager->get(inode);
        if (!inode_data_res.has_value()) {
            return std::unexpected(inode_data_res.error());
        }
        Inode inode_data = inode_data_res.value();

        auto truncate_res = _fileIO->resizeFile(inode, inode_data, 0);
        if (!truncate_res.has_value()) {
            return std::unexpected(truncate_res.error());
        }
    }

    return open_res.value();
}

std::expected<void, FsError> PpFS::_unprotectedClose(file_descriptor_t fd)
{
    if (!isInitialized()) {
        return std::unexpected(FsError::PpFS_NotInitialized);
    }
    auto close_res = _openFilesTable.close(fd);
    if (!close_res.has_value()) {
        return std::unexpected(close_res.error());
    }
    return {};
}

std::expected<void, FsError> PpFS::_checkIfInUseRecursive(inode_index_t inode)
{
    auto inode_res = _inodeManager->get(inode);
    if (!inode_res.has_value()) {
        return std::unexpected(inode_res.error());
    }
    Inode inode_data = inode_res.value();
    if (inode_data.type != InodeType::Directory) {
        auto open_file_res = _openFilesTable.get(inode);
        if (open_file_res.has_value()) {
            return std::unexpected(FsError::PpFS_FileInUse);
        }
        return {};
    }

    // If directory, check entries recursively
    auto entries_res = _directoryManager->getEntries(inode);
    if (!entries_res.has_value()) {
        return std::unexpected(entries_res.error());
    }
    const auto& entries = entries_res.value();
    for (const auto& entry : entries) {
        auto check_res = _checkIfInUseRecursive(entry.inode);
        if (!check_res.has_value()) {
            return std::unexpected(check_res.error());
        }
    }

    return {};
}

std::expected<void, FsError> PpFS::_removeRecursive(inode_index_t parent, inode_index_t inode)
{
    auto inode_res = _inodeManager->get(inode);
    if (!inode_res.has_value()) {
        return std::unexpected(inode_res.error());
    }
    Inode inode_data = inode_res.value();
    if (inode_data.type == InodeType::Directory) {
        auto entries_res = _directoryManager->getEntries(inode);
        if (!entries_res.has_value()) {
            return std::unexpected(entries_res.error());
        }
        const auto& entries = entries_res.value();
        for (const auto& entry : entries) {
            auto remove_res = _removeRecursive(inode, entry.inode);
            if (!remove_res.has_value()) {
                return std::unexpected(remove_res.error());
            }
        }
    }

    auto remove_entry_res = _directoryManager->removeEntry(parent, inode);
    if (!remove_entry_res.has_value()) {
        return std::unexpected(remove_entry_res.error());
    }

    auto remove_inode_res = _inodeManager->remove(inode);
    if (!remove_inode_res.has_value()) {
        return std::unexpected(remove_inode_res.error());
    }
    return {};
}

std::expected<void, FsError> PpFS::_unprotectedRemove(std::string_view path, bool recursive)
{
    if (!isInitialized()) {
        return std::unexpected(FsError::PpFS_NotInitialized);
    }
    if (!_isPathValid(path)) {
        return std::unexpected(FsError::PpFS_InvalidPath);
    }

    auto parent_inode_res = _getParentInodeFromPath(path);
    if (!parent_inode_res.has_value()) {
        return std::unexpected(parent_inode_res.error());
    }
    inode_index_t parent_inode = parent_inode_res.value();

    auto inode_res = _getInodeFromParent(parent_inode, path);
    if (!inode_res.has_value()) {
        return std::unexpected(inode_res.error());
    }
    inode_index_t inode = inode_res.value();

    if (!recursive) {
        // If directory, check if empty
        auto inode_data_res = _inodeManager->get(inode);
        if (!inode_data_res.has_value()) {
            return std::unexpected(inode_data_res.error());
        }
        Inode inode_data = inode_data_res.value();
        if (inode_data.type == InodeType::Directory && inode_data.file_size > 0) {
            return std::unexpected(FsError::PpFS_DirectoryNotEmpty);
        }
    }

    auto check_in_use_res = _checkIfInUseRecursive(inode);
    if (!check_in_use_res.has_value()) {
        return std::unexpected(check_in_use_res.error());
    }

    auto remove_res = _removeRecursive(parent_inode, inode);
    if (!remove_res.has_value()) {
        return std::unexpected(remove_res.error());
    }
    return {};
}

std::expected<std::vector<std::uint8_t>, FsError> PpFS::_unprotectedRead(
    file_descriptor_t fd, std::size_t bytes_to_read)
{
    if (!isInitialized()) {
        return std::unexpected(FsError::PpFS_NotInitialized);
    }

    auto open_table_res = _openFilesTable.get(fd);
    if (!open_table_res.has_value()) {
        return std::unexpected(FsError::PpFS_NotFound);
    }
    OpenFile* open_file = open_table_res.value();

    if (open_file->mode & OpenMode::Append) {
        return std::unexpected(FsError::PpFS_InvalidRequest);
    }

    auto inode_res = _inodeManager->get(open_file->inode);
    if (!inode_res.has_value()) {
        return std::unexpected(inode_res.error());
    }
    Inode inode = inode_res.value();

    auto read_res = _fileIO->readFile(open_file->inode, inode, open_file->position, bytes_to_read);
    if (!read_res.has_value()) {
        return std::unexpected(read_res.error());
    }
    open_file->position += read_res.value().size();

    return read_res;
}

std::expected<void, FsError> PpFS::_unprotectedWrite(
    file_descriptor_t fd, std::vector<std::uint8_t> buffer)
{
    if (!isInitialized()) {
        return std::unexpected(FsError::PpFS_NotInitialized);
    }

    auto open_table_res = _openFilesTable.get(fd);
    if (!open_table_res.has_value()) {
        return std::unexpected(FsError::PpFS_NotFound);
    }
    OpenFile* open_file = open_table_res.value();

    auto inode_res = _inodeManager->get(open_file->inode);
    if (!inode_res.has_value()) {
        return std::unexpected(inode_res.error());
    }
    Inode inode = inode_res.value();

    size_t offset = open_file->position;
    if (open_file->mode & OpenMode::Append) {
        offset = inode.file_size;
    }

    auto write_res = _fileIO->writeFile(open_file->inode, inode, offset, buffer);
    if (!write_res.has_value()) {
        return std::unexpected(write_res.error());
    }

    open_file->position = offset + buffer.size();
    return {};
}

std::expected<void, FsError> PpFS::_unprotectedSeek(file_descriptor_t fd, size_t position)
{
    if (!isInitialized()) {
        return std::unexpected(FsError::PpFS_NotInitialized);
    }

    auto open_table_res = _openFilesTable.get(fd);
    if (!open_table_res.has_value()) {
        return std::unexpected(FsError::PpFS_NotFound);
    }
    OpenFile* open_file = open_table_res.value();

    if (open_file->mode & OpenMode::Append) {
        return std::unexpected(FsError::PpFS_InvalidRequest);
    }

    auto inode_res = _inodeManager->get(open_file->inode);
    if (!inode_res.has_value()) {
        return std::unexpected(inode_res.error());
    }
    Inode inode = inode_res.value();

    if (position > inode.file_size) {
        return std::unexpected(FsError::PpFS_OutOfBounds);
    }

    open_file->position = position;

    return {};
}

std::expected<void, FsError> PpFS::_unprotectedCreateDirectory(std::string_view path)
{
    if (!isInitialized()) {
        return std::unexpected(FsError::PpFS_NotInitialized);
    }

    if (!_isPathValid(path)) {
        return std::unexpected(FsError::PpFS_InvalidPath);
    }

    // Get parent inode
    auto parent_inode_res = _getParentInodeFromPath(path);
    if (!parent_inode_res.has_value()) {
        return std::unexpected(parent_inode_res.error());
    }
    inode_index_t parent_inode = parent_inode_res.value();

    // Check if parent inode does not already contain an entry with the same name
    size_t last_slash = path.find_last_of('/');
    std::string_view dirname = path.substr(last_slash + 1);
    auto unique_res = _directoryManager->checkNameUnique(parent_inode, dirname.data());
    if (!unique_res.has_value()) {
        return std::unexpected(unique_res.error());
    }

    // Create new inode
    Inode new_inode { .type = InodeType::Directory };
    auto create_inode_res = _inodeManager->create(new_inode);
    if (!create_inode_res.has_value()) {
        return std::unexpected(create_inode_res.error());
    }
    inode_index_t new_inode_index = create_inode_res.value();

    // Add entry to parent directory
    DirectoryEntry new_entry { .inode = new_inode_index };
    std::strncpy(new_entry.name.data(), dirname.data(), new_entry.name.size() - 1);
    auto add_entry_res = _directoryManager->addEntry(parent_inode, new_entry);
    if (!add_entry_res.has_value()) {
        return std::unexpected(add_entry_res.error());
    }

    return {};
}

std::expected<std::vector<std::string>, FsError> PpFS::_unprotectedReadDirectory(
    std::string_view path)
{
    if (!isInitialized()) {
        return std::unexpected(FsError::PpFS_NotInitialized);
    }
    if (!_isPathValid(path)) {
        return std::unexpected(FsError::PpFS_InvalidPath);
    }

    auto dir_inode_res = _getInodeFromPath(path);
    if (!dir_inode_res.has_value()) {
        return std::unexpected(dir_inode_res.error());
    }
    inode_index_t dir_inode = dir_inode_res.value();

    auto entries_res = _directoryManager->getEntries(dir_inode);
    if (!entries_res.has_value()) {
        return std::unexpected(entries_res.error());
    }
    const auto& entries = entries_res.value();
    std::vector<std::string> names;
    names.reserve(entries.size());

    for (const auto& entry : entries) {
        names.emplace_back(entry.name.data());
    }
    return names;
}

std::expected<std::size_t, FsError> PpFS::_unprotectedGetFileCount() const
{
    if (!isInitialized()) {
        return std::unexpected(FsError::PpFS_NotInitialized);
    }

    auto free_res = _inodeManager->numFree();
    if (!free_res.has_value()) {
        return std::unexpected(free_res.error());
    }

    return _superBlock.total_inodes - free_res.value();
}