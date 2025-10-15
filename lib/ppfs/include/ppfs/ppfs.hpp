#pragma once
#include "data_structures.hpp"
#include "disk/idisk.hpp"
#include "fusepp/Fuse.hpp"
#include <blockdevice/iblock_device.hpp>
#include <stdexcept>

class PpFS : public Fusepp::Fuse<PpFS> {
public:
    PpFS(IBlockDevice& block_device);
    ~PpFS() override = default;

    static int getattr(const char*, struct stat*, fuse_file_info*);

    static int readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset,
        fuse_file_info* fi, fuse_readdir_flags);

    static int open(const char* path, fuse_file_info* fi);

    static int read(const char* path, char* buf, size_t size, off_t offset, fuse_file_info* fi);

    static int write(const char* path, const char* buf, size_t size, off_t offset, fuse_file_info*);

    static int create(const char*, mode_t, fuse_file_info*);

    static int mknod(const char* path, mode_t mode, dev_t dev);

    static int unlink(const char* path);

    static int truncate(const char* path, off_t offset, fuse_file_info* fi);

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

    IBlockDevice& _block_device;
    
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
    std::expected<void, DiskError> _writeBytes(const std::vector<std::byte>& data, DataLocation address);

    /**
     * Method finds address in memory of offset of given file
     *
     * @param entry file
     * @param offset offset
     * @return returns memory address on success, error otherwise
     */
    std::expected<DataLocation, DiskError> _findFileOffset(const DirectoryEntry& entry, size_t offset);
    std::expected<void, DiskError> _truncateFile(DirectoryEntry& entry, size_t size);

    std::expected<void, DiskError> _flushChanges();
};
