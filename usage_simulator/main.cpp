#include "filesystem/fs_mock.hpp"
#include "mock_user/mock_user.hpp"
#include <iostream>
#include <thread>

int main()
{
    FsMock fs;
    MockUser user(fs);
    std::jthread thread { [&user](std::stop_token stoken) { user.use({ .token = stoken }); } };

    std::this_thread::sleep_for(std::chrono::seconds(5));
    thread.request_stop();
    thread.join();
    return 0;
}