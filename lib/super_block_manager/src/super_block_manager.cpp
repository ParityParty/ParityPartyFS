#include "super_block_manager/super_block_manager.hpp"
#include <cmath>
#include <cstring>

SuperBlockEntry::SuperBlockEntry(block_index_t block_index)
    : block_index(block_index)
{
}

SuperBlockManager::SuperBlockManager(IDisk& disk)
    : _disk(disk)
    , _superBlock({})
{
    _endByte = 2 * sizeof(SuperBlock);
    _startByte = _disk.size() - sizeof(SuperBlock);
}

std::expected<SuperBlock, FsError> SuperBlockManager::get()
{
    if (_superBlock.has_value()) {
        return _superBlock.value();
    }

    auto read_res = _readFromDisk();
    if (!read_res.has_value()) {
        return std::unexpected(read_res.error());
    }

    _superBlock = read_res.value();

    return _superBlock.value();
}

std::expected<void, FsError> SuperBlockManager::put(SuperBlock new_super_block)
{
    _superBlock = new_super_block;

    auto write_res = _writeToDisk();

    if (!write_res.has_value())
        return std::unexpected(write_res.error());

    return {};
}

std::expected<void, FsError> SuperBlockManager::_writeToDisk()
{
    if (!_superBlock.has_value())
        return std::unexpected(FsError::InvalidRequest);

    // write at the beggining

    std::vector<std::byte> buffer(2 * sizeof(SuperBlock));
    std::memcpy(buffer.data(), &_superBlock.value(), sizeof(SuperBlock));
    std::memcpy(buffer.data() + sizeof(SuperBlock), &_superBlock.value(), sizeof(SuperBlock));

    auto write_res = _disk.write(0, buffer);
    if (!write_res.has_value())
        return std::unexpected(write_res.error());

    // write at the end

    write_res = _disk.write(_startByte, { buffer.begin(), buffer.begin() + sizeof(SuperBlock) });
    if (!write_res.has_value())
        return std::unexpected(write_res.error());

    return {};
}

std::expected<SuperBlock, FsError> SuperBlockManager::_readFromDisk()
{
    size_t read = 0;
    std::vector<std::byte> buffer;
    buffer.reserve(sizeof(SuperBlock) * 3);

    // read from the beginninng
    auto read_res = _disk.read(0, 2 * sizeof(SuperBlock));
    if (!read_res.has_value())
        return std::unexpected(read_res.error());
    buffer.insert(buffer.end(), read_res.value().begin(), read_res.value().end());

    // read from the end

    read_res = _disk.read(_startByte, sizeof(SuperBlock));
    if (!read_res.has_value())
        return std::unexpected(read_res.error());
    buffer.insert(buffer.end(), read_res.value().begin(), read_res.value().end());

    SuperBlock sb1;
    SuperBlock sb2;
    SuperBlock sb3;
    std::memcpy(&sb1, buffer.data(), sizeof(SuperBlock));
    std::memcpy(&sb2, buffer.data() + sizeof(SuperBlock), sizeof(SuperBlock));
    std::memcpy(&sb3, buffer.data() + 2 * sizeof(SuperBlock), sizeof(SuperBlock));
    // TODO: glosowanie

    return sb3;
}
