#pragma once
#include <vector>
#include "gf256.hpp"

class PolynomialGF256 {
public:
    PolynomialGF256() = default;
    explicit PolynomialGF256(const std::vector<GF256>& coeffs);

    PolynomialGF256 operator+(const PolynomialGF256& other) const;
    PolynomialGF256 operator*(const PolynomialGF256& other) const;
    PolynomialGF256& operator+=(const PolynomialGF256& other);
    PolynomialGF256& operator*=(const PolynomialGF256& other);

    PolynomialGF256 multiply_by_xk(size_t k) const;

    PolynomialGF256 mod(const PolynomialGF256& divisor) const;

    GF256 evaluate(GF256 x) const;

    std::vector<GF256> slice(size_t from, size_t to) const;

private:
    std::vector<GF256> coeffs;

    void trim();
};
