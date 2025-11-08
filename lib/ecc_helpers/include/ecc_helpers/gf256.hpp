#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include <cstddef>
#include <cstdint>
#include <cstddef>
#include <cstddef>

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

    bool operator==(const uint8_t other) const;
    bool operator!=(const uint8_t other) const;

    bool operator==(const GF256 other) const;

    uint8_t log();

    static GF256 inv(GF256 a);

    static GF256 getPrimitiveElement();

private:
    uint8_t value;
};
