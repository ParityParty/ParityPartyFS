#include "file_io/file_io.hpp"
#include <cstring>

FileIO::FileIO(
    IBlockDevice& block_device, IBlockManager& block_manager, IInodeManager& inode_manager)
    : _block_device(block_device)
    , _block_manager(block_manager)
    , _inode_manager(inode_manager)
{
}

std::expected<std::vector<uint8_t>, FsError> FileIO::readFile(
    Inode& inode, size_t offset, size_t bytes_to_read)
{
    if (offset + bytes_to_read > inode.file_size)
        return std::unexpected(FsError::OutOfBounds);

    size_t block_number = offset / _block_device.dataSize();

    BlockIndexIterator indexIterator(block_number, inode, _block_device, _block_manager, false);
    std::vector<std::uint8_t> data;
    data.reserve(bytes_to_read);

    while (true) {
        auto next_block = indexIterator.next();
        if (!next_block.has_value())
            return std::unexpected(next_block.error());
        auto read_res = _block_device.readBlock(DataLocation(*next_block, offset), bytes_to_read);
        if (!read_res.has_value())
            return std::unexpected(read_res.error());
        data.insert(data.end(), read_res.value().begin(), read_res.value().end());

        offset = 0;
        bytes_to_read -= read_res.value().size();

        if (bytes_to_read == 0)
            return data;
    }
}

std::expected<size_t, FsError> FileIO::writeFile(
    Inode& inode, size_t offset, std::vector<uint8_t> bytes_to_write)
{
    size_t written_bytes = 0;
    size_t block_number = offset / _block_device.dataSize();

    BlockIndexIterator indexIterator(block_number, inode, _block_device, _block_manager, true);

    while (true) {
        auto next_block = indexIterator.next();
        if (!next_block.has_value()) {
            // We wrote some bytes already, so we need to update file size
            if (inode.file_size < offset + written_bytes) {
                inode.file_size = offset + written_bytes;
                auto inode_res = _inode_manager.update(inode);
                if (!inode_res.has_value()) {
                    return std::unexpected(inode_res.error());
                }
            }
            return std::unexpected(next_block.error());
        }

        auto write_res = _block_device.writeBlock(
            { bytes_to_write.begin() + written_bytes, bytes_to_write.end() },
            DataLocation(*next_block, offset));
        if (!write_res.has_value()) {
            // If we failed to write to a new block, we should free it
            if (inode.file_size <= offset + written_bytes)
                _block_manager.free(*next_block);

            // We wrote some bytes already, so we need to update file size
            if (inode.file_size < offset + written_bytes) {
                inode.file_size = offset + written_bytes;
                _inode_manager.update(inode);
            }
            return std::unexpected(write_res.error());
        }

        offset = 0;
        written_bytes += write_res.value();

        if (written_bytes == bytes_to_write.size())
            if (inode.file_size < offset + written_bytes) {
                inode.file_size = offset + written_bytes;
                auto inode_res = _inode_manager.update(inode);
                if (!inode_res.has_value()) {
                    return std::unexpected(inode_res.error());
                }
                return written_bytes;
            }
    }

    return std::unexpected(FsError::InternalError);
}

BlockIndexIterator::BlockIndexIterator(size_t index, Inode& inode, IBlockDevice& block_device,
    IBlockManager& block_manager, bool should_resize)
    : _index(index)
    , _block_device(block_device)
    , _block_manager(block_manager)
    , _inode(inode)
    , _should_resize(should_resize)
{
    _occupied_blocks = (inode.file_size + block_device.dataSize() - 1) / _block_device.dataSize();
}

std::expected<block_index_t, FsError> BlockIndexIterator::next()
{
    if (!_should_resize && _index >= _occupied_blocks)
        _finished = true;

    if (_finished)
        return std::unexpected(FsError::FileSizeExceeded);

    // direct blocks
    if (_index < 12) {
        if (_index >= _occupied_blocks) {
            auto index_res = _findAndReserveBlock();
            if (!index_res.has_value()) {
                return std::unexpected(index_res.error());
            }
            _inode.direct_blocks[_index] = index_res.value();
        }
        return _inode.direct_blocks[_index++];
    }

    size_t indexes_per_block = _block_device.dataSize() / sizeof(block_index_t);

    // indirect blocks
    auto index_in_segment = _index - 12;

    // first block in indirect segment
    if (index_in_segment == 0) {
        if (_index < _occupied_blocks) {
            // if indirect block already exists, read it
            auto read_res = _readIndexBlock(_inode.indirect_block);
            if (!read_res.has_value()) {
                return std::unexpected(read_res.error());
            }
            _index_block_1 = read_res.value();
        } else {
            // else, allocate new indirect block
            auto index_res = _findAndReserveBlock();
            if (!index_res.has_value()) {
                return std::unexpected(index_res.error());
            }
            _inode.indirect_block = index_res.value();
            _block_device.formatBlock(index_res.value());
            _index_block_1 = std::vector<block_index_t>(indexes_per_block);
        }
    }

    if (index_in_segment < indexes_per_block) {
        if (_index >= _occupied_blocks) {
            auto index_res = _findAndReserveBlock();
            if (!index_res.has_value()) {
                return std::unexpected(index_res.error());
            }
            _index_block_1[index_in_segment] = index_res.value();
            auto write_res = _writeIndexBlock(_inode.indirect_block, _index_block_1);
            if (!write_res.has_value()) {
                _block_manager.free(index_res.value());
                return std::unexpected(write_res.error());
            }
        }
        _index++;
        return _index_block_1[index_in_segment];
    }

    // doubly
    index_in_segment -= indexes_per_block;

    if (index_in_segment == 0) {
        if (_index < _occupied_blocks) {
            auto read_res = _readIndexBlock(_inode.doubly_indirect_block);
            if (!read_res.has_value()) {
                return std::unexpected(read_res.error());
            }
            _index_block_1 = read_res.value();
        }

        else {
            auto index_res = _findAndReserveBlock();
            if (!index_res.has_value()) {
                return std::unexpected(index_res.error());
            }
            _inode.doubly_indirect_block = index_res.value();
            _block_device.formatBlock(index_res.value());
            _index_block_1 = std::vector<block_index_t>(indexes_per_block);
        }
    }

    if (index_in_segment < indexes_per_block * indexes_per_block) {
        // next entry in doubly indirect
        if (index_in_segment % indexes_per_block == 0) {
            if (_index < _occupied_blocks) {
                auto index = (_index - 12 - indexes_per_block) / indexes_per_block;
                auto read_res = _readIndexBlock(_index_block_1[index]);
                if (!read_res.has_value()) {
                    return std::unexpected(read_res.error());
                }
                _index_block_2 = read_res.value();
            } else {
                auto index_res = _findAndReserveBlock();
                if (!index_res.has_value()) {
                    return std::unexpected(index_res.error());
                }
                _index_block_1[index_in_segment / indexes_per_block] = index_res.value();
                auto write_res = _writeIndexBlock(_inode.doubly_indirect_block, _index_block_1);
                if (!write_res.has_value()) {
                    _block_manager.free(index_res.value());
                    return std::unexpected(write_res.error());
                }
                _index_block_2 = std::vector<block_index_t>(indexes_per_block);
            }
        }
        if (_index >= _occupied_blocks) {
            auto index_res = _findAndReserveBlock();
            if (!index_res.has_value()) {
                return std::unexpected(index_res.error());
            }
            _index_block_2[index_in_segment % indexes_per_block] = index_res.value();
            auto write_res = _writeIndexBlock(
                _index_block_1[index_in_segment / indexes_per_block], _index_block_2);
            if (!write_res.has_value()) {
                _block_manager.free(index_res.value());
                return std::unexpected(write_res.error());
            }
        }
        return _index_block_2[(_index++ - 12 - indexes_per_block) % indexes_per_block];
    }
    // trebly
    index_in_segment -= indexes_per_block * indexes_per_block;
    if (index_in_segment == 0) {
        if (_index < _occupied_blocks) {
            auto read_res = _readIndexBlock(_inode.trebly_indirect_block);
            if (!read_res.has_value()) {
                return std::unexpected(read_res.error());
            }
            _index_block_1 = read_res.value();
        }

        else {
            auto index_res = _findAndReserveBlock();
            if (!index_res.has_value()) {
                return std::unexpected(index_res.error());
            }
            _inode.trebly_indirect_block = index_res.value();
            _block_device.formatBlock(index_res.value());
            _index_block_1 = std::vector<block_index_t>(indexes_per_block);
        }
    }

    if (index_in_segment < indexes_per_block * indexes_per_block * indexes_per_block) {
        if (index_in_segment % (indexes_per_block * indexes_per_block) == 0) {
            if (_index < _occupied_blocks) {
                auto read_res = _readIndexBlock(
                    _index_block_1[index_in_segment / (indexes_per_block * indexes_per_block)]);
                if (!read_res.has_value()) {
                    return std::unexpected(read_res.error());
                }
                _index_block_2 = read_res.value();
            } else {
                auto index_res = _findAndReserveBlock();
                if (!index_res.has_value()) {
                    return std::unexpected(index_res.error());
                }
                _index_block_2 = std::vector<block_index_t>(indexes_per_block);
                _index_block_1[index_in_segment / (indexes_per_block * indexes_per_block)]
                    = index_res.value();
                auto write_res = _writeIndexBlock(_inode.trebly_indirect_block, _index_block_1);
                if (!write_res.has_value()) {
                    _block_manager.free(index_res.value());
                    return std::unexpected(write_res.error());
                }
            }
        }

        if (index_in_segment % indexes_per_block == 0) {
            if (_index < _occupied_blocks) {
                auto read_res = _readIndexBlock(
                    _index_block_2[(index_in_segment / indexes_per_block) % indexes_per_block]);
                if (!read_res.has_value()) {
                    return std::unexpected(read_res.error());
                }
                _index_block_3 = read_res.value();
            } else {
                auto index_res = _findAndReserveBlock();
                if (!index_res.has_value()) {
                    return std::unexpected(index_res.error());
                }
                _index_block_3 = std::vector<block_index_t>(indexes_per_block);
                _index_block_2[(index_in_segment / indexes_per_block) % indexes_per_block]
                    = index_res.value();
                auto write_res = _writeIndexBlock(
                    _index_block_1[index_in_segment / (indexes_per_block * indexes_per_block)],
                    _index_block_2);
                if (!write_res.has_value()) {
                    _block_manager.free(index_res.value());
                    return std::unexpected(write_res.error());
                }
            }
        }

        if (_index >= _occupied_blocks) {
            auto index_res = _findAndReserveBlock();
            if (!index_res.has_value()) {
                return std::unexpected(index_res.error());
            }
            _index_block_3[index_in_segment % indexes_per_block] = index_res.value();
            auto write_res = _writeIndexBlock(
                _index_block_2[(index_in_segment / indexes_per_block) % indexes_per_block],
                _index_block_3);
            if (!write_res.has_value()) {
                _block_manager.free(index_res.value());
                return std::unexpected(write_res.error());
            }
        }
        _index++;
        return _index_block_3[index_in_segment % indexes_per_block];
    }

    _finished = true;
    return std::unexpected(FsError::FileSizeExceeded);
}

std::expected<std::vector<block_index_t>, FsError> BlockIndexIterator::_readIndexBlock(
    block_index_t index)
{
    size_t indexes_per_block = _block_device.dataSize() / sizeof(block_index_t);

    auto raw = _block_device.readBlock(
        DataLocation(index, 0), indexes_per_block * sizeof(block_index_t));

    if (!raw.has_value())
        return std::unexpected(raw.error());

    const auto& bytes = raw.value();

    std::vector<block_index_t> out;
    out.reserve(indexes_per_block);

    const block_index_t* ptr = reinterpret_cast<const block_index_t*>(bytes.data());

    size_t count = bytes.size() / sizeof(block_index_t);

    for (size_t i = 0; i < count; ++i)
        out.push_back(ptr[i]);

    return out;
}

std::expected<void, FsError> BlockIndexIterator::_writeIndexBlock(
    block_index_t index, const std::vector<block_index_t>& indices)
{
    size_t indexes_per_block = _block_device.dataSize() / sizeof(block_index_t);

    if (indices.size() > indexes_per_block)
        return std::unexpected(FsError::InvalidRequest);

    std::vector<std::uint8_t> bytes;
    bytes.resize(indexes_per_block * sizeof(block_index_t));

    std::memset(bytes.data(), 0, bytes.size());

    std::memcpy(bytes.data(), indices.data(), indices.size() * sizeof(block_index_t));

    auto res = _block_device.writeBlock(bytes, DataLocation(index, 0));

    if (!res.has_value())
        return std::unexpected(res.error());

    return {};
}

std::expected<block_index_t, FsError> BlockIndexIterator::_findAndReserveBlock()
{
    auto index_res = _block_manager.getFree();
    if (!index_res.has_value()) {
        return index_res;
    }
    auto reserve_res = _block_manager.reserve(index_res.value());
    if (!reserve_res.has_value())
        return std::unexpected(reserve_res.error());
    return index_res;
}
