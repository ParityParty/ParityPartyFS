#include "filesystem/fs_mock.hpp"
#include "mock_user/mock_user.hpp"

#include <fstream>
#include <iostream>
#include <thread>

struct ReadEvent {
    int time_stamp;
    size_t read_size;
    int time_ms;
};

struct BitFlipEvent {
    int time_stamp;
};

struct ErrorCorrectionEvent {
    int time_stamp;
};

typedef std::variant<ReadEvent, BitFlipEvent, ErrorCorrectionEvent> Event;

class Logger {
    std::ofstream file;

public:
    void log(Event event)
    {
        switch (event.index()) {
        }
    }
};

int main()
{
    FsMock fs;
    MockUser user(fs, {});

    for (int i = 0; i < 1000; i++) {
        std::cout << i << std::endl;
        user.step();
    }
    return 0;
}