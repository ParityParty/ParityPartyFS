#pragma once
#include "gf256.hpp"
#include "ppfs/common/static_vector.hpp"
#include <array>
#include <iostream>

#define MAX_GF256_POLYNOMIAL_SIZE 256
/**
 * Represents a polynomial over the finite field GF(256), used for Reed-Solomon error correction.
 */
class PolynomialGF256 {
public:
    PolynomialGF256() = default;
    explicit PolynomialGF256(const static_vector<GF256>& coeffs);
    PolynomialGF256(std::initializer_list<GF256> coeffs);

    PolynomialGF256 operator+(const PolynomialGF256& other) const;
    PolynomialGF256 operator*(const PolynomialGF256& other) const;
    PolynomialGF256& operator+=(const PolynomialGF256& other);
    PolynomialGF256& operator*=(const PolynomialGF256& other);

    GF256 operator[](size_t i) const;
    GF256& operator[](size_t i);

    PolynomialGF256 multiply_by_xk(size_t k) const;

    PolynomialGF256 mod(const PolynomialGF256& divisor) const;

    GF256 evaluate(GF256 x) const;

    void slice(size_t from, size_t to, static_vector<GF256>& result) const;
    void slice(size_t from, static_vector<GF256>& result) const;

    size_t degree() const;

    void print(std::ostream& os = std::cout) const;

    PolynomialGF256 derivative() const;

    size_t size() const { return _size; }

private:
    std::array<GF256, MAX_GF256_POLYNOMIAL_SIZE> coeffs;
    size_t _size;

    void trim();
};
