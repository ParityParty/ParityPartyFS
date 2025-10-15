#include "blockdevice/iblock_device.hpp"

class RawBlockDevice : public IBlockDevice {
private:
    size_t _block_size;
    IDisk& _disk;
public:
    RawBlockDevice(size_t block_size, IDisk& disk);

    std::expected<size_t, DiskError> writeBlock(
        const std::vector<std::byte>& data, DataLocation data_location) override;
    
    std::expected<std::vector<std::byte>, DiskError> readBlock(
        DataLocation data_location, size_t bytes_to_read) override;
    
    size_t rawBlockSize() const override;
    size_t dataSize() const override;

};