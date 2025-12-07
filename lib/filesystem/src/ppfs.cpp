#include "filesystem/ppfs.hpp"

#include "blockdevice/crc_block_device.hpp"
#include "blockdevice/hamming_block_device.hpp"
#include "blockdevice/iblock_device.hpp"
#include "blockdevice/parity_block_device.hpp"
#include "blockdevice/raw_block_device.hpp"
#include "blockdevice/rs_block_device.hpp"

#include <cstring>

PpFS::PpFS(IDisk& disk)
    : _disk(disk)
{
}

bool PpFS::isInitialized() const
{
    return _superBlockManager != nullptr && _inodeManager != nullptr && _blockManager != nullptr
        && _directoryManager != nullptr && _fileIO != nullptr;
}

std::expected<void, FsError> PpFS::init()
{
    // Create superblock manager
    _superBlockManager = std::make_unique<SuperBlockManager>(_disk);
    auto sb_res = _superBlockManager->get();
    if (!sb_res.has_value()) {
        return std::unexpected(sb_res.error());
    }
    _superBlock = sb_res.value();
    auto block_size = _superBlock.block_size;

    // Create block device with appropriate ECC
    switch (_superBlock.ecc_type) {
    case ECCType::None: {
        _blockDevice = std::make_unique<RawBlockDevice>(block_size, _disk);
        break;
    }
    case ECCType::Parity: {
        _blockDevice = std::make_unique<ParityBlockDevice>(block_size, _disk);
        break;
    }
    case ECCType::Crc: {
        auto crc_polynomial = CrcPolynomial::MsgExplicit(_superBlock.crc_polynomial);
        _blockDevice = std::make_unique<CrcBlockDevice>(crc_polynomial, _disk, block_size);
        break;
    }
    case ECCType::Hamming: {
        _blockDevice = std::make_unique<HammingBlockDevice>(block_size, _disk);
        break;
    }
    case ECCType::ReedSolomon: {
        auto rs_correctable_bytes = _superBlock.rs_correctable_bytes;
        _blockDevice
            = std::make_unique<ReedSolomonBlockDevice>(_disk, block_size, rs_correctable_bytes);
        break;
    }
    default:
        return std::unexpected(FsError::InvalidRequest);
    }

    // Create inode manager
    _inodeManager = std::make_unique<InodeManager>(*_blockDevice, _superBlock);

    // Create block manager
    auto first = _superBlock.first_data_blocks_address;
    auto last = _superBlock.last_data_block_address;
    _blockManager = std::make_unique<BlockManager>(first, last - first + 1, *_blockDevice);

    // Create file IO
    _fileIO = std::make_unique<FileIO>(*_blockDevice, *_blockManager, *_inodeManager);

    // Create directory manager
    _directoryManager = std::make_unique<DirectoryManager>(*_blockDevice, *_inodeManager, *_fileIO);

    return {};
}

std::expected<void, FsError> PpFS::format(FsConfig options)
{
    // Check if parameters were set
    if (options.total_size == 0 || options.block_size == 0 || options.average_file_size == 0) {
        return std::unexpected(FsError::InvalidRequest);
    }
    // Ensure that block size can hold at least one inode and one superblock
    if (options.block_size < sizeof(Inode) || options.block_size < sizeof(SuperBlock)) {
        return std::unexpected(FsError::InvalidRequest);
    }
    // Check if total size is a multiple of block size
    if (options.total_size % options.block_size != 0) {
        return std::unexpected(FsError::InvalidRequest);
    }
    // Check if block size is a power of two
    if ((options.block_size & (options.block_size - 1)) != 0) {
        return std::unexpected(FsError::InvalidRequest);
    }

    // Create superblock
    SuperBlock sb {};
    sb.total_blocks = options.total_size / options.block_size;
    sb.total_inodes = options.total_size / options.average_file_size;
    sb.inode_bitmap_address = sizeof(SuperBlock) * 2 > options.block_size ? 2 : 1;

    sb.inode_table_address = sb.inode_bitmap_address;
    sb.inode_table_address += sb.total_inodes / 8 / options.block_size;
    if ((sb.total_inodes / 8) % options.block_size != 0)
        sb.inode_table_address += 1;

    if (options.use_journal) {
        return std::unexpected(FsError::NotImplemented);
    }

    sb.block_bitmap_address = sb.inode_table_address;
    sb.block_bitmap_address += (sb.total_inodes * sizeof(Inode)) / options.block_size;
    if ((sb.total_inodes * sizeof(Inode)) % options.block_size != 0)
        sb.block_bitmap_address += 1;

    sb.first_data_blocks_address = sb.block_bitmap_address;
    sb.first_data_blocks_address += sb.total_blocks / 8 / options.block_size;
    if ((sb.total_blocks / 8) % options.block_size != 0)
        sb.first_data_blocks_address += 1;

    sb.last_data_block_address = sb.total_blocks - 1;
    sb.block_size = options.block_size;
    sb.ecc_type = options.ecc_type;
    if (sb.ecc_type == ECCType::Crc)
        sb.crc_polynomial = options.crc_polynomial.getExplicitPolynomial();
    if (sb.ecc_type == ECCType::ReedSolomon)
        sb.rs_correctable_bytes = options.rs_correctable_bytes;

    // Write superblock to disk
    _superBlockManager = std::make_unique<SuperBlockManager>(_disk);
    auto put_res = _superBlockManager->put(sb);
    if (!put_res.has_value()) {
        return std::unexpected(put_res.error());
    }
    _superBlock = sb;

    // Create block device with appropriate ECC
    switch (options.ecc_type) {
    case ECCType::None: {
        _blockDevice = std::make_unique<RawBlockDevice>(options.block_size, _disk);
        break;
    }
    case ECCType::Parity: {
        _blockDevice = std::make_unique<ParityBlockDevice>(options.block_size, _disk);
        break;
    }
    case ECCType::Crc: {
        _blockDevice
            = std::make_unique<CrcBlockDevice>(options.crc_polynomial, _disk, options.block_size);
        break;
    }
    case ECCType::Hamming: {
        _blockDevice = std::make_unique<HammingBlockDevice>(options.block_size, _disk);
        break;
    }
    case ECCType::ReedSolomon: {
        _blockDevice = std::make_unique<ReedSolomonBlockDevice>(
            _disk, options.block_size, options.rs_correctable_bytes);
        break;
    }
    default:
        return std::unexpected(FsError::InvalidRequest);
    }

    // Create and format inode manager
    _inodeManager = std::make_unique<InodeManager>(*_blockDevice, _superBlock);
    auto format_inode_res = _inodeManager->format();
    if (!format_inode_res.has_value()) {
        return std::unexpected(format_inode_res.error());
    }

    // Create and format block manager
    auto first = sb.first_data_blocks_address;
    auto last = sb.last_data_block_address;
    _blockManager = std::make_unique<BlockManager>(first, last - first + 1, *_blockDevice);
    auto format_block_res = _blockManager->format();
    if (!format_block_res.has_value()) {
        return std::unexpected(format_block_res.error());
    }

    // Create file IO
    _fileIO = std::make_unique<FileIO>(*_blockDevice, *_blockManager, *_inodeManager);

    // Create directory manager
    _directoryManager = std::make_unique<DirectoryManager>(*_blockDevice, *_inodeManager, *_fileIO);

    return {};
}

std::expected<void, FsError> PpFS::create(std::string_view path)
{
    if (!isInitialized()) {
        return std::unexpected(FsError::NotInitialized);
    }

    // Validate path
    if (path.empty() || path.front() != '/') {
        return std::unexpected(FsError::InvalidPath);
    }
    // Check if path does not contain double slashes
    for (size_t i = 1; i < path.size(); ++i) {
        if (path[i] == '/' && path[i - 1] == '/') {
            return std::unexpected(FsError::InvalidPath);
        }
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
    auto entries_res = _directoryManager->getEntries(parent_inode);
    if (!entries_res.has_value()) {
        return std::unexpected(entries_res.error());
    }
    const auto& entries = entries_res.value();
    for (const auto& entry : entries) {
        if (filename == entry.name.data()) {
            return std::unexpected(FsError::NameTaken);
        }
    }

    // Create new inode
    Inode new_inode { .type = FileType::File };
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
            return std::unexpected(FsError::NotFound);
        }
        path = path.substr(next_slash);
    }
}

std::expected<inode_index_t, FsError> PpFS::open(std::string_view path)
{
    return std::unexpected(FsError::NotImplemented);
}

std::expected<void, FsError> PpFS::close(inode_index_t inode)
{
    return std::unexpected(FsError::NotImplemented);
}

std::expected<void, FsError> PpFS::remove(std::string_view path)
{
    return std::unexpected(FsError::NotImplemented);
}

std::expected<std::vector<std::uint8_t>, FsError> PpFS::read(
    inode_index_t inode, size_t offset, size_t size)
{
    return std::unexpected(FsError::NotImplemented);
}

std::expected<void, FsError> PpFS::write(
    inode_index_t inode, std::vector<std::uint8_t> buffer, size_t offset)
{
    return std::unexpected(FsError::NotImplemented);
}

std::expected<void, FsError> PpFS::createDirectory(std::string_view path)
{
    return std::unexpected(FsError::NotImplemented);
}

std::expected<std::vector<std::string_view>, FsError> PpFS::readDirectory(std::string_view path)
{
    return std::unexpected(FsError::NotImplemented);
}
