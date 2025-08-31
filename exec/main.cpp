#include "disk/stack_disk.hpp"
#include "ppfs/ppfs.hpp"
#include <iostream>

int main(int argc, char* argv[])
{
    try {
        StackDisk disk;
        PpFS fs(disk);
        return fs.run(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Filesystem crashed 😭: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Filesystem crashed for unknown reasons 💀" << std::endl;
        return 2;
    }
}
