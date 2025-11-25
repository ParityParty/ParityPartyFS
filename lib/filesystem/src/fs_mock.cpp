#include "filesystem/fs_mock.hpp"
#include <chrono>
#include <thread>

std::expected<void, FsError> FsMock::format()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    return {};
}

std::expected<void, FsError> FsMock::create(std::string path)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return {};
}
std::expected<inode_index_t, FsError> FsMock::open(std::string path)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return 10;
}
std::expected<void, FsError> FsMock::close(inode_index_t inode)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return {};
}
std::expected<void, FsError> FsMock::remove(std::string path)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return {};
}
std::expected<std::vector<std::uint8_t>, FsError> FsMock::read(
    inode_index_t inode, size_t offset, size_t size)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    return std::vector<std::uint8_t>(size, 'a');
}
std::expected<void, FsError> FsMock::write(
    inode_index_t inode, std::vector<std::uint8_t> buffer, size_t offset)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    return {};
}
std::expected<void, FsError> FsMock::createDirectory(std::string path)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return {};
}
std::expected<std::vector<std::string>, FsError> FsMock::readDirectory(std::string path)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return { std::vector { std::string("a"), std::string("b"), std::string("c") } };
}
