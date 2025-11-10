#include "super_block_manager/super_block_manager.hpp"
#include <cstring>

SuperBlockEntry::SuperBlockEntry(block_index_t block_index)
    : block_index(block_index)
{
}

SuperBlockManager::SuperBlockManager(IBlockDevice& block_device)
    : _block_device(block_device)
    , _entries({ SuperBlockEntry(SUPERBLOCK_INDX1), SuperBlockEntry(SUPERBLOCK_INDX2) })
    , _superBlock({})
{
}

std::expected<SuperBlock, DiskError> SuperBlockManager::get()
{
    if (_superBlock.has_value()) {
        return *_superBlock;
    }

    for (int i = 0; i < _entries.size(); i++) {
        auto read_res = _readFromDisk(_entries[i].block_index);
        if (!read_res.has_value())
            continue;

        *_superBlock = read_res.value();

        for (int j = 0; j < i; j++) {
            _writeToDisk(j);
        }
        return *_superBlock;
    }
    return std::unexpected(DiskError::InternalError);
    // TODO - when we make errors more descriptive, change returned value
}

std::expected<void, DiskError> SuperBlockManager::update(SuperBlock new_super_block)
{
    _superBlock = new_super_block;

    bool any_success = false;
    for (auto& entry : _entries) {
        auto write_res = _writeToDisk(entry.block_index);
        if (write_res.has_value())
            any_success = true;
    }

    return any_success ? std::expected<void, DiskError>()
                       : std::unexpected(DiskError::InternalError);
}

std::expected<void, DiskError> SuperBlockManager::_writeToDisk(int block_index)
{
    if (!_superBlock.has_value())
        return std::unexpected(DiskError::InvalidRequest);

    std::vector<std::byte> buffer(sizeof(SuperBlock));
    std::memcpy(buffer.data(), &_superBlock.value(), sizeof(SuperBlock));

    size_t written = 0;
    while (written != sizeof(SuperBlock)) {
        auto write_res = _block_device.writeBlock(
            { buffer.begin() + written, buffer.end() }, DataLocation(block_index, 0));
        if (!write_res.has_value())
            return std::unexpected(write_res.error());
        written += write_res.value();
    }

    return {};
}

std::expected<SuperBlock, DiskError> SuperBlockManager::_readFromDisk(block_index_t block_index)
{
    size_t read = 0;
    std::vector<std::byte> buffer;
    buffer.reserve(sizeof(SuperBlock));

    while (read != sizeof(SuperBlock)) {
        auto read_res = _block_device.readBlock(DataLocation(block_index++, 0), sizeof(SuperBlock));
        if (!read_res.has_value())
            return std::unexpected(read_res.error());
        read += read_res.value().size();
        buffer.insert(buffer.end(), read_res.value().begin(), read_res.value().end());
    }

    SuperBlock sb;
    std::memcpy(&sb, buffer.data(), sizeof(SuperBlock));
    return sb;
}
