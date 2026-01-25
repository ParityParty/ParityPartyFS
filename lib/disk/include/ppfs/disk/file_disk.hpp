#pragma once

#include "ppfs/disk/idisk.hpp"

#include <cstddef>
#include <cstdint>
#include <fstream>

/**
 * File-backed disk implementation.
 *
 * Implements the IDisk interface using a binary file as the underlying
 * storage medium.
 */
class FileDisk : public IDisk {
public:
    /**
     * Constructs an unopened file disk.
     *
     * No file is associated with the disk until open() or create()
     * is successfully called.
     */
    FileDisk();

    /**
     * Closes the underlying file if it is open.
     */
    ~FileDisk() override;

    /**
     * Opens an existing file and treats it as a disk.
     *
     * The file must already exist and be readable and writable.
     * The disk size is inferred from the current file size.
     *
     * @param path Path to an existing file.
     * @return Empty on success, FsError on failure.
     */
    std::expected<void, FsError> open(std::string_view path);

    /**
     * Creates a new file-backed disk with a fixed size.
     *
     * If the file already exists, it will be truncated.
     *
     * @param path Path to the file to create.
     * @param size Size of the disk in bytes.
     * @return Empty on success, FsError on failure.s
     */
    std::expected<void, FsError> create(std::string_view path, size_t size);

    /**
     * Returns the total size of the disk in bytes.
     *
     * @return Disk size in bytes.
     */
    size_t size() override;

    [[nodiscard]] std::expected<void, FsError> read(
        size_t address, size_t size, static_vector<uint8_t>& data) override;

    [[nodiscard]] std::expected<size_t, FsError> write(
        size_t address, const static_vector<uint8_t>& data) override;

private:
    std::fstream _file;
    size_t _size = 0;
};
