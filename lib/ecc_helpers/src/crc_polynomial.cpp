#include "ecc_helpers/crc_polynomial.hpp"
#include "blockdevice/iblock_device.hpp"
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

CrcPolynomial::CrcPolynomial(const std::array<bool, MAX_POLYNOMIAL_DEGREE>& coefficients,
    unsigned int n, unsigned long int explicitPolynomial)
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
    std::array<bool, MAX_POLYNOMIAL_DEGREE> poly {};
    std::copy(poly_with_zeros.begin() + start_idx,
        poly_with_zeros.end(), poly.begin());

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
    std::array<bool, MAX_POLYNOMIAL_DEGREE> poly {};
    std::copy(poly_with_zeros.begin() + start_idx,
        poly_with_zeros.end(), poly.begin());

    return { poly, n, polynomial };
}

void CrcPolynomial::divide(const static_vector<bool>& other, static_vector<bool>& remainder)
{
    std::array<bool, MAX_BLOCK_SIZE * 8> result_buf;
    static_vector<bool> result(result_buf.data(), MAX_BLOCK_SIZE * 8, other.size());
    // Copy other into result
    std::copy(other.begin(), other.end(), result.begin());

    for (int i = 0; i < other.size() - (_n + 1); i++) {
        if (!result[i]) {
            continue;
        }
        for (int j = 0; j < _n + 1; j++) {
            result[i + j] = result[i + j] ^ _coefficients[j];
        }
    }

    remainder.resize(_n);
    for (int i = 0; i < _n; i++) {
        remainder[i] = result[result.size() - _n + i];
    }
}
void CrcPolynomial::getCoefficients(static_vector<bool>& coeffs) const
{
    coeffs.resize(_n + 1);
    std::copy(_coefficients.begin(), _coefficients.begin() + _n + 1, coeffs.begin());
}

unsigned int CrcPolynomial::getDegree() const { return _n; }

unsigned long int CrcPolynomial::getExplicitPolynomial() const { return _explicitPolynomial; }