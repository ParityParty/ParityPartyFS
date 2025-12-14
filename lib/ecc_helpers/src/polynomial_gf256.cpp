#include "ecc_helpers/polynomial_gf256.hpp"
#include <algorithm>

PolynomialGF256::PolynomialGF256(const static_vector<GF256>& coeffs_input)
    : _size(coeffs_input.size()) {
    std::copy(coeffs_input.begin(), coeffs_input.end(), this->coeffs.begin());
    trim();
}

PolynomialGF256::PolynomialGF256(std::initializer_list<GF256> init)
    : _size(std::min(init.size(), static_cast<size_t>(MAX_GF256_POLYNOMIAL_SIZE))) {
    std::copy(init.begin(), init.begin() + _size, coeffs.begin());
    trim();
}

void PolynomialGF256::trim() {
    while (_size > 0 && static_cast<uint8_t>(coeffs[_size - 1]) == 0)
        _size--;
}

PolynomialGF256 PolynomialGF256::operator+(const PolynomialGF256& other) const {
    size_t n = std::max(_size, other._size);
    std::array<GF256, MAX_GF256_POLYNOMIAL_SIZE> result_buffer;
    static_vector<GF256> result(result_buffer.data(), MAX_GF256_POLYNOMIAL_SIZE, n);
    for (size_t i = 0; i < n; ++i) {
        GF256 a = (i < _size) ? coeffs[i] : GF256(0);
        GF256 b = (i < other._size) ? other.coeffs[i] : GF256(0);
        result[i] = a + b;
    }

    auto p = PolynomialGF256(result);
    p.trim();
    return p;
}

PolynomialGF256& PolynomialGF256::operator+=(const PolynomialGF256& other) {
    *this = *this + other;
    return *this;
}

PolynomialGF256 PolynomialGF256::operator*(const PolynomialGF256& other) const {
    if (_size == 0 || other._size == 0)
        return PolynomialGF256();

    size_t result_size = _size + other._size - 1;
    std::array<GF256, MAX_GF256_POLYNOMIAL_SIZE> result_buffer;
    static_vector<GF256> result(result_buffer.data(), MAX_GF256_POLYNOMIAL_SIZE, result_size);
    std::fill(result.begin(), result.end(), GF256(0));

    for (size_t i = 0; i < _size; ++i) {
        for (size_t j = 0; j < other._size; ++j) {
            result[i + j] = result[i + j] + coeffs[i] * other.coeffs[j];
        }
    }

    auto p = PolynomialGF256(result);
    p.trim();
    return p;
}

PolynomialGF256& PolynomialGF256::operator*=(const PolynomialGF256& other) {
    *this = *this * other;
    return *this;
}

GF256 PolynomialGF256::operator[](size_t i) const {
    if (i < _size) return coeffs[i];
    return GF256(0);
}

GF256& PolynomialGF256::operator[](size_t i) {
    if (i >= _size) {
        if (i >= MAX_GF256_POLYNOMIAL_SIZE)
            throw std::out_of_range("Polynomial index out of range");
        for (size_t j = _size; j <= i; ++j)
            coeffs[j] = GF256(0);
        _size = i + 1;
    }
    return coeffs[i];
}

PolynomialGF256 PolynomialGF256::multiply_by_xk(size_t k) const {
    std::array<GF256, MAX_GF256_POLYNOMIAL_SIZE> result_buffer;
    static_vector<GF256> result(result_buffer.data(), MAX_GF256_POLYNOMIAL_SIZE, k + _size);
    for (size_t i = 0; i < k; ++i)
        result[i] = GF256(0);
    for (size_t i = 0; i < _size; ++i)
        result[k + i] = coeffs[i];
    return PolynomialGF256(result);
}

PolynomialGF256 PolynomialGF256::mod(const PolynomialGF256& divisor) const {
    if (divisor._size == 0)
        return *this;

    PolynomialGF256 remainder(*this);
    size_t divisor_degree = divisor._size - 1;
    GF256 divisor_lead = divisor.coeffs[divisor._size - 1];

    while (remainder._size >= divisor._size) {
        size_t shift = remainder._size - divisor._size;
        GF256 factor = remainder.coeffs[remainder._size - 1] / divisor_lead;

        std::array<GF256, MAX_GF256_POLYNOMIAL_SIZE> temp_buffer;
        static_vector<GF256> temp(temp_buffer.data(), MAX_GF256_POLYNOMIAL_SIZE, shift + divisor._size);
        for (size_t i = 0; i < shift; ++i)
            temp[i] = GF256(0);
        for (size_t i = 0; i < divisor._size; ++i)
            temp[shift + i] = divisor.coeffs[i] * factor;

        PolynomialGF256 sub(temp);
        remainder += sub;
        remainder.trim();
    }
    return remainder;
}


GF256 PolynomialGF256::evaluate(GF256 x) const {
    GF256 result(0);
    GF256 power(1);
    for (size_t i = 0; i < _size; ++i) {
        result = coeffs[i] * power + result;
        power = x * power;
    }
    return result;
}

void PolynomialGF256::slice(size_t from, size_t to, static_vector<GF256>& result) const {
    size_t count = to - from;
    result.resize(count);
    for (size_t i = 0; i < count; ++i) {
        size_t idx = from + i;
        if (idx < _size)
            result[i] = coeffs[idx];
        else
            result[i] = GF256(0);
    }
}

void PolynomialGF256::slice(size_t from, static_vector<GF256>& result) const {
    size_t count = _size > from ? _size - from : 0;
    result.resize(count);
    for (size_t i = 0; i < count; ++i) {
        result[i] = coeffs[from + i];
    }
}

size_t PolynomialGF256::degree() const {
    const_cast<PolynomialGF256*>(this)->trim();
    return _size == 0 ? 0 : _size - 1;
}

void PolynomialGF256::print(std::ostream& os) const {
    bool first = true;
    for (size_t i = 0; i < _size; ++i) {
        GF256 c = coeffs[i];
        if (c != GF256(0)) {
            if (!first) os << " + ";
            os << static_cast<int>(uint8_t(c));
            if (i > 0) os << "x";
            if (i > 1) os << "^" << i;
            first = false;
        }
    }
    if (first) os << "0";
    os << "\n";
}

PolynomialGF256 PolynomialGF256::derivative() const {
    size_t deriv_size = _size > 0 ? _size - 1 : 0;
    std::array<GF256, MAX_GF256_POLYNOMIAL_SIZE> deriv_buffer;
    static_vector<GF256> deriv(deriv_buffer.data(), MAX_GF256_POLYNOMIAL_SIZE, deriv_size);
    for (size_t i = 1; i < _size; i++) {
        if (i % 2)
            deriv[i - 1] = coeffs[i];
        else
            deriv[i - 1] = GF256(0);
    }
    return PolynomialGF256(deriv);
}