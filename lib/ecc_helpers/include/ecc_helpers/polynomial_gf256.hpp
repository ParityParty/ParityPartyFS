#pragma once
#include "gf256.hpp"
#include <vector>

/**
 * Represents a polynomial over the finite field GF(256), used for Reed-Solomon error correction.
 */
class PolynomialGF256 {
public:
    PolynomialGF256() = default;
    explicit PolynomialGF256(const std::vector<GF256>& coeffs);

    PolynomialGF256 operator+(const PolynomialGF256& other) const;
    PolynomialGF256 operator*(const PolynomialGF256& other) const;
    PolynomialGF256& operator+=(const PolynomialGF256& other);
    PolynomialGF256& operator*=(const PolynomialGF256& other);

    GF256 operator[](size_t i) const;
    GF256& operator[](size_t i);

    PolynomialGF256 multiply_by_xk(size_t k) const;

    PolynomialGF256 mod(const PolynomialGF256& divisor) const;

    GF256 evaluate(GF256 x) const;

    std::vector<GF256> slice(size_t from, size_t to) const;
    std::vector<GF256> slice(size_t from) const;

    size_t degree();

    void print(std::ostream& os = std::cout) const;

    PolynomialGF256 derivative();

private:
    std::vector<GF256> coeffs;

    void trim();
};
