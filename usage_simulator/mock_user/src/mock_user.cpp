#include "mock_user/mock_user.hpp"

#include <chrono>
#include <iostream>
#include <thread>
MockUser::MockUser(IFilesystem& fs)
    : _fs(fs)
{
}
void MockUser::use(UseArgs args)
{
    int sleep_time_ms = (1 / args.avg_ops_per_sec) * 1000;
    std::cout << "formating" << std::endl;
    _fs.format();
    while (!args.token.stop_requested()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time_ms));
        std::cout << "writing to file!" << std::endl;

        _fs.write(10, std::vector<std::uint8_t>(args.max_write_size, 'a'), 22);
    }
}
