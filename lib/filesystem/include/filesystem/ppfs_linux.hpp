#include "filesystem/ppfs.hpp"

struct FileAttributes {
    size_t size;
    size_t block_size;
    InodeType type;
};

/**
 * This class extends PpFS with functionalities needed for working
 * in linux envirionment
 */
class PpFSLinux : public PpFS {
public:
    PpFSLinux(IDisk& disk);
    /**
     * This method returns attribues of a file
     * specified by a given inode.
     */
    std::expected<FileAttributes, FsError> getAttributes(inode_index_t inode_index);

    std::expected<inode_index_t, FsError> lookup(inode_index_t parent_index, std::string_view name)
    {
        auto lookup_res = _getInodeFromParent(parent_index, name);

        return lookup_res;
    }
};