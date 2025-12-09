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
    inode_index_t inode_index, Inode& inode, size_t offset, size_t bytes_to_read)
{
    bytes_to_read = std::min(bytes_to_read, inode.file_size - offset);

    size_t block_number = offset / _block_device.dataSize();
    size_t offset_in_block = offset % _block_device.dataSize();

    BlockIndexIterator indexIterator(block_number, inode, _block_device, _block_manager, false);
    std::vector<std::uint8_t> data;
    data.reserve(bytes_to_read);

    while (bytes_to_read) {
        auto next_block = indexIterator.next();
        if (!next_block.has_value())
            return std::unexpected(next_block.error());
        auto read_res
            = _block_device.readBlock(DataLocation(*next_block, offset_in_block), bytes_to_read);
        if (!read_res.has_value())
            return std::unexpected(read_res.error());
        data.insert(data.end(), read_res.value().begin(), read_res.value().end());

        offset_in_block = 0;
        bytes_to_read -= read_res.value().size();
    }
    return data;
}

std::expected<size_t, FsError> FileIO::writeFile(
    inode_index_t inode_index, Inode& inode, size_t offset, std::vector<uint8_t> bytes_to_write)
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
        auto write_res = _block_device.writeBlock(
            { bytes_to_write.begin() + written_bytes, bytes_to_write.end() },
            DataLocation(*next_block, offset_in_block));
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

            inode.file_size += std::min(free_in_block, new_size - inode.file_size);
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
        auto next_block = indexIterator.nextWithIndirectBlocksAdded();
        if (!next_block.has_value()) {
            return std::unexpected(next_block.error());
        }
        for (auto& index : std::get<1>(next_block.value())) {
            _block_manager.free(index);
        }
        _block_manager.free(std::get<0>(next_block.value()));
    }
    return {};
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

std::expected<std::tuple<block_index_t, std::vector<block_index_t>>, FsError>
BlockIndexIterator::nextWithIndirectBlocksAdded()
{
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
        return std::tuple<block_index_t, std::vector<block_index_t>>(
            _inode.direct_blocks[_index++], {});
    }

    size_t indexes_per_block = _block_device.dataSize() / sizeof(block_index_t);

    // indirect blocks
    auto index_in_segment = _index - 12;
    std::vector<block_index_t> indirect_blocks_added;
    // first block in indirect segment
    if (index_in_segment == 0 || (index_in_segment < indexes_per_block && _index_block_1.empty())) {
        if (_index < _occupied_blocks) {
            // if indirect block already exists, read it
            auto read_res = _readIndexBlock(_inode.indirect_block);
            if (!read_res.has_value()) {
                return std::unexpected(read_res.error());
            }
            if (index_in_segment == 0)
                indirect_blocks_added.push_back(_inode.indirect_block);
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
        return std::tuple(_index_block_1[index_in_segment], indirect_blocks_added);
    }

    // doubly
    index_in_segment -= indexes_per_block;

    if (index_in_segment == 0
        || (index_in_segment < indexes_per_block * indexes_per_block) && _index_block_1.empty()) {
        if (_index < _occupied_blocks) {
            auto read_res = _readIndexBlock(_inode.doubly_indirect_block);
            if (!read_res.has_value()) {
                return std::unexpected(read_res.error());
            }
            _index_block_1 = read_res.value();
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
            _index_block_1 = std::vector<block_index_t>(indexes_per_block);
            if (index_in_segment == 0)
                indirect_blocks_added.push_back(_inode.doubly_indirect_block);
        }
    }

    if (index_in_segment < indexes_per_block * indexes_per_block) {
        // next entry in doubly indirect
        if (index_in_segment % indexes_per_block == 0 || _index_block_2.empty()) {
            if (_index < _occupied_blocks) {
                auto index = (_index - 12 - indexes_per_block) / indexes_per_block;
                auto read_res = _readIndexBlock(_index_block_1[index]);
                if (!read_res.has_value()) {
                    return std::unexpected(read_res.error());
                }
                _index_block_2 = read_res.value();
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
                _index_block_2 = std::vector<block_index_t>(indexes_per_block);
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
        return std::tuple(_index_block_2[(_index++ - 12 - indexes_per_block) % indexes_per_block],
            indirect_blocks_added);
    }
    // trebly
    index_in_segment -= indexes_per_block * indexes_per_block;
    if (index_in_segment == 0
        || (index_in_segment < indexes_per_block * indexes_per_block * indexes_per_block
            && _index_block_1.empty())) {
        if (_index < _occupied_blocks) {
            auto read_res = _readIndexBlock(_inode.trebly_indirect_block);
            if (!read_res.has_value()) {
                return std::unexpected(read_res.error());
            }
            _index_block_1 = read_res.value();
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
            _index_block_1 = std::vector<block_index_t>(indexes_per_block);
            if (index_in_segment == 0)
                indirect_blocks_added.push_back(_inode.trebly_indirect_block);
        }
    }

    if (index_in_segment < indexes_per_block * indexes_per_block * indexes_per_block) {
        if (index_in_segment % (indexes_per_block * indexes_per_block) == 0
            || _index_block_2.empty()) {
            if (_index < _occupied_blocks) {
                auto read_res = _readIndexBlock(
                    _index_block_1[index_in_segment / (indexes_per_block * indexes_per_block)]);
                if (!read_res.has_value()) {
                    return std::unexpected(read_res.error());
                }
                _index_block_2 = read_res.value();
                if (index_in_segment % (indexes_per_block * indexes_per_block) == 0)
                    indirect_blocks_added.push_back(
                        _index_block_1[index_in_segment / (indexes_per_block * indexes_per_block)]);
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
                if (index_in_segment % (indexes_per_block * indexes_per_block) == 0)
                    indirect_blocks_added.push_back(index_res.value());
            }
        }

        if (index_in_segment % indexes_per_block == 0 || _index_block_3.empty()) {
            if (_index < _occupied_blocks) {
                auto read_res = _readIndexBlock(
                    _index_block_2[(index_in_segment / indexes_per_block) % indexes_per_block]);
                if (!read_res.has_value()) {
                    return std::unexpected(read_res.error());
                }
                _index_block_3 = read_res.value();
                if (index_in_segment % indexes_per_block == 0)
                    indirect_blocks_added.push_back(
                        _index_block_2[(index_in_segment / indexes_per_block) % indexes_per_block]);
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
        return std::tuple(
            _index_block_3[index_in_segment % indexes_per_block], indirect_blocks_added);
    }

    _finished = true;
    return std::unexpected(FsError::FileIO_OutOfBounds);
}

std::expected<block_index_t, FsError> BlockIndexIterator::next()
{
    auto res = nextWithIndirectBlocksAdded();
    if (!res.has_value())
        return std::unexpected(res.error());
    return std::get<0>(res.value());
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
        return std::unexpected(FsError::FileIO_InvalidRequest);

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
