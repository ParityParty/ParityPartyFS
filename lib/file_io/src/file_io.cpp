#include "file_io/file_io.hpp"

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

    BlockIndexIterator indexIterator(block_number, inode, _block_device);
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

    BlockIndexIterator indexIterator(block_number, inode, _block_device);

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

BlockIndexIterator::BlockIndexIterator(block_index_t first, Inode inode, IBlockDevice& block_device)
    : _index(first)
    , _block_device(block_device)
    , _inode(inode)
{
}

std::optional<block_index_t> BlockIndexIterator::next()
{
    if (finished)
        return {};

    if (_index < 12) {
        return _inode.direct_blocks[_index++];
    }

    size_t indexes_per_block = _block_device.dataSize() / sizeof(block_index_t);

    if (_index - 12 == 0) {
        auto read_res = _readIndexBlock(_inode.indirect_block);
        if (!read_res.has_value()) {
            finished = true;
            return {};
        }
        _index_block_1 = read_res.value();
    }

    if (_index - 12 < indexes_per_block) {
        return (*_index_block_1)[_index++ - 12];
    }

    if (_index - 12 - indexes_per_block == 0) {
        auto read_res = _readIndexBlock(_inode.doubly_indirect_block);
        if (!read_res.has_value()) {
            finished = true;
            return {};
        }
        _index_block_1 = read_res.value();
    }

    if (_index - 12 - indexes_per_block < indexes_per_block * indexes_per_block) {
        if (_index - 12 - indexes_per_block % indexes_per_block == 0) {
            auto index = (_index - 12 - indexes_per_block) / indexes_per_block;
            auto read_res = _readIndexBlock((*_index_block_1)[index]);
            if (!read_res.has_value()) {
                finished = true;
                return {};
            }
            _index_block_2 = read_res.value();
        }

        return (*_index_block_2)[(_index++ - 12 - indexes_per_block) % indexes_per_block];
    }

    if (_index - 12 - indexes_per_block - indexes_per_block * indexes_per_block
        < indexes_per_block * indexes_per_block * indexes_per_block) {
        if (_index - 12 - indexes_per_block
                - indexes_per_block * indexes_per_block % indexes_per_block * indexes_per_block
            == 0) {
            auto index = (_index - 12 - indexes_per_block - indexes_per_block * indexes_per_block)
                / indexes_per_block * indexes_per_block;
            auto read_res = _readIndexBlock((*_index_block_1)[index]);
            if (!read_res.has_value()) {
                finished = true;
                return {};
            }
            _index_block_2 = read_res.value();
        }

        if (_index - 12 - indexes_per_block
                - indexes_per_block * indexes_per_block % indexes_per_block
            == 0) {
            auto index = (_index - 12 - indexes_per_block - indexes_per_block * indexes_per_block)
                / indexes_per_block;
            auto read_res = _readIndexBlock((*_index_block_2)[index]);
            if (!read_res.has_value()) {
                finished = true;
                return {};
            }
            _index_block_2 = read_res.value();
        }

        return (*_index_block_3)[(_index++ - 12 - indexes_per_block
                                     - indexes_per_block * indexes_per_block)
            % indexes_per_block];
    }

    finished = true;
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