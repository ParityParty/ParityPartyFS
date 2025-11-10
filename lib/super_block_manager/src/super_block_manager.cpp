#include "super_block_manager/super_block_manager.hpp"

SuperBlockEntry::SuperBlockEntry(block_index_t block_index) : block_index(block_index){

}

SuperBlockManager::SuperBlockManager(IBlockDevice& block_device)
    : _block_device(block_device), 
      _entries({ SuperBlockEntry(SUPERBLOCK_INDX1), SuperBlockEntry(SUPERBLOCK_INDX2) }),
      _superBlock({})
{
}

std::expected<SuperBlock, DiskError> SuperBlockManager::get() {
    if (_superBlock.has_value()) {
        return *_superBlock;
    }

    for (int i = 0; i < _entries.size(); i++){
        auto read_res = _block_device.readBlock(DataLocation(_entries[i].block_index, 0), sizeof(SuperBlock));
        if (!read_res.has_value())
            continue;
        _superBlock = *reinterpret_cast<SuperBlock*>(&read_res.value());

        for(int j = 0; j < i; j++){
            _writeToDisk(j);
        }
    }
    return std::unexpected(DiskError::InternalError);
    // TODO - when we make errors more descriptive, change returned value
}

std::expected<void, DiskError> SuperBlockManager::update(SuperBlock new_super_block) {
    _superBlock = new_super_block;

    bool any_success = false;
    for (auto &entry : _entries) {
        auto write_res = _writeToDisk(entry.block_index);
        if (write_res.has_value())
            any_success = false;
    }

    return any_success ? std::expected<void, DiskError>() : std::unexpected(DiskError::InternalError);
}

std::expected<void, DiskError> SuperBlockManager::_writeToDisk(int block_index) {
    if (!_superBlock.has_value())
        return std::unexpected(DiskError::InvalidRequest);

    const SuperBlock& sb = _superBlock.value();
    const std::byte* data_ptr = reinterpret_cast<const std::byte*>(&sb);
    size_t data_size = sizeof(SuperBlock);

    std::vector<std::byte> buffer(data_ptr, data_ptr + data_size);

    auto write_res = _block_device.writeBlock(buffer, DataLocation(block_index, 0));
    if (!write_res.has_value())
        return std::unexpected(write_res.error());

    return {};
}
