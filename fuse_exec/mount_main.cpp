#include "ppfs/disk/file_disk.hpp"
#include "ppfs/filesystem/fs_config_helpers.hpp" // dla load_fs_config
#include "ppfs/low_level_fuse/fuse_ppfs.hpp"
#include <iostream>

int main(int argc, char* argv[])
{
    try {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0]
                      << " <disk_file> <mount_point> [config_file] [-- fuse args...]\n";
            return 1;
        }

        std::string disk_path = argv[1];
        std::string mount_point = argv[2];
        std::string cfg_path = (argc >= 4) ? argv[3] : "";

        FileDisk disk;
        // create/open disk, format, etc.

        auto open_res = disk.open(disk_path);
        if (!open_res.has_value()) {
            std::cerr << "Failed to open disk file: " << disk_path << "\n";
            return 1;
        }

        PpFSLowLevel ppfs(disk);
        auto init_res = ppfs.init();
        if (!init_res.has_value()) {
            std::cerr << "Disk not formatted...\n";
            return 1;
        }

        std::vector<char*> fuse_argv;
        fuse_argv.push_back(argv[0]); // nazwa programu
        fuse_argv.push_back(const_cast<char*>(mount_point.c_str()));

        for (int i = 3; i < argc; ++i) {
            if (std::string(argv[i]) == "--") {
                for (int j = i + 1; j < argc; ++j)
                    fuse_argv.push_back(argv[j]);
                break;
            }
        }

        FusePpFS fuse_ppfs(ppfs);

        return fuse_ppfs.run(fuse_argv.size(), fuse_argv.data());
    }

    catch (const std::exception& e) {
        std::cerr << "Filesystem crashed 😭: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Filesystem crashed for unknown reasons 💀\n";
        return 2;
    }
}
