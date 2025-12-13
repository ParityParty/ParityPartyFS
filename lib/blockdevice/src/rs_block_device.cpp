#include "blockdevice/rs_block_device.hpp"

#include "common/static_vector.hpp"
#include "data_collection/data_colection.hpp"
#include "ecc_helpers/gf256_utils.hpp"

#include <array>
#include <iostream>

size_t ReedSolomonBlockDevice::numOfBlocks() const { return _disk.size() / _raw_block_size; }

size_t ReedSolomonBlockDevice::dataSize() const { return _raw_block_size - 2 * _correctable_bytes; }

size_t ReedSolomonBlockDevice::rawBlockSize() const { return _raw_block_size; }

std::expected<void, FsError> ReedSolomonBlockDevice::formatBlock(unsigned int block_index)
{
    std::array<uint8_t, MAX_BLOCK_SIZE> zero_data_buffer;
    static_vector<uint8_t> zero_data(zero_data_buffer.data(), MAX_BLOCK_SIZE, _raw_block_size);
    std::fill(zero_data.begin(), zero_data.end(), std::uint8_t(0));
    auto write_result = _disk.write(block_index * _raw_block_size, zero_data);
    return write_result.has_value() ? std::expected<void, FsError> {}
                                    : std::unexpected(write_result.error());
}

std::expected<void, FsError> ReedSolomonBlockDevice::readBlock(
    DataLocation data_location, size_t bytes_to_read, static_vector<uint8_t>& data)
{
    bytes_to_read = std::min(dataSize() - data_location.offset, bytes_to_read);
    std::array<uint8_t, MAX_BLOCK_SIZE> raw_block_buffer;
    static_vector<uint8_t> raw_block(raw_block_buffer.data(), MAX_BLOCK_SIZE);
    raw_block.resize(_raw_block_size);
    auto read_res = _disk.read(data_location.block_index * _raw_block_size, _raw_block_size, raw_block);
    if (!read_res.has_value()) {
        return std::unexpected(read_res.error());
    }

    std::vector<std::uint8_t> raw_vec(raw_block.begin(), raw_block.end());
    auto decoded_data = _fixBlockAndExtract(raw_vec, data_location.block_index);

    data.resize(bytes_to_read);
    std::copy_n(decoded_data.begin() + data_location.offset, bytes_to_read, data.begin());
    return {};
}

ReedSolomonBlockDevice::ReedSolomonBlockDevice(
    IDisk& disk, size_t raw_block_size, size_t correctable_bytes, std::shared_ptr<Logger> logger)
    : _disk(disk)
    , _logger(logger)
{
    _raw_block_size = std::min(raw_block_size, static_cast<size_t>(MAX_BLOCK_SIZE));
    _correctable_bytes = std::min(correctable_bytes, _raw_block_size / 2);
    _generator = _calculateGenerator();
}
std::expected<size_t, FsError> ReedSolomonBlockDevice::writeBlock(
    const static_vector<std::uint8_t>& data, DataLocation data_location)
{
    size_t to_write = std::min(data.size(), dataSize() - data_location.offset);

    std::array<uint8_t, MAX_BLOCK_SIZE> raw_block_buffer;
    static_vector<uint8_t> raw_block(raw_block_buffer.data(), MAX_BLOCK_SIZE);
    raw_block.resize(_raw_block_size);
    auto read_res = _disk.read(_raw_block_size * data_location.block_index, _raw_block_size, raw_block);
    if (!read_res.has_value()) {
        return std::unexpected(read_res.error());
    }

    std::vector<std::uint8_t> raw_vec(raw_block.begin(), raw_block.end());
    auto decoded = _fixBlockAndExtract(raw_vec, data_location.block_index);

    std::copy(data.begin(), data.begin() + to_write, decoded.begin() + data_location.offset);

    auto new_encoded_block = _encodeBlock(decoded);

    std::array<uint8_t, MAX_BLOCK_SIZE> encoded_buffer;
    static_vector<uint8_t> encoded_block(encoded_buffer.data(), MAX_BLOCK_SIZE, new_encoded_block.size());
    std::copy_n(new_encoded_block.begin(), new_encoded_block.size(), encoded_block.begin());

    auto disk_result = _disk.write(data_location.block_index * _raw_block_size, encoded_block);

    if (!disk_result.has_value())
        return std::unexpected(disk_result.error());

    return to_write;
}

std::vector<std::uint8_t> ReedSolomonBlockDevice::_encodeBlock(std::vector<std::uint8_t> data)
{
    int t = 2 * _correctable_bytes;

    auto message = PolynomialGF256(gf256_utils::bytes_to_gf(data));
    auto shifted_message = message.multiply_by_xk(t);

    auto encoded = shifted_message + shifted_message.mod(_generator);

    return gf256_utils::gf_to_bytes(encoded.slice(0, _raw_block_size));
}

std::vector<std::uint8_t> ReedSolomonBlockDevice::_fixBlockAndExtract(
    std::vector<std::uint8_t> raw_bytes, block_index_t block_index)
{
    using namespace gf256_utils;

    auto code_word = PolynomialGF256(bytes_to_gf(raw_bytes));

    // Calculate syndromes
    std::vector<GF256> syndromes;
    syndromes.reserve(2 * _correctable_bytes);

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

    if (is_message_correct)
        return _extractMessage(code_word);

    // Calculate error locator polynomial
    auto sigma = _berlekampMassey(syndromes);

    // Find locations of errors
    auto error_positions = _errorLocations(sigma);

    // Calculate omega polynomial to find error values
    auto omega = _calculateOmega(syndromes, sigma);

    // Calculate error valuer
    auto error_values = _forney(omega, sigma, error_positions);

    // Correct errors
    for (size_t i = 0; i < error_positions.size(); i++) {
        auto pos = error_positions[i].log();
        code_word[pos] = code_word[pos] + error_values[i];
    }

    // TODO: Write fixed version to disk
    if (_logger) {
        _logger->logEvent(ErrorCorrectionEvent("ReedSolomon", block_index));
    }

    auto correct_data = gf256_utils::gf_to_bytes(code_word.slice(0));
    std::array<uint8_t, MAX_BLOCK_SIZE> correct_buffer;
    static_vector<uint8_t> correct_vec(correct_buffer.data(), MAX_BLOCK_SIZE, correct_data.size());
    std::copy_n(correct_data.begin(), correct_data.size(), correct_vec.begin());
    auto disk_result = _disk.write(block_index * _raw_block_size, correct_vec);

    return _extractMessage(code_word);
}

std::vector<std::uint8_t> ReedSolomonBlockDevice::_extractMessage(
    PolynomialGF256 endoced_polynomial)
{
    return gf256_utils::gf_to_bytes(
        endoced_polynomial.slice(2 * _correctable_bytes, _raw_block_size));
}

PolynomialGF256 ReedSolomonBlockDevice::_calculateGenerator()
{
    PolynomialGF256 g({ GF256(1) });
    GF256 alpha = GF256::getPrimitiveElement();

    GF256 power(alpha);
    for (int i = 0; i < 2 * _correctable_bytes; i++) {
        // (x - α^(i+1))
        PolynomialGF256 term({ power, GF256(1) });
        g = g * term;
        power = power * alpha;
    }

    return g;
}

std::vector<GF256> ReedSolomonBlockDevice::_forney(
    const PolynomialGF256& omega, PolynomialGF256& sigma, const std::vector<GF256>& error_locations)
{
    std::vector<GF256> error_values;
    PolynomialGF256 sigma_derivative = sigma.derivative();

    for (auto& X : error_locations) {
        GF256 Xi_inv = X.inv();
        GF256 numerator = omega.evaluate(Xi_inv);
        GF256 denominator = sigma_derivative.evaluate(Xi_inv);
        error_values.push_back(numerator / denominator);
    }

    return error_values;
}

PolynomialGF256 ReedSolomonBlockDevice::_calculateOmega(
    const std::vector<GF256>& syndromes, PolynomialGF256& sigma)
{
    PolynomialGF256 S(syndromes);
    auto omega = (S * sigma).slice(0, syndromes.size());
    return PolynomialGF256(omega);
}

PolynomialGF256 ReedSolomonBlockDevice::_berlekampMassey(const std::vector<GF256>& syndromes)
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

std::vector<GF256> ReedSolomonBlockDevice::_errorLocations(
    PolynomialGF256 error_location_polynomial)
{
    std::vector<GF256> error_locations;

    for (int i = 1; i <= 255; i++) {
        if (error_location_polynomial.evaluate(i) == 0) {
            error_locations.push_back(GF256(i).inv());
        }
    }

    return error_locations;
}
