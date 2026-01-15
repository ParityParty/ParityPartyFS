#pragma once

#include "blockdevice/ecc_type.hpp"
#include "ecc_helpers/crc_polynomial.hpp"
#include "filesystem/types.hpp"

#include <cstdint>
#include <expected>
#include <iostream>
#include <string_view>

/**
 * Loads a filesystem configuration from a key=value text file.
 *
 * Fields not specified in the file keep their default values.
 * Validates that required fields (total_size, block_size) are present.
 *
 * @param path path to the configuration file
 * @return FsConfig on success, FsError on failure
 */
std::expected<FsConfig, FsError> load_fs_config(std::string_view path);

/**
 * Prints an example configuration file to stdout.
 */
void print_fs_config_usage(std::ostream& os = std::cout);
