#include "super_block_manager/super_block_manager.hpp"
#include <cmath>
#include <cstring>

SuperBlockEntry::SuperBlockEntry(block_index_t block_index)
    : block_index(block_index)
{
}

SuperBlockManager::SuperBlockManager(
    IBlockDevice& block_device, std::vector<SuperBlockEntry> entries)
    : _block_device(block_device)
    , _entries(entries)
    , _rawSuperBlock({})
{
}

std::optional<SuperBlockManager> SuperBlockManager::CreateInstance(
    IBlockDevice& block_device, std::vector<SuperBlockEntry> entries)
{
    int span = std::ceil(sizeof(RawSuperBlock) / float(block_device.dataSize()));

    for (int i = 0; i < entries.size(); ++i) {
        int a_start = entries[i].block_index;
        int a_end = a_start + span - 1;

        for (int j = i + 1; j < entries.size(); ++j) {
            int b_start = entries[j].block_index;
            int b_end = b_start + span - 1;

            bool overlap = !(a_end < b_start || b_end < a_start);
            if (overlap)
                return {};
        }
    }

    return SuperBlockManager(block_device, entries);
}

std::expected<SuperBlock, DiskError> SuperBlockManager::get()
{
    if (_rawSuperBlock.has_value()) {
        return _rawSuperBlock->super_block;
    }

    // Vector of bools to mark block indexes that need updates, so that we do not run multiple
    // updates
    auto needs_update = std::vector<bool>(_entries.size(), false);

    for (int i = 0; i < _entries.size(); i++) {
        auto read_res = _readFromDisk(_entries[i].block_index);
        if (!read_res.has_value())
            needs_update[i] = true;

        // if read from previous blocks was unsuccesful or the version was older, try to update them
        if (!_rawSuperBlock.has_value() || _rawSuperBlock->version < read_res.value().version) {
            _rawSuperBlock = read_res.value();
            for (int j = 0; j < i; j++) {
                needs_update[j] = true;
            }
        }
    }

    if (!_rawSuperBlock.has_value())
        return std::unexpected(DiskError::InternalError);
    // TODO - when we make errors more descriptive, change returned value

    for (int i = 0; i < _entries.size(); i++) {
        if (needs_update[i])
            _writeToDisk(_entries[i].block_index);
    }

    return _rawSuperBlock->super_block;
}

std::expected<void, DiskError> SuperBlockManager::put(SuperBlock new_super_block)
{
    _rawSuperBlock = RawSuperBlock {
        .super_block = new_super_block,
        .version = 0,
    };
    _rawSuperBlock->super_block = new_super_block;
    _rawSuperBlock->version++;

    bool any_success = false;
    for (auto& entry : _entries) {
        auto write_res = _writeToDisk(entry.block_index);
        if (write_res.has_value())
            any_success = true;
    }

    return any_success ? std::expected<void, DiskError>()
                       : std::unexpected(DiskError::InternalError);
}

std::expected<void, DiskError> SuperBlockManager::update(SuperBlock new_super_block)
{
    if (!_rawSuperBlock.has_value()) {
        get();
        if (!_rawSuperBlock.has_value())
            return std::unexpected(DiskError::InternalError); // We can't update supeblock, if we
                                                              // don't know what the value is
    }

    _rawSuperBlock->super_block = new_super_block;
    _rawSuperBlock->version++;

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
    if (!_rawSuperBlock.has_value())
        return std::unexpected(DiskError::InvalidRequest);

    std::vector<std::byte> buffer(sizeof(RawSuperBlock));
    std::memcpy(buffer.data(), &_rawSuperBlock.value(), sizeof(RawSuperBlock));

    size_t written = 0;
    while (written != sizeof(RawSuperBlock)) {
        auto format_res = _block_device.formatBlock(block_index);
        if (!format_res.has_value())
            return std::unexpected(format_res.error());

        auto write_res = _block_device.writeBlock(
            { buffer.begin() + written, buffer.end() }, DataLocation(block_index++, 0));
        if (!write_res.has_value())
            return std::unexpected(write_res.error());

        written += write_res.value();
    }

    return {};
}

std::expected<RawSuperBlock, DiskError> SuperBlockManager::_readFromDisk(block_index_t block_index)
{
    size_t read = 0;
    std::vector<std::byte> buffer;
    buffer.reserve(sizeof(RawSuperBlock));

    while (read != sizeof(RawSuperBlock)) {
        auto read_res
            = _block_device.readBlock(DataLocation(block_index++, 0), sizeof(RawSuperBlock) - read);
        if (!read_res.has_value())
            return std::unexpected(read_res.error());

        read += read_res.value().size();
        buffer.insert(buffer.end(), read_res.value().begin(), read_res.value().end());
    }

    RawSuperBlock rsb;
    std::memcpy(&rsb, buffer.data(), sizeof(RawSuperBlock));
    return rsb;
}
