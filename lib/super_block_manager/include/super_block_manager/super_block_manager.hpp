#pragma once
#include "isuper_block_manager.hpp"

#include "blockdevice/iblock_device.hpp"
#include "super_block_manager/isuper_block_manager.hpp"

class SuperBlockManager : public ISuperBlockManager {
    IBlockDevice& _block_device;

public:
    SuperBlockManager(IBlockDevice& block_device);
};
