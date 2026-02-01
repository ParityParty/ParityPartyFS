#include "ppfs/filesystem/fs_config_helpers.hpp"
#include "ppfs/common/types.hpp"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <fstream>
#include <string>

std::string trim(const std::string& s)
{
    size_t first = s.find_first_not_of(" \t\n\r");
    if (first == std::string::npos)
        return "";
    size_t last = s.find_last_not_of(" \t\n\r");
    return s.substr(first, (last - first + 1));
}

std::expected<FsConfig, FsError> load_fs_config(std::string_view path)
{
    std::ifstream file(path.data());
    if (!file.is_open()) {
        return std::unexpected(FsError::Config_IOError);
    }

    FsConfig cfg;
    std::string line;

    struct {
        bool total_size = false, average_file_size = false, block_size = false;
        bool ecc_type = false, crc_polynomial = false, rs_correctable_bytes = false;
    } seen;

    while (std::getline(file, line)) {
        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }
        size_t slash_pos = line.find("//");
        if (slash_pos != std::string::npos) {
            line = line.substr(0, slash_pos);
        }

        line = trim(line);
        if (line.empty())
            continue;

        auto eq = line.find('=');
        if (eq == std::string::npos)
            return std::unexpected(FsError::Config_SyntaxError);

        std::string key = trim(line.substr(0, eq));
        std::string value = trim(line.substr(eq + 1));

        try {
            if (key == "total_size") {
                seen.total_size = true;
                cfg.total_size = std::stoull(value);
            } else if (key == "average_file_size") {
                seen.average_file_size = true;
                cfg.average_file_size = std::stoull(value);
            } else if (key == "block_size") {
                seen.block_size = true;
                cfg.block_size = std::stoul(value);
            } else if (key == "rs_correctable_bytes") {
                seen.rs_correctable_bytes = true;
                cfg.rs_correctable_bytes = std::stoul(value);
            } else if (key == "use_journal") {
                cfg.use_journal = (value == "true" || value == "1");
            } else if (key == "ecc_type") {
                seen.ecc_type = true;
                if (value == "none")
                    cfg.ecc_type = ECCType::None;
                else if (value == "crc")
                    cfg.ecc_type = ECCType::Crc;
                else if (value == "reed_solomon")
                    cfg.ecc_type = ECCType::ReedSolomon;
                else if (value == "parity")
                    cfg.ecc_type = ECCType::Parity;
                else if (value == "hamming")
                    cfg.ecc_type = ECCType::Hamming;
                else
                    return std::unexpected(FsError::Config_InvalidValue);
            } else if (key == "crc_polynomial") {
                seen.crc_polynomial = true;
                cfg.crc_polynomial = CrcPolynomial::MsgImplicit(std::stoull(value, nullptr, 0));
            } else {
                return std::unexpected(FsError::Config_UnknownKey);
            }
        } catch (const std::invalid_argument& e) {
            return std::unexpected(FsError::Config_InvalidValue);
        } catch (const std::out_of_range& e) {
            return std::unexpected(FsError::Config_InvalidValue);
        }
    }

    if (!seen.total_size || !seen.block_size || !seen.average_file_size || !seen.ecc_type)
        return std::unexpected(FsError::Config_MissingField);

    if (cfg.ecc_type == ECCType::Crc && !seen.crc_polynomial)
        return std::unexpected(FsError::Config_MissingField);

    if (cfg.ecc_type == ECCType::ReedSolomon && !seen.rs_correctable_bytes)
        return std::unexpected(FsError::Config_MissingField);

    return cfg;
}

void print_fs_config_usage(std::ostream& os)
{
    os << "# Example FsConfig file\n"
          "# Lines starting with '#' or '//' are ignored\n\n"

          "# ---------------- numeric fields ----------------\n"
          "total_size = 4194304            # uint64_t: total size of filesystem in bytes, must be "
          "a multiple of block_size\n"
          "average_file_size = 256         # uint64_t: expected average file size in bytes\n"
          "block_size = 128                # uint32_t: block size in bytes (must be a power of "
          "two)\n"
          "rs_correctable_bytes = 3        # uint32_t: required if ecc_type=reed_solomon\n\n"
          "crc_polynomial = 0x9960034c     # unsigned long int: required if ecc_type=crc, can be "
          "decimal or hexadecimal (0x...)\n\n"

          "# ---------------- boolean fields ----------------\n"
          "use_journal = false             # bool: enable journaling (true or false, default: "
          "false)\n\n"

          "# ---------------- enum fields ----------------\n"
          "ecc_type = crc                  # ECCType: none | crc | reed_solomon | parity | "
          "hamming\n";
}
