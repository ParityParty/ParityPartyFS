#include "ecc_helpers/polynomial_gf256.hpp"
#include <algorithm>

PolynomialGF256::PolynomialGF256(const std::vector<GF256>& coeffs)
    : coeffs(coeffs) {
    trim();
}

void PolynomialGF256::trim() {
    while (!coeffs.empty() && static_cast<uint8_t>(coeffs.back()) == 0)
        coeffs.pop_back();
}

PolynomialGF256 PolynomialGF256::operator+(const PolynomialGF256& other) const {
    size_t n = std::max(coeffs.size(), other.coeffs.size());
    std::vector<GF256> result(n, GF256(0));
    for (size_t i = 0; i < n; ++i) {
        GF256 a = (i < coeffs.size()) ? coeffs[i] : GF256(0);
        GF256 b = (i < other.coeffs.size()) ? other.coeffs[i] : GF256(0);
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
    if (coeffs.empty() || other.coeffs.empty())
        return PolynomialGF256();

    std::vector<GF256> result(coeffs.size() + other.coeffs.size() - 1, GF256(0));

    for (size_t i = 0; i < coeffs.size(); ++i) {
        for (size_t j = 0; j < other.coeffs.size(); ++j) {
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
    if (i < coeffs.size()) return coeffs[i];
    return GF256(0);
}

GF256& PolynomialGF256::operator[](size_t i) {
    if (i >= coeffs.size())
        coeffs.resize(i+1, GF256(0));
    return coeffs[i];
}

PolynomialGF256 PolynomialGF256::multiply_by_xk(size_t k) const {
    std::vector<GF256> result(k, GF256(0));
    result.insert(result.end(), coeffs.begin(), coeffs.end());
    return PolynomialGF256(result);
}

PolynomialGF256 PolynomialGF256::mod(const PolynomialGF256& divisor) const {
    if (divisor.coeffs.empty())
        return *this;

    PolynomialGF256 remainder(*this);
    size_t divisor_degree = divisor.coeffs.size() - 1;
    GF256 divisor_lead = divisor.coeffs.back();

    while (remainder.coeffs.size() >= divisor.coeffs.size()) {
        size_t shift = remainder.coeffs.size() - divisor.coeffs.size();
        GF256 factor = remainder.coeffs.back() / divisor_lead;

        std::vector<GF256> temp(shift, GF256(0));
        for (const auto& c : divisor.coeffs)
            temp.push_back(c * factor);

        PolynomialGF256 sub(temp);
        remainder += sub; // w GF(256) to XOR, więc += jest okej
        remainder.trim();
    }
    return remainder;
}


GF256 PolynomialGF256::evaluate(GF256 x) const {
    GF256 result(0);
    GF256 power(1); // x^0 = 1 na start
    for (const auto& c : coeffs) {
        result = c * power + result;
        power = x * power;
    }
    return result;
}

std::vector<GF256> PolynomialGF256::slice(size_t from, size_t to) const {
    std::vector<GF256> res;
    for (size_t i = from; i < to; ++i) {
        if (i < coeffs.size())
            res.push_back(coeffs[i]);
        else
            res.push_back(GF256(0));
    }
    return res;
}

std::vector<GF256> PolynomialGF256::slice(size_t from) const {
    std::vector<GF256> res;
    for (size_t i = from; i < coeffs.size(); ++i)
            res.push_back(coeffs[i]);
    return res;
}

size_t PolynomialGF256::degree(){
    trim();
    return coeffs.size();
}

void PolynomialGF256::print(std::ostream& os) const {
    bool first = true;
    for (size_t i = 0; i < coeffs.size(); ++i) {
        GF256 c = coeffs[i];
        if (c != GF256(0)) {
            if (!first) os << " + ";
            os << static_cast<int>(uint8_t(c));
            if (i > 0) os << "x";
            if (i > 1) os << "^" << i;
            first = false;
        }
    }
    if (first) os << "0";  // jeśli wszystkie współczynniki = 0
    os << "\n";
}

PolynomialGF256 PolynomialGF256::derivative() {
    std::vector<GF256> deriv;
    deriv.reserve(coeffs.size());
    for (size_t i = 1; i < coeffs.size(); i ++) {  // tylko nieparzyste potęgi
        if(i % 2)
            deriv.push_back(coeffs[i]);
        else 
            deriv.push_back(0);
    }
    return PolynomialGF256(deriv);
}