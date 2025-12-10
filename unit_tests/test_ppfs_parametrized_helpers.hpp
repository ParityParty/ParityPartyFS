#pragma once

#include "blockdevice/ecc_type.hpp"
#include "disk/stack_disk.hpp"
#include "filesystem/ppfs.hpp"
#include "filesystem/types.hpp"
#include "super_block_manager/super_block.hpp"
#include <cctype>
#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <string>
#include <vector>

struct TestConfig {
    ECCType ecc_type;
    uint32_t block_size;
    uint32_t rs_correctable_bytes = 3; // Only used for Reed-Solomon
    std::string test_name;

    TestConfig(ECCType ecc, uint32_t bs, uint32_t rs = 3, std::string name = "")
        : ecc_type(ecc)
        , block_size(bs)
        , rs_correctable_bytes(rs)
        , test_name(name)
    {
        if (test_name.empty()) {
            test_name = generateTestName();
        }
    }

private:
    std::string generateTestName()
    {
        std::string name;
        switch (ecc_type) {
        case ECCType::None:
            name = "None";
            break;
        case ECCType::Parity:
            name = "Parity";
            break;
        case ECCType::Crc:
            name = "CRC";
            break;
        case ECCType::Hamming:
            name = "Hamming";
            break;
        case ECCType::ReedSolomon:
            name = "ReedSolomon_RS" + std::to_string(rs_correctable_bytes);
            break;
        }
        name += "_BS" + std::to_string(block_size);
        return name;
    }
};

// Helper to sanitize test names for Google Test (only alphanumeric and underscore)
inline std::string sanitizeTestName(const std::string& name)
{
    std::string sanitized;
    sanitized.reserve(name.size());
    for (char c : name) {
        if (std::isalnum(c) || c == '_') {
            sanitized += c;
        } else {
            sanitized += '_';
        }
    }
    return sanitized;
}

// Custom printer for TestConfig to avoid hex dump output
inline void PrintTo(const TestConfig& config, std::ostream* os) { *os << config.test_name; };

class PpFSParametrizedTest : public ::testing::TestWithParam<TestConfig> {
protected:
    StackDisk disk;
    std::unique_ptr<PpFS> fs;
    FsConfig config;

    void SetUp() override
    {
        const auto& param = GetParam();

        config.total_size = disk.size();
        config.block_size = param.block_size;
        config.average_file_size = 1024;
        config.ecc_type = param.ecc_type;

        if (param.ecc_type == ECCType::ReedSolomon) {
            config.rs_correctable_bytes = param.rs_correctable_bytes;
        }

        fs = std::make_unique<PpFS>(disk);
    }

    void TearDown() override { fs.reset(); }
};

inline std::vector<TestConfig> generateCrcConfigs()
{
    std::vector<TestConfig> configs;

    // Block sizes for non-Reed-Solomon ECC types
    std::vector<uint32_t> standard_block_sizes = { 256, 1024, 4096 };

    // Generate configurations for standard ECC types
    for (auto block_size : standard_block_sizes) {
        configs.emplace_back(ECCType::Crc, block_size);
    }
    return configs;
}

inline std::vector<TestConfig> generateHammingConfigs()
{
    std::vector<TestConfig> configs;

    // Block sizes for non-Reed-Solomon ECC types
    std::vector<uint32_t> standard_block_sizes = { 256, 1024, 4096 };

    // Generate configurations for standard ECC types
    for (auto block_size : standard_block_sizes) {
        configs.emplace_back(ECCType::Hamming, block_size);
    }
    return configs;
}

inline std::vector<TestConfig> generateRSConfigs()
{
    std::vector<TestConfig> configs;
    std::vector<uint32_t> rs_block_sizes = { 256 };
    std::vector<uint32_t> rs_correctable_values = { 1, 2, 3, 4, 5 };

    for (auto block_size : rs_block_sizes) {
        for (auto rs_bytes : rs_correctable_values) {
            configs.emplace_back(ECCType::ReedSolomon, block_size, rs_bytes);
        }
    }

    return configs;
}

inline std::vector<TestConfig> generateNonEccConfigs()
{
    std::vector<TestConfig> configs;

    // Block sizes for non-Reed-Solomon ECC types
    std::vector<uint32_t> standard_block_sizes = { 256, 1024, 4096 };

    // Generate configurations for standard ECC types
    for (auto block_size : standard_block_sizes) {
        configs.emplace_back(ECCType::None, block_size);
    }
    return configs;
}

// Helper function to generate all test configurations
inline std::vector<TestConfig> generateTestConfigs()
{
    std::vector<TestConfig> configs;
    auto hamming = generateHammingConfigs();
    auto rs = generateRSConfigs();
    auto nonEcc = generateNonEccConfigs();
    auto crc = generateCrcConfigs();
    configs.insert(configs.end(), nonEcc.cbegin(), nonEcc.cend());
    configs.insert(configs.end(), crc.cbegin(), crc.cend());
    configs.insert(configs.end(), hamming.cbegin(), hamming.cend());
    configs.insert(configs.end(), rs.cbegin(), rs.cend());

    return configs;
}

// Error injection helper functions
inline void injectBitFlip(StackDisk& disk, size_t byte_offset, uint8_t bit_mask)
{
    auto read_res = disk.read(byte_offset, 1);
    ASSERT_TRUE(read_res.has_value());

    auto bytes = read_res.value();
    bytes[0] ^= bit_mask;

    auto write_res = disk.write(byte_offset, bytes);
    ASSERT_TRUE(write_res.has_value());
}

inline void injectByteError(StackDisk& disk, size_t byte_offset, uint8_t corrupt_value)
{
    auto read_res = disk.read(byte_offset, 1);
    ASSERT_TRUE(read_res.has_value());

    auto bytes = read_res.value();
    bytes[0] = corrupt_value;

    auto write_res = disk.write(byte_offset, bytes);
    ASSERT_TRUE(write_res.has_value());
}

inline void injectRandomBitFlip(StackDisk& disk, size_t start_offset, size_t end_offset)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());

    size_t byte_offset = start_offset + (gen() % (end_offset - start_offset));
    uint8_t bit_mask = 1 << (gen() % 8);

    injectBitFlip(disk, byte_offset, bit_mask);
}

inline void injectRandomByteError(StackDisk& disk, size_t start_offset, size_t end_offset)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());

    size_t byte_offset = start_offset + (gen() % (end_offset - start_offset));
    uint8_t corrupt_value = static_cast<uint8_t>(gen());

    injectByteError(disk, byte_offset, corrupt_value);
}

// Helper to create deterministic test data
inline std::vector<uint8_t> createTestData(size_t size, uint8_t pattern = 0xAA)
{
    std::vector<uint8_t> data(size);
    for (size_t i = 0; i < size; ++i) {
        data[i] = static_cast<uint8_t>(pattern + (i % 256));
    }
    return data;
}

inline std::vector<uint8_t> createAlternatingPattern(size_t size)
{
    std::vector<uint8_t> data(size);
    for (size_t i = 0; i < size; ++i) {
        data[i] = (i % 2 == 0) ? 0xAA : 0x55;
    }
    return data;
}

inline std::vector<uint8_t> createIncrementalPattern(size_t size)
{
    std::vector<uint8_t> data(size);
    for (size_t i = 0; i < size; ++i) {
        data[i] = static_cast<uint8_t>(i % 256);
    }
    return data;
}

// Helper to find data block region on disk
inline size_t findDataBlockRegion(StackDisk& disk, const SuperBlock& sb)
{
    return sb.first_data_blocks_address * sb.block_size;
}
