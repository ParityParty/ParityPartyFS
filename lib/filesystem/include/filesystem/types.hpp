#pragma once
#include "blockdevice/ecc_type.hpp"
#include "common/types.hpp"
#include "ecc_helpers/crc_polynomial.hpp"

enum class OpenMode : std::uint8_t {
    // Cursor at the beginning of the file, moves as data is read/written.
    Normal = 0,

    // Cursor always at the end of the file. Read and seek operations fail. Cannot be combined with
    // Protected.
    Append = 1 << 0,

    // Truncates the file upon opening. Cannot be combined with Protected.
    Truncate = 1 << 1,

    // Fails if the file is already open. When the file is open as exclusive, all other opens will
    // fail.
    Exclusive = 1 << 2,

    // Ensures that data won't be changed when open. Fails if the file is already open without this
    // flag. When the file is open as protected, all non-protected opens will fail.
    Protected = 1 << 3,
};

inline constexpr OpenMode operator|(OpenMode lhs, OpenMode rhs)
{
    return static_cast<OpenMode>(static_cast<std::uint8_t>(lhs) | static_cast<std::uint8_t>(rhs));
}
inline constexpr bool operator&(OpenMode lhs, OpenMode rhs)
{
    return (static_cast<std::uint8_t>(lhs) & static_cast<std::uint8_t>(rhs)) != 0;
}
inline OpenMode& operator|=(OpenMode& lhs, OpenMode rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

struct FsConfig {
    // Total size of the filesystem in bytes. Has to be multiple of block_size.
    std::uint64_t total_size = 0;

    // Expected average file size in bytes. Used for calculating filesystem parameters.
    std::uint64_t average_file_size = 0;

    // Block size in bytes. Must be a power of two.
    std::uint32_t block_size = 512;

    // Error correction type.
    ECCType ecc_type = ECCType::None;

    // If ECC type is CRC, polynomial used for cyclic redundancy check.
    //
    // The default polynomial guarantees error detection up to 5 bit flips in messages up to 30 000
    // bits according to CrcZoo
    CrcPolynomial crc_polynomial = CrcPolynomial::MsgImplicit(0x9960034c);

    // If ECC type is Reed-Solomon, number of correctable bytes.
    std::uint32_t rs_correctable_bytes = 3;

    bool use_journal = false;
};
