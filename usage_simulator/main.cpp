#include "data_collection/data_colection.hpp"
#include "filesystem/fs_mock.hpp"
#include "mock_user/mock_user.hpp"

#include <fstream>
#include <iostream>
#include <thread>

int main()
{
    FsMock fs;
    Logger logger;
    MockUser user(fs, logger, {});

    for (int i = 0; i < 100; i++) {
        logger.step();
        user.step();
    }
    return 0;
}