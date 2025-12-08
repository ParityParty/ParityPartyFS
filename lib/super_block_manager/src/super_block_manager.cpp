#include "super_block_manager/super_block_manager.hpp"
#include "common/bit_helpers.hpp"
#include "common/static_vector.hpp"
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

    static_vector<uint8_t, 2 * sizeof(SuperBlock)> sb_buffer(2 * sizeof(SuperBlock));
    std::memcpy(sb_buffer.data(), &_superBlock.value(), sizeof(SuperBlock));
    std::memcpy(sb_buffer.data() + sizeof(SuperBlock), &_superBlock.value(), sizeof(SuperBlock));

    if (writeAtBeginning) {
        auto write_res = _disk.write(0, sb_buffer);
        if (!write_res.has_value())
            return std::unexpected(write_res.error());
    }

    // write at the end
    if (writeAtEnd) {
        sb_buffer.resize(sizeof(SuperBlock));
        auto write_res = _disk.write(_startByte, sb_buffer);
        if (!write_res.has_value())
            return std::unexpected(write_res.error());
    }
    return {};
}

std::expected<void, FsError> SuperBlockManager::_readFromDisk()
{
    size_t read = 0;
    static_vector<uint8_t, 3 * sizeof(SuperBlock)> buffer1(0);
    static_vector<uint8_t, 2 * sizeof(SuperBlock)> buffer2(2 * sizeof(SuperBlock));

    // read from the beginning
    auto read_res = _disk.read(0, 2 * sizeof(SuperBlock), buffer2);
    if (!read_res.has_value())
        return std::unexpected(read_res.error());
    buffer1.insert(buffer1.base_type::end(), buffer2.begin(), buffer2.end());

    // read from the end

    read_res = _disk.read(_startByte, sizeof(SuperBlock), buffer2);
    if (!read_res.has_value())
        return std::unexpected(read_res.error());
    buffer1.insert(buffer1.base_type::end(), buffer2.begin(), buffer2.begin() + sizeof(SuperBlock));

    SuperBlock sb;

    auto voting_res = _performBitVoting(buffer1);

    std::memcpy(&sb, voting_res.finalData.data(), sizeof(SuperBlock));

    _writeToDisk(voting_res.damaged1 || voting_res.damaged2, voting_res.damaged3);

    if (std::memcmp(sb.signature, "PPFS", 4) != 0) {
        return std::unexpected(FsError::DiskNotFormatted);
    }

    _superBlock = sb;

    return {};
}

VotingResult SuperBlockManager::_performBitVoting(
    const static_vector<uint8_t, 3 * sizeof(SuperBlock)> copies)
{
    size_t numBytes = sizeof(SuperBlock);
    size_t numBits = numBytes * 8;

    static_vector<uint8_t, sizeof(SuperBlock)> resultBytes(numBytes);

    bool damaged1 = false;
    bool damaged2 = false;
    bool damaged3 = false;

    for (size_t bit = 0; bit < numBits; bit++) {
        bool b1 = BitHelpers::getBit(copies, bit);
        bool b2 = BitHelpers::getBit(copies, bit + numBits);
        bool b3 = BitHelpers::getBit(copies, bit + numBits * 2);

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

    VotingResult res(resultBytes, damaged1, damaged2, damaged3);

    return res;
}