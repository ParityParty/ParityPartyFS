#include "super_block_manager/super_block_manager.hpp"

#include "blockdevice/iblock_device.hpp"

SuperBlockManager::SuperBlockManager(IBlockDevice& block_device)
    : _block_device(block_device)
{
}