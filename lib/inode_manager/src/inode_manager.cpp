#include "inode_manager/inode_manager.hpp"

InodeManager::InodeManager(Bitmap& bitmap, block_index_t table_start)
    : _bitmap(bitmap)
    , _table_start(table_start)
{
}
