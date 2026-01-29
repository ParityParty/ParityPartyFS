#pragma once
#include "ppfs/common/static_vector.hpp"

#include <array>

#define MAX_CRC_POLYNOMIAL_SIZE 65

/**
 * Class representing polynomial used in crc error detection
 */
class CrcPolynomial {
    /**
     * Coefficients with most significant bit first, with explicit +1
     */
    std::array<bool, MAX_CRC_POLYNOMIAL_SIZE> _coefficients;
    unsigned int _n;
    unsigned long int _explicitPolynomial;

    /**
     *
     * @param coefficients Polynomial with explicit +1
     * @return Degree of the polynomial
     */
    static unsigned int _findDegree(unsigned long int coefficients);

    CrcPolynomial(const std::array<bool, MAX_CRC_POLYNOMIAL_SIZE>& coefficients, unsigned int n,
        unsigned long int explicitPolynomial);

public:
    CrcPolynomial() = delete;
    /**
     * Create polynomial
     *
     * @param polynomial polynomial with most significant bit first, with explicit +1
     * @return Polynomial
     */
    static CrcPolynomial MsgExplicit(unsigned long int polynomial);

    /**
     * Create polynomial
     *
     * @param polynomial polynomial with most significant bit first, with implicit +1
     * @return Polynomial
     */
    static CrcPolynomial MsgImplicit(unsigned long int polynomial);

    /**
     * Divide long polynomial by self
     *
     * @param other Coefficients of other polynomial
     * @return remainder after division other/self
     */
    void divide(const static_vector<bool>& other, static_vector<bool>& remainder);

    void getCoefficients(static_vector<bool>& coeffs) const;
    unsigned int getDegree() const;
    unsigned long int getExplicitPolynomial() const;
};