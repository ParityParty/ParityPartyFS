#include "ecc_helpers/gf256.h"

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