#pragma once
#include "blockdevice/ecc_type.hpp"
#include "common/types.hpp"
#include "ecc_helpers/crc_polynomial.hpp"

enum class OpenMode : std::uint8_t {
    /** Cursor at the beginning, moves with read/write operations */
    Normal = 0,
    /** Cursor always at file end; read and seek fail */
    Append = 1 << 0,
    /** Truncates file on open */
    Truncate = 1 << 1,
    /** Fails if file already open; exclusive access */
    Exclusive = 1 << 2,
    /** Read-only protection; fails if file already open without protection */
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

struct FileStat {
    /** Size of the file in bytes */
    std::uint32_t size = 0;

    /** Number of entries in the directory. Only valid for directories. */
    std::uint32_t number_of_entries = 0;

    /** Whether the file is a directory */
    bool is_directory = false;
};

/**
 * Configuration parameters for filesystem initialization.
 */
struct FsConfig {
    /** Total size of the filesystem in bytes. Has to be multiple of block_size. */
    std::uint64_t total_size = 0;

    /** Expected average file size in bytes. Used for calculating filesystem parameters. */
    std::uint64_t average_file_size = 0;

    /** Block size in bytes. Must be a power of two. */
    std::uint32_t block_size = 512;

    /** Error correction type. */
    ECCType ecc_type = ECCType::None;

    /** If ECC type is CRC, polynomial used for cyclic redundancy check.
     * The default polynomial guarantees error detection up to 5 bit flips in messages up to 30 000
     * bits according to CrcZoo */
    CrcPolynomial crc_polynomial = CrcPolynomial::MsgImplicit(0x9960034c);

    /** If ECC type is Reed-Solomon, number of correctable bytes. */
    std::uint32_t rs_correctable_bytes = 3;

    /** Enable journaling */
    bool use_journal = false;
};
