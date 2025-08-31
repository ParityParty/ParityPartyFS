#pragma once
#include "data_structures.hpp"
#include "disk/idisk.hpp"
#include "fusepp/Fuse.hpp"
#include <stdexcept>

class PpFS : public Fusepp::Fuse<PpFS> {
public:
    PpFS(IDisk& disk);
    ~PpFS() override = default;

    /**
     * Retrieves attributes of a file or directory.
     *
     * This function implements the `getattr` operation for the HelloFS filesystem.
     * It is responsible for providing file attributes for given paths. The
     * function populates the `stBuffer` structure with the appropriate metadata
     * based on the specified `path`.
     *
     * @param path The file or directory path for which the attributes are to be retrieved.
     * @param stBuffer A pointer to a `struct stat` that will be populated with the file's
     * attributes.
     * @param fuse_file_info Reserved for future use.
     *
     * @return 0 on success, or a negative error code (e.g., -ENOENT) if the path does not exist.
     *
     * Behavior:
     * - If the path matches the root directory (`rootPath()`), the function provides
     *   directory attributes.
     * - If the path matches the file path (`helloPath()`), the function provides
     *   file attributes, including the file's size equal to the length of the string
     *   from `helloStr()`.
     * - If the path does not match any known paths, the function returns -ENOENT.
     */
    static int getattr(const char*, struct stat*, struct fuse_file_info*);

    /**
     * Reads directory contents.
     *
     * This function implements the `readdir` operation for the HelloFS filesystem.
     * It populates the buffer with the entries for the given directory path. These
     * entries include `.` (current directory), `..` (parent directory), and any
     * additional entries specific to the filesystem implementation.
     *
     * @param path The directory path whose contents are to be listed.
     * @param buf A user-provided buffer where the directory entries will be filled.
     * @param filler A callback function to add entries to the buffer.
     * @param offset This parameter is unused in this implementation.
     * @param fi Reserved for future use.
     * @param flags Additional flags for the `readdir` operation.
     *
     * @return 0 on success, or a negative error code (e.g., -ENOENT) if the directory
     *         does not exist.
     *
     * Behavior:
     * - If the `path` matches the root directory (`rootPath()`), the function
     *   fills the buffer with `.` (current directory), `..` (parent directory),
     *   and the relative path of the `helloPath()` file.
     * - If the `path` does not match any known paths, the function returns -ENOENT.
     */
    static int readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset,
        struct fuse_file_info* fi, enum fuse_readdir_flags);

    /**
     * Opens a file for read-only access.
     *
     * This function implements the `open` operation for the HelloFS filesystem.
     * It is invoked to verify the accessibility of the file and its mode. The
     * function ensures that the specified file exists and is opened with the
     * correct permissions.
     *
     * @param path The path to the file being opened.
     * @param fi A pointer to `struct fuse_file_info` containing information about
     *           the file access flags.
     *
     * @return 0 on success, or a negative error code:
     *         - -ENOENT if the file does not exist.
     *         - -EACCES if the file is not opened with read-only access.
     *
     * Behavior:
     * - If the `path` matches the `helloPath()` file, it validates the access flags.
     * - If the `path` does not match any known paths, the function returns -ENOENT.
     * - Only read-only access (`O_RDONLY`) is permitted; other modes return -EACCES.
     */
    static int open(const char* path, struct fuse_file_info* fi);

    /**
     * Reads data from a file in the HelloFS filesystem.
     *
     * This function implements the `read` operation for the HelloFS filesystem.
     * It reads a specified number of bytes from the file at the given offset and
     * stores them in the provided buffer. The file content is defined by the
     * `helloStr()` string of the filesystem.
     *
     * @param path The path to the file from which data will be read.
     * @param buf A pointer to the buffer where the read data will be stored.
     * @param size The number of bytes to read.
     * @param offset The offset from the beginning of the file where the read operation should
     * start.
     * @param fuse_file_info Reserved for future use.
     *
     * @return The number of bytes read on success. Returns 0 if the offset is beyond the file's
     * size, or a negative error code (e.g., -ENOENT) if the file does not exist.
     *
     * Behavior:
     * - If the `path` matches the file path (`helloPath()`), the function reads data from the
     *   `helloStr()` string starting at the specified offset, up to the requested size or the
     *   end of the string, whichever comes first.
     * - If the `path` does not match any known file path, the function returns -ENOENT.
     */
    static int read(const char* path, char* buf, size_t size, off_t offset, fuse_file_info* fi);

    static int write(const char* path, const char* buf, size_t size, off_t offset, fuse_file_info*);

    static int create(const char*, mode_t, fuse_file_info*);

    static int mknod(const char* path, mode_t mode, dev_t dev);

    static int unlink(const char* path);

    /**
     * Formats disk to work with ppfs
     *
     * This function writes important metadata into a disk
     *
     * @return returns void on success or DiskError in case of an error
     */
    std::expected<void, DiskError> formatDisk(unsigned int block_size);

    /**
     * Retrieves the root directory path of the filesystem.
     *
     * This function returns a constant reference to the internal string
     * representing the root directory path used by the HelloFS filesystem.
     *
     * @return A constant reference to the string representing the root directory path.
     *
     * Usage Context:
     * - The root path is typically used to identify and match operations specific
     *   to the root directory within the filesystem.
     */
    const std::string& rootPath() const { return _root_path; }

    Directory getRootDirectory() const { return _root_dir; }

private:
    std::string _root_path = "/";

    IDisk& _disk;
    unsigned int _block_size;
    unsigned int _root_dir_block;

    SuperBlock _super_block;
    FileAllocationTable _fat;
    Directory _root_dir;

    std::expected<void, DiskError> _createFile(const std::string& name);
    std::expected<void, DiskError> _removeFile(const DirectoryEntry& entry);
    std::expected<std::vector<std::byte>, DiskError> _readFile(
        const DirectoryEntry& entry, size_t size, size_t offset);
    std::expected<void, DiskError> _writeFile(
        DirectoryEntry& entry, const std::vector<std::byte>& data, size_t offset);

    /**
     * Writes bytes starting from offset, then follows fat allocating new blocks if needed
     *
     * @param data data to be written to disk
     * @param address start address
     * @return returns DiskError on error, void otherwise
     */
    std::expected<void, DiskError> _writeBytes(const std::vector<std::byte>& data, size_t address);

    /**
     * Method finds address in memory of offset of given file
     *
     * @param entry file
     * @param offset offset
     * @return returns memory address on success, error otherwise
     */
    std::expected<size_t, DiskError> _findFileOffset(const DirectoryEntry& entry, size_t offset);

    std::expected<void, DiskError> _flushChanges();
};
