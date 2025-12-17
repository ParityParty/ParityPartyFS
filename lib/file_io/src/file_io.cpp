#include "file_io/file_io.hpp"
#include <cstring>

FileIO::FileIO(
    IBlockDevice& block_device, IBlockManager& block_manager, IInodeManager& inode_manager)
    : _block_device(block_device)
    , _block_manager(block_manager)
    , _inode_manager(inode_manager)
{
}

std::expected<void, FsError> FileIO::readFile(inode_index_t inode_index, Inode& inode,
    size_t offset, size_t bytes_to_read, static_vector<uint8_t>& data)
{
    if (offset + bytes_to_read > inode.file_size) {
        if (offset >= inode.file_size)
            return std::unexpected(FsError::FileIO_OutOfBounds);
        bytes_to_read = inode.file_size - offset;
    }
    if (data.capacity() < bytes_to_read)
        return std::unexpected(FsError::FileIO_InvalidRequest);
    data.resize(0);

    size_t block_number = offset / _block_device.dataSize();
    size_t offset_in_block = offset % _block_device.dataSize();

    BlockIndexIterator indexIterator(block_number, inode, _block_device, _block_manager, false);

    while (bytes_to_read) {
        auto next_block = indexIterator.next();
        if (!next_block.has_value())
            return std::unexpected(next_block.error());
        static_vector<uint8_t> buf(data.end(), bytes_to_read, bytes_to_read);
        auto read_res = _block_device.readBlock(
            DataLocation(*next_block, offset_in_block), bytes_to_read, buf);
        if (!read_res.has_value())
            return std::unexpected(read_res.error());
        data.resize(data.size() + buf.size());

        offset_in_block = 0;
        bytes_to_read -= buf.size();
    }
    return {};
}

std::expected<size_t, FsError> FileIO::writeFile(inode_index_t inode_index, Inode& inode,
    size_t offset, const static_vector<uint8_t>& bytes_to_write)
{
    size_t written_bytes = 0;
    size_t block_number = offset / _block_device.dataSize();
    size_t offset_in_block = offset % _block_device.dataSize();

    BlockIndexIterator indexIterator(block_number, inode, _block_device, _block_manager, true);
    while (true) {
        auto next_block = indexIterator.next();
        if (!next_block.has_value()) {
            // We wrote some bytes already, so we need to update file size
            if (inode.file_size < offset + written_bytes) {
                inode.file_size = offset + written_bytes;
                auto inode_res = _inode_manager.update(inode_index, inode);
                if (!inode_res.has_value()) {
                    return std::unexpected(inode_res.error());
                }
            }
            return std::unexpected(next_block.error());
        }
        static_vector<std::uint8_t> buf(const_cast<uint8_t*>(bytes_to_write.data()) + written_bytes, bytes_to_write.size() - written_bytes, bytes_to_write.size() - written_bytes);
        auto write_res
            = _block_device.writeBlock(buf, DataLocation(*next_block, offset_in_block));
        if (!write_res.has_value()) {
            // If we failed to write to a new block, we should free it
            if (inode.file_size <= offset + written_bytes)
                _block_manager.free(*next_block);

            // We wrote some bytes already, so we need to update file size
            if (inode.file_size < offset + written_bytes) {
                inode.file_size = offset + written_bytes;
                _inode_manager.update(inode_index, inode);
            }

            return std::unexpected(write_res.error());
        }

        offset_in_block = 0;
        written_bytes += write_res.value();

        if (written_bytes != bytes_to_write.size())
            continue;

        if (inode.file_size >= offset + written_bytes) {
            return written_bytes;
        }

        inode.file_size = offset + written_bytes;
        auto inode_res = _inode_manager.update(inode_index, inode);
        if (!inode_res.has_value()) {
            return std::unexpected(inode_res.error());
        };

        return written_bytes;
    }

    return std::unexpected(FsError::FileIO_InternalError);
}

std::expected<void, FsError> FileIO::resizeFile(
    inode_index_t inode_index, Inode& inode, size_t new_size)
{
    if (new_size == inode.file_size)
        return {};

    if (new_size > inode.file_size) {
        BlockIndexIterator indexIterator(
            (inode.file_size + _block_device.dataSize() - 1) / _block_device.dataSize(), inode,
            _block_device, _block_manager, true);
        size_t bytes_to_allocate = new_size - inode.file_size;
        // Check if we can resize part of file without allocating new blocks
        auto remaining_in_block = inode.file_size % _block_device.dataSize();
        if (remaining_in_block != 0) {
            size_t free_in_block = _block_device.dataSize() - remaining_in_block;
            bytes_to_allocate
                = bytes_to_allocate > free_in_block ? bytes_to_allocate - free_in_block : 0;

            inode.file_size += std::min(free_in_block, new_size - (size_t)inode.file_size);
            auto inode_res = _inode_manager.update(inode_index, inode);
            if (!inode_res.has_value())
                return std::unexpected(inode_res.error());
        }
        while (bytes_to_allocate > 0) {
            auto next_block = indexIterator.next();
            if (!next_block.has_value()) {
                inode.file_size = new_size - bytes_to_allocate;
                auto inode_res = _inode_manager.update(inode_index, inode);
                if (!inode_res.has_value())
                    return std::unexpected(inode_res.error());
                return std::unexpected(next_block.error());
            }
            auto format_res = _block_device.formatBlock(*next_block);
            if (!format_res.has_value()) {
                inode.file_size = new_size - bytes_to_allocate;
                auto inode_res = _inode_manager.update(inode_index, inode);
                if (!inode_res.has_value())
                    return std::unexpected(inode_res.error());
                return std::unexpected(format_res.error());
            }
            bytes_to_allocate = bytes_to_allocate > _block_device.dataSize()
                ? bytes_to_allocate - _block_device.dataSize()
                : 0;
        }
        inode.file_size = new_size;
        auto inode_res = _inode_manager.update(inode_index, inode);
        if (!inode_res.has_value())
            return std::unexpected(inode_res.error());
        return {};
    }

    BlockIndexIterator indexIterator(
        (new_size + _block_device.dataSize() - 1) / _block_device.dataSize(), inode, _block_device,
        _block_manager, false);

    auto old_size = inode.file_size;
    inode.file_size = new_size;
    auto inode_res = _inode_manager.update(inode_index, inode);
    if (!inode_res.has_value()) {
        inode.file_size = old_size;
        return std::unexpected(inode_res.error());
    }

    size_t blocks_to_free = (old_size + _block_device.dataSize() - 1) / _block_device.dataSize()
        - (inode.file_size + _block_device.dataSize() - 1) / _block_device.dataSize();
    for (size_t i = 0; i < blocks_to_free; i++) {
        std::array<block_index_t, 3> indirect_blocks_added_buffer;
        static_vector<block_index_t> indirect_blocks_added(indirect_blocks_added_buffer.data(), 3);
        auto next_block = indexIterator.nextWithIndirectBlocksAdded(indirect_blocks_added);
        if (!next_block.has_value()) {
            return std::unexpected(next_block.error());
        }
        for (auto& index : indirect_blocks_added) {
            _block_manager.free(index);
        }
        _block_manager.free(next_block.value());
    }
    return {};
}

BlockIndexIterator::BlockIndexIterator(size_t index, Inode& inode, IBlockDevice& block_device,
    IBlockManager& block_manager, bool should_resize)
    : _index(index)
    , _block_device(block_device)
    , _block_manager(block_manager)
    , _inode(inode)
    , _index_block_1(_index_block_1_buffer.data(), MAX_BLOCK_SIZE / sizeof(block_index_t))
    , _index_block_2(_index_block_2_buffer.data(), MAX_BLOCK_SIZE / sizeof(block_index_t))
    , _index_block_3(_index_block_3_buffer.data(), MAX_BLOCK_SIZE / sizeof(block_index_t))
    , _should_resize(should_resize)
{
    _occupied_blocks = (inode.file_size + block_device.dataSize() - 1) / _block_device.dataSize();
}

std::expected<block_index_t, FsError>
BlockIndexIterator::nextWithIndirectBlocksAdded(static_vector<block_index_t>& indirect_blocks_added)
{
    if(indirect_blocks_added.capacity() < 3)
        return std::unexpected(FsError::FileIO_InvalidRequest);
    indirect_blocks_added.resize(0);

    if (!_should_resize && _index >= _occupied_blocks)
        _finished = true;

    if (_finished) {
        return std::unexpected(FsError::FileIO_OutOfBounds);
    }

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
    if (index_in_segment == 0 || (index_in_segment < indexes_per_block && _index_block_1.empty())) {
        if (index_in_segment != 0 || index_in_segment < _occupied_blocks) {
            // if indirect block already exists, read it
            auto read_res = _readIndexBlock(_inode.indirect_block, _index_block_1);
            if (!read_res.has_value()) {
                return std::unexpected(read_res.error());
            }
            if (index_in_segment == 0)
                indirect_blocks_added.push_back(_inode.indirect_block);
        } else {
            // else, allocate new indirect block
            auto index_res = _findAndReserveBlock();
            if (!index_res.has_value()) {
                return std::unexpected(index_res.error());
            }
            _inode.indirect_block = index_res.value();
            _block_device.formatBlock(index_res.value());
            _index_block_1.resize(indexes_per_block);
            if (index_in_segment == 0)
                indirect_blocks_added.push_back(_inode.indirect_block);
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

    if (index_in_segment == 0
        || (index_in_segment < indexes_per_block * indexes_per_block) && _index_block_1.empty()) {
        if (index_in_segment != 0 || _index < _occupied_blocks) {
            auto read_res = _readIndexBlock(_inode.doubly_indirect_block, _index_block_1);

            if (!read_res.has_value()) {
                return std::unexpected(read_res.error());
            }
            if (index_in_segment == 0)
                indirect_blocks_added.push_back(_inode.doubly_indirect_block);
        }

        else {
            auto index_res = _findAndReserveBlock();
            if (!index_res.has_value()) {
                return std::unexpected(index_res.error());
            }
            _inode.doubly_indirect_block = index_res.value();
            _block_device.formatBlock(index_res.value());
            _index_block_1.resize(indexes_per_block);
            if (index_in_segment == 0)
                indirect_blocks_added.push_back(_inode.doubly_indirect_block);
        }
    }

    if (index_in_segment < indexes_per_block * indexes_per_block) {
        // next entry in doubly indirect
        if (index_in_segment % indexes_per_block == 0 || _index_block_2.empty()) {
            if (index_in_segment % indexes_per_block != 0 || _index < _occupied_blocks) {
                auto index = (_index - 12 - indexes_per_block) / indexes_per_block;
                auto read_res = _readIndexBlock(_index_block_1[index], _index_block_2);
                if (!read_res.has_value()) {
                    return std::unexpected(read_res.error());
                }
                if (index_in_segment % indexes_per_block == 0)
                    indirect_blocks_added.push_back(_index_block_1[index]);
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
                _index_block_2.resize(indexes_per_block);
                if (index_in_segment % indexes_per_block == 0)
                    indirect_blocks_added.push_back(index_res.value());
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
    if (index_in_segment == 0
        || (index_in_segment < indexes_per_block * indexes_per_block * indexes_per_block
            && _index_block_1.empty())) {
        if (index_in_segment != 0 || _index < _occupied_blocks) {
            auto read_res = _readIndexBlock(_inode.trebly_indirect_block, _index_block_1);
            if (!read_res.has_value()) {
                return std::unexpected(read_res.error());
            }
            if (index_in_segment == 0)
                indirect_blocks_added.push_back(_inode.trebly_indirect_block);
        }

        else {
            auto index_res = _findAndReserveBlock();
            if (!index_res.has_value()) {
                return std::unexpected(index_res.error());
            }
            _inode.trebly_indirect_block = index_res.value();
            _block_device.formatBlock(index_res.value());
            _index_block_1.resize(indexes_per_block);
            if (index_in_segment == 0)
                indirect_blocks_added.push_back(_inode.trebly_indirect_block);
        }
    }

    if (index_in_segment < indexes_per_block * indexes_per_block * indexes_per_block) {
        if (index_in_segment % (indexes_per_block * indexes_per_block) == 0
            || _index_block_2.empty()) {
            if (_index < _occupied_blocks
                || index_in_segment % (indexes_per_block * indexes_per_block) != 0) {
                auto read_res = _readIndexBlock(
                    _index_block_1[index_in_segment / (indexes_per_block * indexes_per_block)],
                    _index_block_2);
                if (!read_res.has_value()) {
                    return std::unexpected(read_res.error());
                }
                if (index_in_segment % (indexes_per_block * indexes_per_block) == 0)
                    indirect_blocks_added.push_back(
                        _index_block_1[index_in_segment / (indexes_per_block * indexes_per_block)]);
            } else {
                auto index_res = _findAndReserveBlock();
                if (!index_res.has_value()) {
                    return std::unexpected(index_res.error());
                }
                _index_block_2.resize(indexes_per_block);
                _index_block_1[index_in_segment / (indexes_per_block * indexes_per_block)]
                    = index_res.value();
                auto write_res = _writeIndexBlock(_inode.trebly_indirect_block, _index_block_1);
                if (!write_res.has_value()) {
                    _block_manager.free(index_res.value());
                    return std::unexpected(write_res.error());
                }
                if (index_in_segment % (indexes_per_block * indexes_per_block) == 0)
                    indirect_blocks_added.push_back(index_res.value());
            }
        }

        if (index_in_segment % indexes_per_block == 0 || _index_block_3.empty()) {
            if (_index < _occupied_blocks || index_in_segment % indexes_per_block != 0) {
                auto read_res = _readIndexBlock(
                    _index_block_2[(index_in_segment / indexes_per_block) % indexes_per_block],
                    _index_block_3);
                if (!read_res.has_value()) {
                    return std::unexpected(read_res.error());
                }
                if (index_in_segment % indexes_per_block == 0)
                    indirect_blocks_added.push_back(
                        _index_block_2[(index_in_segment / indexes_per_block) % indexes_per_block]);
            } else {
                auto index_res = _findAndReserveBlock();
                if (!index_res.has_value()) {
                    return std::unexpected(index_res.error());
                }
                _index_block_3.resize(indexes_per_block);
                _index_block_2[(index_in_segment / indexes_per_block) % indexes_per_block]
                    = index_res.value();
                auto write_res = _writeIndexBlock(
                    _index_block_1[index_in_segment / (indexes_per_block * indexes_per_block)],
                    _index_block_2);
                if (!write_res.has_value()) {
                    _block_manager.free(index_res.value());
                    return std::unexpected(write_res.error());
                }
                if (index_in_segment % indexes_per_block == 0)
                    indirect_blocks_added.push_back(index_res.value());
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
    return std::unexpected(FsError::FileIO_OutOfBounds);
}

std::expected<block_index_t, FsError> BlockIndexIterator::next()
{
    std::array<block_index_t, 3> indirect_blocks_added_buffer;
    static_vector<block_index_t> indirect_blocks_added_vec(indirect_blocks_added_buffer.data(), 3);
    auto res = nextWithIndirectBlocksAdded(indirect_blocks_added_vec);
    if (!res.has_value())
        return std::unexpected(res.error());
    return res.value();
}

std::expected<void, FsError> BlockIndexIterator::_readIndexBlock(
    block_index_t index, static_vector<block_index_t>& buf)
{
    size_t indexes_per_block = _block_device.dataSize() / sizeof(block_index_t);

    static_vector<uint8_t> temp_buff(reinterpret_cast<uint8_t*>(buf.data()), indexes_per_block * sizeof(block_index_t), 0);
    auto read_res = _block_device.readBlock(
        DataLocation(index, 0), indexes_per_block * sizeof(block_index_t), temp_buff);

    if (!read_res.has_value())
        return std::unexpected(read_res.error());

    buf.resize(indexes_per_block);
    return {};
}

std::expected<void, FsError> BlockIndexIterator::_writeIndexBlock(
    block_index_t index, const static_vector<block_index_t>& indices)
{
    size_t indexes_per_block = _block_device.dataSize() / sizeof(block_index_t);
    size_t bytes_to_write = indices.size() * sizeof(block_index_t);

    static_vector<uint8_t> bytes(reinterpret_cast<uint8_t*>(const_cast<block_index_t*>(indices.data())), bytes_to_write, bytes_to_write);

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
