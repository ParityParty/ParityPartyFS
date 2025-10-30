#include "iblock_device.hpp"

class HammingBlockDevice : public IBlockDevice {
public:
    HammingBlockDevice(int block_size_power, IDisk& disk);

    std::expected<size_t, DiskError> writeBlock(
        const std::vector<std::byte>& data, DataLocation data_location) override;

    std::expected<std::vector<std::byte>, DiskError> readBlock(
        DataLocation data_location, size_t bytes_to_read) override;

    size_t rawBlockSize() const override;
    size_t dataSize() const override;
    size_t numOfBlocks() const override;

private:
    size_t _block_size;
    size_t _data_size;
    IDisk& _disk;

    std::vector<std::byte> _encodeData(const std::vector<std::byte>& data);
    std::vector<std::byte> _extractData(const std::vector<std::byte>& encoded_data);

    int _getBit(const std::vector<std::byte>& data, unsigned int index);
    void _setBit(std::vector<std::byte>& data, unsigned int index, int value);

    std::expected<std::vector<std::byte>, DiskError> _readAndFixBlock(int block_index);

};