#include "ecc_helpers/gf256.h"

namespace {
    constexpr std::array<uint8_t, 256> make_exp_table() {
        std::array<uint8_t, 256> exp{};
        uint16_t x = 1;
        for (size_t i = 0; i < 255; ++i) {
            exp[i] = static_cast<uint8_t>(x);
            x <<= 1;
            if (x & 0x100)
                x ^= GF256::PRIMITIVE_POLY;
        }
        exp[255] = exp[0];
        return exp;
    }

    constexpr std::array<uint8_t, 256> make_log_table(const std::array<uint8_t, 256>& exp) {
        std::array<uint8_t, 256> log{};
        for (auto& v : log) v = 0xFF;
        for (size_t i = 0; i < 255; ++i)
            log[exp[i]] = static_cast<uint8_t>(i);
        return log;
    }

    static constexpr auto EXP = make_exp_table();
    static constexpr auto LOG = make_log_table(EXP);
}

GF256::GF256() : value(0) {}

GF256::GF256(uint8_t v) : value(v) {}

GF256::GF256(std::byte b) : value(static_cast<uint8_t>(b)) {}

GF256 GF256::operator+(GF256 other) const {
    return GF256(value ^ other.value);
}

GF256 GF256::operator-(GF256 other) const {
    return GF256(value ^ other.value);
}

GF256 GF256::operator*(GF256 other) const {
    if (value == 0 || other.value == 0) return GF256(0);
    unsigned int log_sum = LOG[value] + LOG[other.value];
    log_sum &= 255;
    return GF256(EXP[log_sum]);
}

GF256 GF256::operator/(GF256 other) const {
    if (value == 0 || other.value == 0) return GF256(0);
    int log_diff = LOG[value] - LOG[other.value];
    if (log_diff < 0) log_diff += 255;
    return GF256(EXP[log_diff]);
}

GF256 GF256::inv(GF256 a) {
    if (a.value == 0) return 0;
    return EXP[255 - LOG[a.value]];
}

GF256 GF256::getPrimitiveElement() {
    return GF256(2);
}