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

    constexpr GF256 operator+(GF256 other) const { return GF256(value ^ other.value); }
    constexpr GF256 operator-(GF256 other) const { return GF256(value ^ other.value); }

    GF256 operator*(GF256 other) const;
    GF256 operator/(GF256 other) const;

    GF256& operator+=(GF256 other){ value ^= other.value; return *this; }
    GF256& operator-=(GF256 other){ value ^= other.value; return *this; }
    GF256& operator*=(GF256 other){ *this = *this * other; return *this; }
    GF256& operator/=(GF256 other){ *this = *this / other; return *this; }

    static GF256 inv(GF256 a);

private:
    uint8_t value;

public:
    static inline constexpr std::array<uint8_t, 256> EXP = []() consteval {
        std::array<uint8_t, 256> exp{};
        uint16_t x = 1;
        for (size_t i = 0; i < 255; ++i) {
            exp[i] = static_cast<uint8_t>(x);
            x <<= 1;
            if (x & 0x100) x ^= PRIMITIVE_POLY;
        }
        return exp;
    }();

    static inline constexpr std::array<uint8_t, 256> LOG = []() consteval {
        std::array<uint8_t, 256> log{};
        for (auto &v : log) v = 0xFF;
        for (size_t i = 0; i < 255; ++i)
            log[EXP[i]] = static_cast<uint8_t>(i);
        return log;
    }();
};
