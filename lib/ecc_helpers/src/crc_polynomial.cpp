#include "ecc_helpers/crc_polynomial.hpp"
#include "common/bit_helpers.hpp"
#include "common/static_vector.hpp"
#include <array>

unsigned int CrcPolynomial::_findDegree(unsigned long int coefficients)
{
    unsigned int counter = 0;
    while (coefficients > 0) {
        coefficients >>= 1;
        counter++;
    }

    // Counter counts terms, so we need to subtract one to get degree
    return counter - 1;
}

CrcPolynomial::CrcPolynomial(
    const std::vector<bool>& coefficients, unsigned int n, unsigned long int explicitPolynomial)
    : _coefficients(coefficients)
    , _n(n)
    , _explicitPolynomial(explicitPolynomial)
{
}

CrcPolynomial CrcPolynomial::MsgExplicit(unsigned long int polynomial)
{
    auto n = _findDegree(polynomial);

    std::array<bool, 64> poly_with_zeros_buffer;
    static_vector<bool> poly_with_zeros(poly_with_zeros_buffer.data(), 64);
    BitHelpers::ulongToBits(polynomial, poly_with_zeros);
    size_t start_idx = poly_with_zeros.size() - n - 1;
    std::vector<bool> poly(poly_with_zeros.begin() + start_idx, poly_with_zeros.end());

    return { poly, n, polynomial };
}

CrcPolynomial CrcPolynomial::MsgImplicit(unsigned long int polynomial)
{
    polynomial = (polynomial << 1) + 1;
    auto n = _findDegree(polynomial);

    std::array<bool, 64> poly_with_zeros_buffer;
    static_vector<bool> poly_with_zeros(poly_with_zeros_buffer.data(), 64);
    BitHelpers::ulongToBits(polynomial, poly_with_zeros);
    size_t start_idx = poly_with_zeros.size() - n - 1;
    std::vector<bool> poly(poly_with_zeros.begin() + start_idx, poly_with_zeros.end());

    return { poly, n, polynomial };
}

std::vector<bool> CrcPolynomial::divide(const std::vector<bool>& other)
{
    std::vector<bool> result(other.begin(), other.end());

    for (int i = 0; i < other.size() - _coefficients.size(); i++) {
        if (!result[i]) {
            continue;
        }
        for (int j = 0; j < _coefficients.size(); j++) {
            result[i + j] = result[i + j] ^ _coefficients[j];
        }
    }

    std::vector<bool> remainder(_n);
    for (int i = 0; i < _n; i++) {
        remainder[i] = result[result.size() - _n + i];
    }
    return remainder;
}
std::vector<bool> CrcPolynomial::getCoefficients() const
{
    return { _coefficients.begin(), _coefficients.end() };
}

unsigned int CrcPolynomial::getDegree() const { return _n; }

unsigned long int CrcPolynomial::getExplicitPolynomial() const { return _explicitPolynomial; }