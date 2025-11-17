#include "blockdevice/iblock_device.hpp"
#include "inode_manager/iinode_manager.hpp"

class FileReader {
    IInodeManager _iinode_manager;
    IBlockDevice _block_device;
};