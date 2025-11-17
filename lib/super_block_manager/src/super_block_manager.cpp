#include "super_block_manager/super_block_manager.hpp"
#include <cmath>
#include <cstring>

SuperBlockEntry::SuperBlockEntry(block_index_t block_index)
    : block_index(block_index)
{
}

SuperBlockManager::SuperBlockManager(IBlockDevice& block_device)
    : _block_device(block_device)
    , _superBlock({})
{
    _endBlock = (2 * sizeof(SuperBlock) + _block_device.dataSize() - 1) / _block_device.dataSize();
    _startBlock = _block_device.numOfBlocks()
        - (sizeof(SuperBlock) + _block_device.dataSize() - 1) / _block_device.dataSize();
}

std::expected<SuperBlock, DiskError> SuperBlockManager::get()
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

std::expected<void, DiskError> SuperBlockManager::put(SuperBlock new_super_block)
{
    _superBlock = new_super_block;

    auto write_res = _writeToDisk();

    if (!write_res.has_value())
        return std::unexpected(write_res.error());

    return {};
}

std::expected<void, DiskError> SuperBlockManager::_writeToDisk()
{
    if (!_superBlock.has_value())
        return std::unexpected(DiskError::InvalidRequest);

    // write at the beggining

    std::vector<std::byte> buffer(2 * sizeof(SuperBlock));
    std::memcpy(buffer.data(), &_superBlock.value(), sizeof(SuperBlock));
    std::memcpy(buffer.data() + sizeof(SuperBlock), &_superBlock.value(), sizeof(SuperBlock));

    size_t written = 0;
    block_index_t index = 0;
    while (written != 2 * sizeof(SuperBlock)) {
        auto format_res = _block_device.formatBlock(index);
        if (!format_res.has_value())
            return std::unexpected(format_res.error());

        auto write_res = _block_device.writeBlock(
            { buffer.begin() + written, buffer.end() }, DataLocation(index++, 0));
        if (!write_res.has_value())
            return std::unexpected(write_res.error());

        written += write_res.value();
    }

    // write at the end

    written = 0;
    index = _startBlock; // todo
    while (written != sizeof(SuperBlock)) {
        auto format_res = _block_device.formatBlock(index);
        if (!format_res.has_value())
            return std::unexpected(format_res.error());

        auto write_res = _block_device.writeBlock(
            { buffer.begin() + written, buffer.begin() + sizeof(SuperBlock) },
            DataLocation(index++, 0));
        if (!write_res.has_value())
            return std::unexpected(write_res.error());

        written += write_res.value();
    }

    return {};
}

std::expected<SuperBlock, DiskError> SuperBlockManager::_readFromDisk()
{
    size_t read = 0;
    std::vector<std::byte> buffer;
    buffer.reserve(sizeof(SuperBlock) * 3);

    // read from the beginninng
    block_index_t block_index = 0;
    while (read != sizeof(SuperBlock) * 2) {
        auto read_res = _block_device.readBlock(
            DataLocation(block_index++, 0), sizeof(SuperBlock) * 2 - read);
        if (!read_res.has_value())
            return std::unexpected(read_res.error());

        read += read_res.value().size();
        buffer.insert(buffer.end(), read_res.value().begin(), read_res.value().end());
    }

    // read from the end

    read = 0;
    block_index = _startBlock;
    while (read != sizeof(SuperBlock)) {
        auto read_res
            = _block_device.readBlock(DataLocation(block_index++, 0), sizeof(SuperBlock) - read);
        if (!read_res.has_value())
            return std::unexpected(read_res.error());

        read += read_res.value().size();
        buffer.insert(buffer.end(), read_res.value().begin(), read_res.value().end());
    }

    SuperBlock sb1;
    SuperBlock sb2;
    SuperBlock sb3;
    std::memcpy(&sb1, buffer.data(), sizeof(SuperBlock));
    std::memcpy(&sb2, buffer.data() + sizeof(SuperBlock), sizeof(SuperBlock));
    std::memcpy(&sb3, buffer.data() + 2 * sizeof(SuperBlock), sizeof(SuperBlock));
    // TODO: glosowanie

    return sb3;
}
