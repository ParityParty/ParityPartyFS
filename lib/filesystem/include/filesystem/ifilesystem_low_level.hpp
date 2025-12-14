#include "common/static_vector.hpp"
#include "directory_manager/directory.hpp"
#include "filesystem/ifilesystem.hpp"
#include "inode_manager/inode.hpp"

struct FileAttributes {
    size_t size;
    size_t block_size;
    InodeType type;
};

/**
 * Interface of basic filesystem operations including low level
 * operations with inodes.
 */
struct IFilesystemLowLevel : public virtual IFilesystem {
public:
    /**
     * Get attributes of a file or directory
     *
     * @param inode_index inode of the object to query
     * @return attributes on success, error otherwise
     */
    [[nodiscard]] virtual std::expected<FileAttributes, FsError> getAttributes(inode_index_t inode_index) = 0;

    /**
     * Lookup a child entry inside a directory
     *
     * @param parent_index inode of the parent directory
     * @param name name of the entry to find
     * @return inode of the found entry on success, error otherwise
     */
    [[nodiscard]] virtual std::expected<inode_index_t, FsError> lookup(
        inode_index_t parent_index, std::string_view name)
        = 0;

    /**
     * Read entries of a directory
     *
     * @param inode inode of the directory
     * @param buf buffer to fill with directory entries, must have sufficient capacity
     * @param offset entry offset to start reading from
     * @param size maximum number of entries to read (0 = all remaining entries)
     * @return void on success, error otherwise
     */
    [[nodiscard]] virtual std::expected<void, FsError> getDirectoryEntries(
        inode_index_t inode, static_vector<DirectoryEntry>& buf, size_t offset, size_t size)
        = 0;

    /**
     * Create new directory
     *
     * @param name name of the new directory
     * @param parent inode of the directory to create file in
     * @return inode of the new directory on success, error otherwise
     */
    [[nodiscard]] virtual std::expected<inode_index_t, FsError> createDirectoryByParent(
        inode_index_t parent, std::string_view name)
        = 0;

    /**
     * Opens file for read/write operations.
     *
     * Method can check data integrity. Every call to open should be paired with
     * close one the returned inode
     *
     * @param inode inode index of the file to be opened
     * @param mode mode in which to open the file
     * @return file descriptor to be used with read, write, close operations on success,
     * error otherwise
     */
    [[nodiscard]] virtual std::expected<file_descriptor_t, FsError> openByInode(
        inode_index_t inode, OpenMode mode)
        = 0;

    /**
     * Create new file
     *
     * Does not open file.
     *
     * @param name name of the new file
     * @param parent inode of the directory to create file in
     * @return inode of the new file on success, error otherwise
     */
    [[nodiscard]] virtual std::expected<inode_index_t, FsError> createWithParentInode(
        std::string_view name, inode_index_t parent)
        = 0;

    /**
     * Remove a file or directory by name
     *
     * @param inode inode of the parent directory
     * @param name name of the entry to remove
     * @param recursive whether to remove directories recursively
     * @return success value on success, error otherwise
     */
    [[nodiscard]] virtual std::expected<void, FsError> removeByNameAndParent(
        inode_index_t inode, std::string_view name, bool recursive = false)
        = 0;

    /**
     * Truncate or extend a file to a new size.
     * - Fails if the file is open with exclusive flag.
     * - Fails if reducing size and the file pointer is beyond the new size (for normally opened
     * files without append).
     *
     * @param inode inode of the file
     * @param new_size desired file size
     * @return success on success, error otherwise
     */
    [[nodiscard]] virtual std::expected<void, FsError> truncate(inode_index_t inode, size_t new_size) = 0;
};