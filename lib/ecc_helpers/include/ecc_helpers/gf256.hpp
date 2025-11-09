#pragma once

#include <cstdint>
#include <cstddef>
/**
 * Represents an element of the finite field GF(256), used in Reed-Solomon error correction.
 * 
 * This class encapsulates arithmetic in GF(2^8) with a fixed primitive polynomial.
 * It supports addition, subtraction, multiplication, division, inversion, and logarithms
 * of field elements.
 */
class GF256 {
public:
    static constexpr uint16_t PRIMITIVE_POLY = 0x11D;

    GF256();
    GF256(uint8_t v);
    GF256(std::byte b);

    explicit operator std::byte() const;
    explicit operator uint8_t() const;

    GF256 operator+(GF256 other) const;
    GF256 operator-(GF256 other) const;

    GF256 operator*(GF256 other) const;
    GF256 operator/(GF256 other) const;

    GF256 operator-() const;

    bool operator==(const GF256 other) const;
    bool operator!=(const GF256 other) const;

    uint8_t log() const;

    GF256 inv() const;

    static GF256 getPrimitiveElement();

private:
    uint8_t value;
};
