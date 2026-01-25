#include "ppfs/blockdevice/rs_block_device.hpp"

#include "ppfs/common/static_vector.hpp"
#include "ppfs/data_collection/data_colection.hpp"

#include <array>
#include <iostream>

size_t ReedSolomonBlockDevice::numOfBlocks() const { return _disk.size() / _raw_block_size; }

size_t ReedSolomonBlockDevice::dataSize() const { return _raw_block_size - 2 * _correctable_bytes; }

size_t ReedSolomonBlockDevice::rawBlockSize() const { return _raw_block_size; }

std::expected<void, FsError> ReedSolomonBlockDevice::formatBlock(unsigned int block_index)
{
    std::array<uint8_t, MAX_RS_BLOCK_SIZE> zero_data_buffer;
    static_vector<uint8_t> zero_data(zero_data_buffer.data(), MAX_RS_BLOCK_SIZE, _raw_block_size);
    std::fill(zero_data.begin(), zero_data.end(), std::uint8_t(0));
    auto write_result = _disk.write(block_index * _raw_block_size, zero_data);
    return write_result.has_value() ? std::expected<void, FsError> {}
                                    : std::unexpected(write_result.error());
}

std::expected<void, FsError> ReedSolomonBlockDevice::readBlock(
    DataLocation data_location, size_t bytes_to_read, static_vector<uint8_t>& data)
{
    data.resize(0);
    if (data.capacity() < bytes_to_read) {
        return std::unexpected(FsError::Disk_InvalidRequest);
    }
    bytes_to_read = std::min(dataSize() - data_location.offset, bytes_to_read);
    std::array<uint8_t, MAX_RS_BLOCK_SIZE> raw_block_buffer;
    static_vector<uint8_t> raw_block(raw_block_buffer.data(), MAX_RS_BLOCK_SIZE);
    raw_block.resize(_raw_block_size);
    auto read_res
        = _disk.read(data_location.block_index * _raw_block_size, _raw_block_size, raw_block);
    if (!read_res.has_value()) {
        return std::unexpected(read_res.error());
    }

    std::array<uint8_t, MAX_RS_BLOCK_SIZE> decoded_buffer;
    static_vector<uint8_t> decoded_data(decoded_buffer.data(), MAX_RS_BLOCK_SIZE);
    decoded_data.resize(dataSize());
    _fixBlockAndExtract(raw_block, decoded_data, data_location.block_index);

    data.resize(bytes_to_read);
    std::copy_n(decoded_data.begin() + data_location.offset, bytes_to_read, data.begin());
    return {};
}

ReedSolomonBlockDevice::ReedSolomonBlockDevice(
    IDisk& disk, size_t raw_block_size, size_t correctable_bytes, std::shared_ptr<Logger> logger)
    : _disk(disk)
    , _logger(logger)
{
    _raw_block_size = std::min(raw_block_size, static_cast<size_t>(MAX_RS_BLOCK_SIZE));
    _correctable_bytes = std::min(correctable_bytes, _raw_block_size / 2);
    _generator = _calculateGenerator();
}
std::expected<size_t, FsError> ReedSolomonBlockDevice::writeBlock(
    const static_vector<std::uint8_t>& data, DataLocation data_location)
{
    size_t to_write = std::min(data.size(), dataSize() - data_location.offset);

    std::array<uint8_t, MAX_RS_BLOCK_SIZE> raw_block_buffer;
    static_vector<uint8_t> raw_block(raw_block_buffer.data(), MAX_RS_BLOCK_SIZE);
    raw_block.resize(_raw_block_size);
    auto read_res
        = _disk.read(_raw_block_size * data_location.block_index, _raw_block_size, raw_block);
    if (!read_res.has_value()) {
        return std::unexpected(read_res.error());
    }

    std::array<uint8_t, MAX_RS_BLOCK_SIZE> decoded_buffer;
    static_vector<uint8_t> decoded(decoded_buffer.data(), MAX_RS_BLOCK_SIZE);
    decoded.resize(dataSize());
    _fixBlockAndExtract(raw_block, decoded, data_location.block_index);

    std::copy(data.begin(), data.begin() + to_write, decoded.begin() + data_location.offset);

    std::array<uint8_t, MAX_RS_BLOCK_SIZE> encoded_buffer;
    static_vector<uint8_t> encoded_block(encoded_buffer.data(), MAX_RS_BLOCK_SIZE);
    encoded_block.resize(_raw_block_size);
    _encodeBlock(decoded, encoded_block);

    auto disk_result = _disk.write(data_location.block_index * _raw_block_size, encoded_block);

    if (!disk_result.has_value())
        return std::unexpected(disk_result.error());

    return to_write;
}

void ReedSolomonBlockDevice::_encodeBlock(
    const static_vector<std::uint8_t>& raw_block, static_vector<std::uint8_t>& data)
{
    int t = 2 * _correctable_bytes;

    static_vector<GF256> gf_message(
        reinterpret_cast<GF256*>(const_cast<uint8_t*>(raw_block.data())), MAX_RS_BLOCK_SIZE,
        raw_block.size());
    auto message = PolynomialGF256(gf_message);
    auto shifted_message = message.multiply_by_xk(t);

    auto encoded = shifted_message + shifted_message.mod(_generator);

    std::array<GF256, MAX_RS_BLOCK_SIZE> encoded_slice_buffer;
    static_vector<GF256> encoded_slice(encoded_slice_buffer.data(), MAX_RS_BLOCK_SIZE);
    encoded.slice(0, _raw_block_size, encoded_slice);
    static_vector<uint8_t> encoded_bytes(
        reinterpret_cast<uint8_t*>(encoded_slice.data()), MAX_RS_BLOCK_SIZE, encoded_slice.size());

    // Write encoded result to data (output buffer)
    data.resize(encoded_bytes.size());
    std::copy_n(encoded_bytes.data(), encoded_bytes.size(), data.begin());
}

void ReedSolomonBlockDevice::_fixBlockAndExtract(static_vector<std::uint8_t> raw_block,
    const static_vector<std::uint8_t>& data, block_index_t block_index)
{
    static_vector<GF256> gf_static(
        reinterpret_cast<GF256*>(raw_block.data()), MAX_RS_BLOCK_SIZE, raw_block.size());
    auto code_word = PolynomialGF256(gf_static);

    // Calculate syndromes
    std::array<GF256, MAX_RS_BLOCK_SIZE> syndromes_buffer;
    static_vector<GF256> syndromes(syndromes_buffer.data(), MAX_RS_BLOCK_SIZE);
    syndromes.resize(0);

    GF256 alpha = GF256::getPrimitiveElement();
    GF256 power = GF256(alpha);

    bool is_message_correct = true;
    for (int i = 0; i < 2 * _correctable_bytes; i++) {
        auto s = code_word.evaluate(power);
        syndromes.push_back(s);
        if (s != 0)
            is_message_correct = false;
        power = power * alpha;
    }

    if (is_message_correct) {
        _extractMessage(code_word, const_cast<static_vector<std::uint8_t>&>(data));
        return;
    }

    // Calculate error locator polynomial
    auto sigma = _berlekampMassey(syndromes);

    // Find locations of errors
    std::array<GF256, MAX_RS_BLOCK_SIZE> error_positions_buffer;
    static_vector<GF256> error_positions(error_positions_buffer.data(), MAX_RS_BLOCK_SIZE);
    _errorLocations(sigma, error_positions);

    // Calculate omega polynomial to find error values
    auto omega = _calculateOmega(syndromes, sigma);

    // Calculate error values
    std::array<GF256, MAX_RS_BLOCK_SIZE> error_values_buffer;
    static_vector<GF256> error_values(error_values_buffer.data(), MAX_RS_BLOCK_SIZE);
    _forney(omega, sigma, error_positions, error_values);

    // Correct errors
    for (size_t i = 0; i < error_positions.size(); i++) {
        auto pos = error_positions[i].log();
        code_word[pos] = code_word[pos] + error_values[i];
    }

    // Write fixed version to disk
    if (_logger) {
        _logger->logEvent(ErrorCorrectionEvent("ReedSolomon", block_index));
    }

    std::array<GF256, MAX_RS_BLOCK_SIZE> correct_slice_buffer;
    static_vector<GF256> correct_slice(correct_slice_buffer.data(), MAX_RS_BLOCK_SIZE);
    code_word.slice(0, correct_slice);
    static_vector<uint8_t> correct_vec(
        reinterpret_cast<uint8_t*>(correct_slice.data()), MAX_RS_BLOCK_SIZE, correct_slice.size());
    auto disk_result = _disk.write(block_index * _raw_block_size, correct_vec);

    _extractMessage(code_word, const_cast<static_vector<std::uint8_t>&>(data));
}

void ReedSolomonBlockDevice::_extractMessage(PolynomialGF256 p, static_vector<std::uint8_t>& data)
{
    std::array<GF256, MAX_RS_BLOCK_SIZE> message_slice_buffer;
    static_vector<GF256> message_slice(message_slice_buffer.data(), MAX_RS_BLOCK_SIZE);
    p.slice(2 * _correctable_bytes, _raw_block_size, message_slice);
    data.resize(message_slice.size());
    std::copy_n(
        reinterpret_cast<uint8_t*>(message_slice.data()), message_slice.size(), data.begin());
}

PolynomialGF256 ReedSolomonBlockDevice::_calculateGenerator()
{
    PolynomialGF256 g({ GF256(1) });
    GF256 alpha = GF256::getPrimitiveElement();

    GF256 power(alpha);
    for (int i = 0; i < 2 * _correctable_bytes; i++) {
        PolynomialGF256 term({ power, GF256(1) });
        g = g * term;
        power = power * alpha;
    }

    return g;
}

void ReedSolomonBlockDevice::_forney(const PolynomialGF256& omega, PolynomialGF256& sigma,
    const static_vector<GF256>& error_locations, static_vector<GF256>& error_values)
{
    PolynomialGF256 sigma_derivative = sigma.derivative();

    error_values.resize(error_locations.size());
    for (size_t i = 0; i < error_locations.size(); i++) {
        GF256 Xi_inv = error_locations[i].inv();
        GF256 numerator = omega.evaluate(Xi_inv);
        GF256 denominator = sigma_derivative.evaluate(Xi_inv);
        error_values[i] = numerator / denominator;
    }
}

PolynomialGF256 ReedSolomonBlockDevice::_calculateOmega(
    const static_vector<GF256>& syndromes, PolynomialGF256& sigma)
{
    PolynomialGF256 S(syndromes);
    std::array<GF256, MAX_RS_BLOCK_SIZE> omega_slice_buffer;
    static_vector<GF256> omega_slice(omega_slice_buffer.data(), MAX_RS_BLOCK_SIZE);
    (S * sigma).slice(0, syndromes.size(), omega_slice);
    return PolynomialGF256(omega_slice);
}

PolynomialGF256 ReedSolomonBlockDevice::_berlekampMassey(const static_vector<GF256>& syndromes)
{
    PolynomialGF256 sigma({ GF256(1) });
    PolynomialGF256 B({ GF256(1) });
    GF256 b(1);

    int L = 0;
    int m = 1;

    for (size_t n = 0; n < syndromes.size(); n++) {
        GF256 d = syndromes[n];
        for (int i = 1; i <= L; i++) {
            d = d + sigma[i] * syndromes[n - i];
        }

        if (d != 0) {
            auto T = sigma;
            PolynomialGF256 diff = B * PolynomialGF256({ d / b });
            diff = diff.multiply_by_xk(m);
            sigma += diff;

            if (2 * L <= static_cast<int>(n)) {
                L = n + 1 - L;
                B = T;
                b = d;
                m = 1;
            } else {
                m++;
            }
        } else {
            m++;
        }
    }

    return sigma;
}

void ReedSolomonBlockDevice::_errorLocations(
    PolynomialGF256 error_location_polynomial, static_vector<GF256>& error_locations)
{
    error_locations.resize(0);
    for (int i = 1; i <= 255; i++) {
        if (error_location_polynomial.evaluate(i) == 0) {
            error_locations.push_back(GF256(i).inv());
        }
    }
}
