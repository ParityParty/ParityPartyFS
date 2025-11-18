#include "file_io/file_io.hpp"
#include <cstring>

FileIO::FileIO(IBlockDevice& block_device)
    : _block_device(block_device)
{
}

std::expected<std::vector<std::byte>, FsError> FileIO::readFile(
    Inode inode, size_t offset, size_t bytes_to_read)
{
    if (offset + bytes_to_read > inode.file_size)
        return std::unexpected(FsError::OutOfBounds);

    size_t block_number = offset / _block_device.dataSize();

    BlockIndexIterator indexIterator(block_number, inode, _block_device, false);
    std::vector<std::byte> data;
    data.reserve(bytes_to_read);

    while (auto next_block = indexIterator.next()) {
        auto read_res = _block_device.readBlock(DataLocation(*next_block, offset), bytes_to_read);
        if (!read_res.has_value())
            return std::unexpected(read_res.error());
        data.insert(data.end(), read_res.value().begin(), read_res.value().end());

        offset = 0;
        bytes_to_read -= read_res.value().size();

        if (bytes_to_read == 0)
            return data;
    }

    return std::unexpected(FsError::InternalError);
}

std::expected<void, FsError> FileIO::writeFile(
    Inode inode, size_t offset, std::vector<std::byte> bytes_to_write)
{
    if (offset + bytes_to_write.size() > inode.file_size)
        return std::unexpected(FsError::OutOfBounds);

    size_t written_bytes = 0;
    size_t block_number = offset / _block_device.dataSize();

    BlockIndexIterator indexIterator(block_number, inode, _block_device, true);

    while (auto next_block = indexIterator.next()) {
        auto write_res = _block_device.writeBlock(
            { bytes_to_write.begin() + written_bytes, bytes_to_write.end() },
            DataLocation(*next_block, offset));
        if (!write_res.has_value())
            return std::unexpected(write_res.error());

        offset = 0;
        written_bytes += write_res.value();

        if (written_bytes == bytes_to_write.size())
            return {};
    }

    return std::unexpected(FsError::InternalError);
}

BlockIndexIterator::BlockIndexIterator(
    size_t index, Inode& inode, IBlockDevice& block_device, bool should_resize)
    : _index(index)
    , _block_device(block_device)
    , _inode(inode)
    , _should_resize(should_resize)
{
    _occupied_blocks = (inode.file_size + block_device.dataSize() - 1) / _block_device.dataSize();
}

std::optional<block_index_t> BlockIndexIterator::next()
{
    if (!_should_resize & _index >= _occupied_blocks)
        _finished = true;

    if (_finished)
        return {};

    if (_index < 12) {
        if (_index >= _occupied_blocks) {
            auto index_res = _findAndReserveBlock();
            if (!index_res.has_value()) {
                _finished = true;
                return {};
            }
            _occupied_blocks++;
            _inode.direct_blocks[_index] = index_res.value();
            _inode_changed = true;
        }
        return _inode.direct_blocks[_index++];
    }

    size_t indexes_per_block = _block_device.dataSize() / sizeof(block_index_t);

    if (_index - 12 == 0) {
        if (_index < _occupied_blocks) {
            auto read_res = _readIndexBlock(_inode.indirect_block);
            if (!read_res.has_value()) {
                _finished = true;
                return {};
            }
            _index_block_1 = read_res.value();
        } else {
            auto index_res = _findAndReserveBlock();
            if (!index_res.has_value()) {
                _finished = true;
                return {};
            }
            _inode.indirect_block = index_res.value();
            _inode_changed = true;
            _block_device.formatBlock(index_res.value());
            _index_block_1 = std::vector<block_index_t>(indexes_per_block);
        }
    }

    if (_index - 12 < indexes_per_block) {
        if (_index >= _occupied_blocks) {
            auto index_res = _findAndReserveBlock();
            if (!index_res.has_value()) {
                _finished = true;
                return {};
            }
            (*_index_block_1)[_index - 12] = index_res.value();
            _writeIndexBlock(_inode.indirect_block, *_index_block_1);
        }
        return (*_index_block_1)[_index++ - 12];
    }

    if (_index - 12 - indexes_per_block == 0) {
        if (_index < _occupied_blocks) {
            auto read_res = _readIndexBlock(_inode.doubly_indirect_block);
            if (!read_res.has_value()) {
                _finished = true;
                return {};
            }
            _index_block_1 = read_res.value();
        }

        else {
            auto index_res = _findAndReserveBlock();
            if (!index_res.has_value()) {
                _finished = true;
                return {};
            }
            _inode.doubly_indirect_block = index_res.value();
            _inode_changed = true;
            _block_device.formatBlock(index_res.value());
            _index_block_1 = std::vector<block_index_t>(indexes_per_block);
        }
    }

    // doubly
    if (_index - 12 - indexes_per_block < indexes_per_block * indexes_per_block) {
        if (_index - 12 - indexes_per_block % indexes_per_block == 0) {
            if (_index < _occupied_blocks) {
                auto index = (_index - 12 - indexes_per_block) / indexes_per_block;
                auto read_res = _readIndexBlock((*_index_block_1)[index]);
                if (!read_res.has_value()) {
                    _finished = true;
                    return {};
                }
                _index_block_2 = read_res.value();
            } else {
                auto index_res = _findAndReserveBlock();
                if (!index_res.has_value()) {
                    _finished = true;
                    return {};
                }
                (*_index_block_1)[_index - 12 - indexes_per_block] = index_res.value();
                _writeIndexBlock(_inode.doubly_indirect_block, *_index_block_1);
            }
        }
        if (_index < _occupied_blocks) {
            auto index_res = _findAndReserveBlock();
            if (!index_res.has_value()) {
                _finished = true;
                return {};
            }
            (*_index_block_2)[(_index++ - 12 - indexes_per_block) % indexes_per_block]
                = index_res.value();
            _writeIndexBlock(
                (*_index_block_1)[(_index++ - 12 - indexes_per_block) / indexes_per_block],
                *_index_block_2);
        }
        return (*_index_block_2)[(_index++ - 12 - indexes_per_block) % indexes_per_block];
    }
    // triply
    if (_index - 12 - indexes_per_block - indexes_per_block * indexes_per_block == 0) {
        if (_index < _occupied_blocks) {
            auto read_res = _readIndexBlock(_inode.trebly_indirect_block);
            if (!read_res.has_value()) {
                _finished = true;
                return {};
            }
            _index_block_1 = read_res.value();
        }

        else {
            auto index_res = _findAndReserveBlock();
            if (!index_res.has_value()) {
                _finished = true;
                return {};
            }
            _inode.trebly_indirect_block = index_res.value();
            _inode_changed = true;
            _block_device.formatBlock(index_res.value());
            _index_block_1 = std::vector<block_index_t>(indexes_per_block);
        }
    }

    if (_index - 12 - indexes_per_block - indexes_per_block * indexes_per_block
        < indexes_per_block * indexes_per_block * indexes_per_block) {
        if (_index - 12 - indexes_per_block
                - indexes_per_block * indexes_per_block % indexes_per_block * indexes_per_block
            == 0) {
            if (_index < _occupied_blocks) {
                auto index
                    = (_index - 12 - indexes_per_block - indexes_per_block * indexes_per_block)
                    / indexes_per_block * indexes_per_block;
                auto read_res = _readIndexBlock((*_index_block_1)[index]);
                if (!read_res.has_value()) {
                    _finished = true;
                    return {};
                }
                _index_block_2 = read_res.value();
            } else {
                auto index_res = _findAndReserveBlock();
                if (!index_res.has_value()) {
                    _finished = true;
                    return {};
                }
                _index_block_2 = std::vector<block_index_t>(indexes_per_block);
                (*_index_block_1)[(_index - 12 - indexes_per_block
                                      - indexes_per_block * indexes_per_block)
                    / indexes_per_block * indexes_per_block]
                    = index_res.value();
                _writeIndexBlock(_inode.trebly_indirect_block, *_index_block_1);
            }
        }

        if (_index - 12 - indexes_per_block
                - indexes_per_block * indexes_per_block % indexes_per_block
            == 0) {
            if (_index < _occupied_blocks) {
                auto index
                    = (_index - 12 - indexes_per_block - indexes_per_block * indexes_per_block)
                    / indexes_per_block;
                auto read_res = _readIndexBlock((*_index_block_2)[index]);
                if (!read_res.has_value()) {
                    _finished = true;
                    return {};
                }
                _index_block_3 = read_res.value();
            } else {
                auto index_res = _findAndReserveBlock();
                if (!index_res.has_value()) {
                    _finished = true;
                    return {};
                }
                _index_block_3 = std::vector<block_index_t>(indexes_per_block);
                auto index
                    = (_index - 12 - indexes_per_block - indexes_per_block * indexes_per_block)
                    / indexes_per_block;
                (*_index_block_2)[index] = index_res.value();
                _writeIndexBlock((*_index_block_1)[(_index - 12 - indexes_per_block
                                                       - indexes_per_block * indexes_per_block)
                                     % indexes_per_block],
                    *_index_block_2);
            }
        }

        if (_index < _occupied_blocks) {
            auto index_res = _findAndReserveBlock();
            if (!index_res.has_value()) {
                _finished = true;
                return {};
            }
            (*_index_block_3)[(
                _index++ - 12 - indexes_per_block - indexes_per_block * indexes_per_block)]
                = index_res.value();
            _writeIndexBlock((*_index_block_2)[(_index - 12 - indexes_per_block
                                                   - indexes_per_block * indexes_per_block)
                                 / indexes_per_block],
                (*_index_block_3));
        }
        return (*_index_block_3)[(_index++ - 12 - indexes_per_block
                                     - indexes_per_block * indexes_per_block)
            % indexes_per_block];
    }

    _finished = true;
    return {};
}

std::expected<std::vector<block_index_t>, FsError> BlockIndexIterator::_readIndexBlock(
    block_index_t index)
{
    size_t indexes_per_block = _block_device.dataSize() / sizeof(block_index_t);

    auto raw = _block_device.readBlock(
        DataLocation(index, 0), indexes_per_block * sizeof(block_index_t));

    if (!raw.has_value())
        return std::unexpected(raw.error()); // sanity ✨

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

    // konwersja vector<block_index_t> -> vector<std::byte>
    std::vector<std::byte> bytes;
    bytes.resize(indexes_per_block * sizeof(block_index_t));

    // wypełniamy zerami resztę bloku (jeżeli jest)
    std::memset(bytes.data(), 0, bytes.size());

    // kopiujemy realne dane
    std::memcpy(bytes.data(), indices.data(), indices.size() * sizeof(block_index_t));

    // zapisujemy blok na dysk ✨
    auto res = _block_device.writeBlock(bytes, DataLocation(index, 0));

    if (!res.has_value())
        return std::unexpected(res.error());

    return {};
}
