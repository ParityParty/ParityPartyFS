#pragma once
#include "bitmap/bitmap.hpp"
#include "inode_manager/iinode_manager.hpp"

class InodeManager : public IInodeManager {
    Bitmap& _bitmap;
    block_index_t _table_start;

public:
    InodeManager(Bitmap& bitmap, block_index_t table_start);
};
