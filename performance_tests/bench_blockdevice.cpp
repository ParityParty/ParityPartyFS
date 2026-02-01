#include "ppfs/blockdevice/crc_block_device.hpp"
#include "ppfs/blockdevice/hamming_block_device.hpp"
#include "ppfs/blockdevice/iblock_device.hpp"
#include "ppfs/blockdevice/raw_block_device.hpp"
#include "ppfs/blockdevice/rs_block_device.hpp"
#include "ppfs/common/static_vector.hpp"
#include "ppfs/disk/stack_disk.hpp"

#include <array>
#include <benchmark/benchmark.h>
#include <cmath>
template <class... Args> static void BM_BlockDevice_Read(benchmark::State& state, Args&&... args)
{
    size_t block_size = 256;
    auto args_tuple = std::make_tuple(std::move(args)...);
    StackDisk disk;
    auto raw = RawBlockDevice(block_size, disk);
    auto rs = ReedSolomonBlockDevice(disk, block_size, 16);
    auto hamming = HammingBlockDevice(static_cast<int>(std::log2(block_size)), disk);
    auto crc = CrcBlockDevice(CrcPolynomial::MsgImplicit(0xea), disk, block_size);

    std::map<std::string, IBlockDevice&> block_devices;
    block_devices.emplace("raw", raw);
    block_devices.emplace("crc", crc);
    block_devices.emplace("hamming", hamming);
    block_devices.emplace("rs", rs);

    IBlockDevice& device = block_devices.at(std::get<0>(args_tuple));
    state.counters["BytesRead"] = benchmark::Counter(
        0, benchmark::Counter::kIsIterationInvariantRate, benchmark::Counter::OneK::kIs1024);

    // Allocate buffer for reading
    size_t max_read_size = device.dataSize();
    std::array<std::uint8_t, 4096> read_buffer;
    static_vector<std::uint8_t> read_data(read_buffer.data(), read_buffer.size());

    for (auto _ : state) {
        auto ret = device.readBlock({ 0, 0 }, static_cast<size_t>(state.range(0)), read_data);
        if (!ret.has_value()) {
            state.SkipWithError("readBlock failed");
        }
        benchmark::DoNotOptimize(read_data);
        benchmark::ClobberMemory();
        state.counters["BytesRead"] += read_data.size();
    }
}
BENCHMARK_CAPTURE(BM_BlockDevice_Read, raw_test, std::string("raw"))
    ->RangeMultiplier(2)
    ->Range(1, 256);
BENCHMARK_CAPTURE(BM_BlockDevice_Read, crc_test, std::string("crc"))
    ->RangeMultiplier(2)
    ->Range(1, 256);
BENCHMARK_CAPTURE(BM_BlockDevice_Read, hamming_test, std::string("hamming"))
    ->RangeMultiplier(2)
    ->Range(1, 256);
BENCHMARK_CAPTURE(BM_BlockDevice_Read, rs_test, std::string("rs"))
    ->RangeMultiplier(2)
    ->Range(1, 256);

template <class... Args> static void BM_BlockDevice_Write(benchmark::State& state, Args&&... args)
{
    size_t block_size = 256;
    auto args_tuple = std::make_tuple(std::move(args)...);
    StackDisk disk;
    auto raw = RawBlockDevice(block_size, disk);
    auto rs = ReedSolomonBlockDevice(disk, block_size, 16);
    auto hamming = HammingBlockDevice(8, disk);
    auto crc = CrcBlockDevice(CrcPolynomial::MsgImplicit(0xea), disk, block_size);

    std::map<std::string, IBlockDevice&> block_devices;
    block_devices.emplace("raw", raw);
    block_devices.emplace("crc", crc);
    block_devices.emplace("hamming", hamming);
    block_devices.emplace("rs", rs);

    IBlockDevice& device = block_devices.at(std::get<0>(args_tuple));
    state.counters["BytesWritten"] = benchmark::Counter(
        0, benchmark::Counter::kIsIterationInvariantRate, benchmark::Counter::OneK::kIs1024);

    // Allocate buffer for writing
    size_t write_size = static_cast<size_t>(state.range(0));
    std::array<std::uint8_t, 4096> write_buffer;
    std::fill(write_buffer.begin(), write_buffer.begin() + write_size, std::uint8_t { 0x55 });
    static_vector<std::uint8_t> write_data(write_buffer.data(), write_buffer.size(), write_size);

    for (auto _ : state) {
        auto ret = device.writeBlock(write_data, { 1, 0 });
        if (!ret.has_value()) {
            state.SkipWithError("writeBlock failed");
        }
        benchmark::DoNotOptimize(write_data);
        benchmark::DoNotOptimize(disk);
        benchmark::ClobberMemory();
        state.counters["BytesWritten"] += ret.value();
    }
}
BENCHMARK_CAPTURE(BM_BlockDevice_Write, raw_test, std::string("raw"))
    ->RangeMultiplier(2)
    ->Range(1, 256);
BENCHMARK_CAPTURE(BM_BlockDevice_Write, crc_test, std::string("crc"))
    ->RangeMultiplier(2)
    ->Range(1, 256);
BENCHMARK_CAPTURE(BM_BlockDevice_Write, hamming_test, std::string("hamming"))
    ->RangeMultiplier(2)
    ->Range(1, 256);
BENCHMARK_CAPTURE(BM_BlockDevice_Write, rs_test, std::string("rs"))
    ->RangeMultiplier(2)
    ->Range(1, 256);

BENCHMARK_MAIN();
