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
    virtual std::expected<FileAttributes, FsError> getAttributes(inode_index_t inode_index) = 0;

    /**
     * Lookup a child entry inside a directory
     *
     * @param parent_index inode of the parent directory
     * @param name name of the entry to find
     * @return inode of the found entry on success, error otherwise
     */
    virtual std::expected<inode_index_t, FsError> lookup(
        inode_index_t parent_index, std::string_view name)
        = 0;

    /**
     * Read entries of a directory
     *
     * @param path Absolute path to directory
     * @return list of filenames and inodes on success, error otherwise
     */
    virtual std::expected<std::vector<DirectoryEntry>, FsError> getDirectoryEntries(
        inode_index_t inode)
        = 0;

    /**
     * Create new directory
     *
     * @param name name of the new directory
     * @param parent inode of the directory to create file in
     * @return inode of the new directory on success, error otherwise
     */
    virtual std::expected<inode_index_t, FsError> createDirectoryByParent(
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
    virtual std::expected<file_descriptor_t, FsError> openByInode(
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
    virtual std::expected<inode_index_t, FsError> createWithParentInode(
        std::string_view name, inode_index_t parent)
        = 0;
};