#include "disk/stack_disk.hpp"
#include "low_level_fuse/fuse_ppfs.hpp"

#include <iostream>

int main(int argc, char* argv[])
{
    try {
        StackDisk disk;
        PpFSLinux ppfs(disk);
        FsConfig options { .total_size = 4096,
            .average_file_size = 256,
            .block_size = 128,
            .ecc_type = ECCType::None,
            .use_journal = false };
        auto format_res = ppfs.format(options);

        if (!format_res.has_value()) {
            std::cerr << "Could not format😭: " << toString(format_res.error()) << std::endl;
            return 1;
        }

        FusePpFS fuse_ppfs(ppfs);

        return fuse_ppfs.run(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Filesystem crashed 😭: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Filesystem crashed for unknown reasons 💀" << std::endl;
        return 2;
    }
}
