#include "super_block_manager/super_block_manager.hpp"
#include "common/bit_helpers.hpp"
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

    return _superBlock.value();
}

std::expected<void, FsError> SuperBlockManager::put(SuperBlock new_super_block)
{
    _superBlock = new_super_block;

    auto write_res = _writeToDisk(true, true);

    if (!write_res.has_value())
        return std::unexpected(write_res.error());

    return {};
}

std::expected<BlockRange, FsError> SuperBlockManager::getFreeBlocksIndexes()
{
    if (!_superBlock.has_value()) {
        auto read_res = _readFromDisk();
        if (!read_res.has_value())
            return std::unexpected(read_res.error());
    }

    block_index_t firstFreeBlock
        = (2 * sizeof(SuperBlock) + _superBlock->block_size - 1) / _superBlock->block_size;
    block_index_t lastFreeBlock = (_disk.size() / _superBlock->block_size)
        - (sizeof(SuperBlock) + _superBlock->block_size - 1) / _superBlock->block_size;
    return BlockRange { firstFreeBlock, lastFreeBlock };
}

std::expected<void, FsError> SuperBlockManager::_writeToDisk(bool writeAtBeginning, bool writeAtEnd)
{
    if (!writeAtBeginning && !writeAtEnd)
        return std::unexpected(FsError::InvalidRequest);

    if (!_superBlock.has_value())
        return std::unexpected(FsError::InvalidRequest);

    // write at the beggining

    std::vector<uint8_t> buffer(2 * sizeof(SuperBlock));
    std::memcpy(buffer.data(), &_superBlock.value(), sizeof(SuperBlock));
    std::memcpy(buffer.data() + sizeof(SuperBlock), &_superBlock.value(), sizeof(SuperBlock));

    if (writeAtBeginning) {
        auto write_res = _disk.write(0, buffer);
        if (!write_res.has_value())
            return std::unexpected(write_res.error());
    }

    // write at the end
    if (writeAtEnd) {
        auto write_res
            = _disk.write(_startByte, { buffer.begin(), buffer.begin() + sizeof(SuperBlock) });
        if (!write_res.has_value())
            return std::unexpected(write_res.error());
    }
    return {};
}

std::expected<void, FsError> SuperBlockManager::_readFromDisk()
{
    size_t read = 0;
    std::vector<uint8_t> buffer;
    buffer.reserve(sizeof(SuperBlock) * 3);

    // read from the beginning
    auto read_res = _disk.read(0, 2 * sizeof(SuperBlock));
    if (!read_res.has_value())
        return std::unexpected(read_res.error());
    buffer.insert(buffer.end(), read_res.value().begin(), read_res.value().end());

    // read from the end

    read_res = _disk.read(_startByte, sizeof(SuperBlock));
    if (!read_res.has_value())
        return std::unexpected(read_res.error());
    buffer.insert(buffer.end(), read_res.value().begin(), read_res.value().end());

    SuperBlock sb;

    auto voting_res = _performBitVoting(
        std::vector<uint8_t>(buffer.begin(), buffer.begin() + sizeof(SuperBlock)),
        std::vector<uint8_t>(
            buffer.begin() + sizeof(SuperBlock), buffer.begin() + 2 * sizeof(SuperBlock)),
        std::vector<uint8_t>(
            buffer.begin() + 2 * sizeof(SuperBlock), buffer.begin() + 3 * sizeof(SuperBlock)));

    std::memcpy(&sb, voting_res.finalData.data(), sizeof(SuperBlock));

    _writeToDisk(voting_res.damaged1 || voting_res.damaged2, voting_res.damaged3);

    if (std::memcmp(sb.signature, "PPFS", 4) != 0) {
        return std::unexpected(FsError::DiskNotFormatted);
    }

    _superBlock = sb;

    return {};
}

VotingResult SuperBlockManager::_performBitVoting(const std::vector<uint8_t>& copy1,
    const std::vector<uint8_t>& copy2, const std::vector<uint8_t>& copy3)
{
    size_t numBytes = copy1.size();
    size_t numBits = numBytes * 8;

    std::vector<uint8_t> resultBytes(numBytes, 0);

    bool damaged1 = false;
    bool damaged2 = false;
    bool damaged3 = false;

    for (size_t bit = 0; bit < numBits; bit++) {
        bool b1 = BitHelpers::getBit(copy1, bit);
        bool b2 = BitHelpers::getBit(copy2, bit);
        bool b3 = BitHelpers::getBit(copy3, bit);

        int sum = b1 + b2 + b3;
        bool majority = (sum >= 2);

        BitHelpers::setBit(resultBytes, bit, majority);

        if (b1 != majority)
            damaged1 = true;
        if (b2 != majority)
            damaged2 = true;
        if (b3 != majority)
            damaged3 = true;
    }

    VotingResult res;
    res.finalData = resultBytes;
    res.damaged1 = damaged1;
    res.damaged2 = damaged2;
    res.damaged3 = damaged3;

    return res;
}