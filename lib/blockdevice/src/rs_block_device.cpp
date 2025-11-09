#include "blockdevice/rs_block_device.hpp"
#include "ecc_helpers/gf256_utils.hpp"

#include<error.h>
#include <iostream>

std::vector<GF256> forney(const PolynomialGF256& omega,
                          PolynomialGF256& sigma,
                          const std::vector<GF256>& error_locations) {
    std::vector<GF256> error_values;
    PolynomialGF256 sigma_derivative = sigma.derivative(); // tylko nieparzyste potęgi

    for (auto& X : error_locations) {
        GF256 Xi_inv = X.inv();
        GF256 numerator = omega.evaluate(Xi_inv);
        GF256 denominator = sigma_derivative.evaluate(Xi_inv);
        error_values.push_back(numerator / denominator); // minus nie trzeba w GF(256)
    }

    return error_values;
}

PolynomialGF256 calculateOmega(const std::vector<GF256>& syndromes, PolynomialGF256& sigma) {
    PolynomialGF256 S(syndromes);
    auto omega = (S * sigma).slice(0, syndromes.size()); // mod x^(2t) -> slice(0, 2t)
    return PolynomialGF256(omega);
}


PolynomialGF256 berlekamp_massey(const std::vector<GF256>& syndromes) {
    PolynomialGF256 sigma({ GF256(1) });  // σ(x) = 1
    PolynomialGF256 B({ GF256(1) });      // kopia poprzedniego σ(x)
    GF256 b(1);

    int L = 0; // liczba błędów (stopień σ)
    int m = 1; // licznik kroków od ostatniej zmiany

    for (size_t n = 0; n < syndromes.size(); n++) {
        // 🔹 discrepancy d_n
        GF256 d = syndromes[n];
        for (int i = 1; i <= L; i++) {
            d = d+ sigma[i] * syndromes[n - i];
        }

        if (d != 0) {
            auto T = sigma;
            PolynomialGF256 diff = B * PolynomialGF256({d / b});
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

std::vector<GF256> errorLocations(PolynomialGF256 error_location_polynomial) {
    std::vector<GF256> error_locations;

    for(int i = 1; i <= 255; i++){
        if (error_location_polynomial.evaluate(i) == 0){
            error_locations.push_back(GF256(i).inv());
    }}

    return error_locations;
}


ReedSolomonBlockDevice::ReedSolomonBlockDevice(IDisk& disk) : _disk(disk) { 
    _generator = _calculateGenerator();
}

size_t ReedSolomonBlockDevice::numOfBlocks() const {
    return _disk.size() / BLOCK_LENGTH;
}

size_t ReedSolomonBlockDevice::dataSize() const {
    return MESSAGE_LENGTH;
}

size_t ReedSolomonBlockDevice::rawBlockSize() const {
    return BLOCK_LENGTH;
}

std::expected<void, DiskError> ReedSolomonBlockDevice::formatBlock(unsigned int block_index) {
    std::vector<std::byte> zero_data(BLOCK_LENGTH, std::byte(0));
    auto write_result = _disk.write(block_index * BLOCK_LENGTH, zero_data);
    return write_result.has_value() ? std::expected<void, DiskError>{} : std::unexpected(write_result.error());
}


std::expected<std::vector<std::byte>, DiskError> ReedSolomonBlockDevice::readBlock(
    DataLocation data_location, size_t bytes_to_read){
    
    bytes_to_read = std::min(MESSAGE_LENGTH - data_location.offset, bytes_to_read);
    auto raw_block = _disk.read(data_location.block_index * BLOCK_LENGTH, BLOCK_LENGTH);
    if(!raw_block.has_value()){
        return std::unexpected(raw_block.error());
    }

    auto decoded = _readAndFixBlock(raw_block.value());
    if(!decoded.has_value()){
        return std::unexpected(decoded.error());
    }

    auto decoded_data = decoded.value();
    return std::vector<std::byte>(decoded_data.begin() + data_location.offset, decoded_data.begin() + data_location.offset + bytes_to_read);
}

std::expected<size_t, DiskError> ReedSolomonBlockDevice::writeBlock(const std::vector<std::byte>& data, DataLocation data_location) {
    size_t to_write = std::min(data.size(), MESSAGE_LENGTH - data_location.offset);
    
    auto raw_block = _disk.read(BLOCK_LENGTH * data_location.block_index, BLOCK_LENGTH);
    if (!raw_block.has_value()) {
        return std::unexpected(raw_block.error());
    }

    auto decoding_res = _readAndFixBlock(raw_block.value());
    if (!decoding_res.has_value()){
        return std::unexpected(decoding_res.error());
    }

    auto decoded = decoding_res.value();

    std::copy(data.begin(), data.begin() + to_write, decoded.begin() + data_location.offset);
    
    auto new_encoded_block = _encodeBlock(decoded);
    
    auto disk_result = _disk.write(data_location.block_index * BLOCK_LENGTH, new_encoded_block);
    
    return to_write;
}

std::vector<std::byte> ReedSolomonBlockDevice::_encodeBlock(std::vector<std::byte> data){
    int t = BLOCK_LENGTH - MESSAGE_LENGTH;
    
    auto message = PolynomialGF256(gf256_utils::bytes_to_gf(data));
    auto shifted_message = message.multiply_by_xk(t);

    auto encoded = shifted_message + shifted_message.mod(_generator);

    GF256 alpha = GF256::getPrimitiveElement();
    GF256 power = GF256(alpha);
     for (int i = 0; i < BLOCK_LENGTH - MESSAGE_LENGTH; i++) {
        auto s = _generator.evaluate(power);
        if (s != 0)
        power = power * alpha;
    }

    return gf256_utils::gf_to_bytes(encoded.slice(0, BLOCK_LENGTH));

}

std::vector<std::byte> ReedSolomonBlockDevice::_extractMessage(PolynomialGF256 p){
    return gf256_utils::gf_to_bytes(p.slice(BLOCK_LENGTH - MESSAGE_LENGTH, BLOCK_LENGTH));
}


std::expected<std::vector<std::byte>, DiskError> 
ReedSolomonBlockDevice::_readAndFixBlock(std::vector<std::byte> raw_bytes) {
    using namespace gf256_utils;

    // === 1. Konwersja bajtów na elementy GF(256)
    auto code_word = PolynomialGF256(bytes_to_gf(raw_bytes));

    // === 2. Oblicz syndromy ===
    std::vector<GF256> syndromes;
    syndromes.reserve(BLOCK_LENGTH - MESSAGE_LENGTH);

    GF256 alpha = GF256::getPrimitiveElement();
    GF256 power = GF256(alpha);

    bool is_message_correct = true;
    for (int i = 0; i < BLOCK_LENGTH - MESSAGE_LENGTH; i++) {
        auto s = code_word.evaluate(power);
        syndromes.push_back(s);
        if (s != 0)
            is_message_correct = false;
        power = power * alpha;
    }

     //return _extractMessage(code_word);

    // === 3. Jeśli brak błędów, zwróć wiadomość ===
    if (is_message_correct)
        return _extractMessage(code_word);

    // === 4. Oblicz wielomian lokalizacji błędów σ(x) (error locator polynomial)
    // za pomocą np. algorytmu Berlekampa–Masseya
    auto sigma = berlekamp_massey(syndromes);


    // === 5. Znajdź pozycje błędów (roots of σ(x))
    auto error_positions = errorLocations(sigma);

    auto omega = calculateOmega(syndromes, sigma);

    auto error_values = forney(omega, sigma, error_positions);


    // === 6. Oblicz wielomian korekcji błędów (error evaluator)
    //auto z = calculateZ(syndromes, sigma);

    //return std::unexpected(DiskError::CorrectionError);

    // === 7. Wylicz wartości błędów (Forney’s algorithm)
    //auto error_values = calculateErrorValues(error_positions, z);

    // === 8. Popraw błędy w słowie kodowym
    for (size_t i = 0; i < error_positions.size(); i++) {
        auto pos = error_positions[i].log();
        
        
        code_word[pos] = code_word[pos] + error_values[i];

        std::cout << "Correcting error - at position:" <<(int)(uint8_t) pos << "error value" << (int)(uint8_t)error_values[i] << std::endl;
    }

    // === 9. Zwróć poprawioną wiadomość ===
    return _extractMessage(code_word);
}

std::vector<std::byte> _extractMessage(PolynomialGF256 endoced_polynomial) {
    return gf256_utils::gf_to_bytes(endoced_polynomial.slice(BLOCK_LENGTH - MESSAGE_LENGTH, BLOCK_LENGTH));
}


PolynomialGF256 ReedSolomonBlockDevice::_calculateGenerator() {
    PolynomialGF256 g({ GF256(1) });
    GF256 alpha = GF256::getPrimitiveElement();

    GF256 power(alpha);
    for (int i = 0; i < BLOCK_LENGTH - MESSAGE_LENGTH; i++) {
        // (x - α^(i+1))
        PolynomialGF256 term({ power, GF256(1) });
        g = g * term;
        power = power * alpha;
    }

    return g;
}





