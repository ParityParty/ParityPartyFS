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

    constexpr GF256() : value(0) {}
    constexpr GF256(uint8_t v) : value(v) {}
    constexpr GF256(std::byte b) : value(static_cast<uint8_t>(b)) {}

    explicit operator std::byte() const { return static_cast<std::byte>(value); }
    explicit operator uint8_t() const { return value; }

    constexpr GF256 operator+(GF256 other) const;
    constexpr GF256 operator-(GF256 other) const;

    GF256 operator*(GF256 other) const;
    GF256 operator/(GF256 other) const;

    GF256 operator-() const;

    static GF256 inv(GF256 a);

    static GF256 getPrimitiveElement();

private:
    uint8_t value;
};
