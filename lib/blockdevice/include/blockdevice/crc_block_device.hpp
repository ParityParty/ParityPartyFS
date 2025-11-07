#pragma once

#include "blockdevice/iblock_device.hpp"

/**
 * Block device with customizable crc error detection and correction
 *
 * Everybody is invited to Parity Party!
 */
class CrcBlockDevice : public IBlockDevice {
    unsigned long int _polynomial;
    unsigned int _n;

public:
    /**
     * Create CrcBlockDevice with specified polynomial
     *
     * At the party there is enough error correction for everybody, unfortunately there isn't
     * enough generator polynomials, so we had to enforce a rule: Bring your own polynomial to
     * Parity Party! If you forgot yours, ask around, maybe somebody have a *redundant* polynomials.
     *
     * @param polynomial polynomial used for crc with most significant bit first with explicit +1
     */
    CrcBlockDevice(unsigned long int polynomial);
};