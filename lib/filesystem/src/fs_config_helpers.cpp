#include "filesystem/fs_config_helpers.hpp"
#include "common/types.hpp"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <fstream>
#include <string>

// ==================== helpers ====================

static void trim(std::string& s)
{
    auto not_space = [](int ch) { return !std::isspace(ch); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
    s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
}

static bool parse_u64(std::string_view s, uint64_t& out)
{
    auto res = std::from_chars(s.data(), s.data() + s.size(), out, 0);
    return res.ec == std::errc {} && res.ptr == s.data() + s.size();
}

static bool parse_u32(std::string_view s, uint32_t& out)
{
    uint64_t tmp;
    if (!parse_u64(s, tmp) || tmp > 0xFFFFFFFF)
        return false;
    out = static_cast<uint32_t>(tmp);
    return true;
}

static bool parse_bool(std::string_view s, bool& out)
{
    if (s == "true" || s == "1") {
        out = true;
        return true;
    }
    if (s == "false" || s == "0") {
        out = false;
        return true;
    }
    return false;
}

// ==================== loader ====================

// Pomocnicza funkcja trim (nadal potrzebna, bo std::string nie ma jej wbudowanej)
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
    if (!file.is_open())
        return std::unexpected(FsError::Config_IOError);

    FsConfig cfg;
    std::string line;

    struct {
        bool total_size = false, average_file_size = false, block_size = false;
        bool ecc_type = false, crc_polynomial = false, rs_correctable_bytes = false;
    } seen;

    while (std::getline(file, line)) {
        if (line.empty() || line.starts_with("#") || line.starts_with("//"))
            continue;

        auto eq = line.find('=');
        if (eq == std::string::npos)
            return std::unexpected(FsError::Config_SyntaxError);

        std::string key = trim(line.substr(0, eq));
        std::string value = trim(line.substr(eq + 1));

        try {
            if (key == "total_size") {
                seen.total_size = true;
                cfg.total_size = std::stoull(value); // string to unsigned long long
            } else if (key == "average_file_size") {
                seen.average_file_size = true;
                cfg.average_file_size = std::stoull(value);
            } else if (key == "block_size") {
                seen.block_size = true;
                cfg.block_size = std::stoul(value); // string to unsigned long
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
                else
                    return std::unexpected(FsError::Config_InvalidValue);
            } else if (key == "crc_polynomial") {
                seen.crc_polynomial = true;
                cfg.crc_polynomial = CrcPolynomial::MsgImplicit(std::stoull(value, nullptr, 0));
                // Użycie 0 jako podstawy pozwala na auto-detekcję (np. 0x dla hex)
            } else {
                return std::unexpected(FsError::Config_UnknownKey);
            }
        } catch (const std::invalid_argument& e) {
            return std::unexpected(FsError::Config_InvalidValue);
        } catch (const std::out_of_range& e) {
            return std::unexpected(FsError::Config_InvalidValue);
        }
    }

    // Walidacja pól wymaganych
    if (!seen.total_size || !seen.block_size || !seen.average_file_size || !seen.ecc_type)
        return std::unexpected(FsError::Config_MissingField);

    if (cfg.ecc_type == ECCType::Crc && !seen.crc_polynomial)
        return std::unexpected(FsError::Config_MissingField);

    if (cfg.ecc_type == ECCType::ReedSolomon && !seen.rs_correctable_bytes)
        return std::unexpected(FsError::Config_MissingField);

    return cfg;
}

// ==================== usage ====================
void print_fs_config_usage(std::ostream& os)
{
    os << "# Example FsConfig file\n"
          "# Lines starting with '#' or '//' are ignored\n\n"

          "# ---------------- numeric fields ----------------\n"
          "total_size = 1048576            # uint64_t: total size of filesystem in bytes\n"
          "average_file_size = 4096        # uint64_t: expected average file size in bytes\n"
          "block_size = 512                # uint32_t: block size in bytes (power of two)\n"
          "rs_correctable_bytes = 3        # uint32_t: required if ecc_type=reed_solomon\n\n"
          "crc_polynomial = 0x9960034c     # unsigned long int: required if ecc_type=crc, can be "
          "decimal or hexadecimal (0x...)\n\n"

          "# ---------------- boolean fields ----------------\n"
          "use_journal = false             # bool: enable journaling (true or false)\n\n"

          "# ---------------- enum fields ----------------\n"
          "ecc_type = none                  # ECCType: none | crc | reed_solomon\n"

          "# ---------------- notes ----------------\n"
          "# All fields must be specified.\n"
          "# Example usage:\n"
          "#   total_size = 1048576\n"
          "#   average_file_size = 4096\n"
          "#   block_size = 512\n"
          "#   ecc_type = crc\n"
          "#   crc_polynomial = 0x9960034c\n"
          "#   use_journal = false\n";
}
