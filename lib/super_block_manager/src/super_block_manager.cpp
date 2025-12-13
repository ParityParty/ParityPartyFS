#include "super_block_manager/super_block_manager.hpp"
#include "common/bit_helpers.hpp"
#include "common/static_vector.hpp"
#include <array>
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
        return {};

    if (!_superBlock.has_value())
        return std::unexpected(FsError::SuperBlockManager_InvalidRequest);

    // write at the beggining

    std::array<SuperBlock, 2> sb_buffer;
    static_vector<SuperBlock> sb_data(sb_buffer.data(), 2);
    sb_data.push_back(_superBlock.value());
    sb_data.push_back(_superBlock.value());

    if (writeAtBeginning) {
        auto write_res = _disk.write(0, static_vector<uint8_t>((uint8_t*)sb_data.data(), sb_data.size() * sizeof(SuperBlock), sb_data.size() * sizeof(SuperBlock)));
        if (!write_res.has_value())
            return std::unexpected(write_res.error());
    }

    // write at the end
    if (writeAtEnd) {
        sb_data.resize(1);
        auto write_res = _disk.write(_startByte, static_vector<uint8_t>((uint8_t*)sb_data.data(), sizeof(SuperBlock), sizeof(SuperBlock)));
        if (!write_res.has_value())
            return std::unexpected(write_res.error());
    }
    return {};
}

std::expected<void, FsError> SuperBlockManager::_readFromDisk()
{
    size_t read = 0;
    std::array<SuperBlock, 3> buffer;

    // read from the beginning
    static_vector<uint8_t> temp1((uint8_t*)buffer.data(), 2 * sizeof(SuperBlock));
    auto read_res = _disk.read(0, 2 * sizeof(SuperBlock), temp1);
    if (!read_res.has_value())
        return std::unexpected(read_res.error());

    // read from the end

    static_vector<uint8_t> temp2((uint8_t*)buffer.data() + 2 * sizeof(SuperBlock), sizeof(SuperBlock));
    read_res = _disk.read(_startByte, sizeof(SuperBlock), temp2);
    if (!read_res.has_value())
        return std::unexpected(read_res.error());

    auto voting_res = _performBitVoting(static_vector<SuperBlock>(buffer.data(), 3));

    // Check if the disk is formatted by verifying the signature
    if (std::memcmp(voting_res.finalData.signature, "PPFS", 4) != 0) {
        return std::unexpected(FsError::PpFS_DiskNotFormatted);
    }

    auto write_res = _writeToDisk(voting_res.damaged1 || voting_res.damaged2, voting_res.damaged3);
    if (!write_res.has_value()) {
        return std::unexpected(write_res.error());
    }

    _superBlock = voting_res.finalData;

    return {};
}

VotingResult SuperBlockManager::_performBitVoting(
    const static_vector<SuperBlock> copies)
{
    size_t numBytes = sizeof(SuperBlock);
    size_t numBits = numBytes * 8;
    SuperBlock sb;

    static_vector<uint8_t> copies_bytes(static_vector<uint8_t>((uint8_t*)copies.data(), copies.size() * sizeof(SuperBlock)));
    static_vector<uint8_t> resultBytes((uint8_t*)&sb, numBytes, numBytes);

    bool damaged1 = false;
    bool damaged2 = false;
    bool damaged3 = false;

    for (size_t bit = 0; bit < numBits; bit++) {
        bool b1 = BitHelpers::getBit(copies_bytes, bit);
        bool b2 = BitHelpers::getBit(copies_bytes, bit + numBits);
        bool b3 = BitHelpers::getBit(copies_bytes, bit + numBits * 2);

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

    VotingResult res(sb, damaged1, damaged2, damaged3);

    return res;
}