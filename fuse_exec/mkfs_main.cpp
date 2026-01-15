#include "disk/file_disk.hpp"
#include "filesystem/fs_config_helpers.hpp"
#include "filesystem/ppfs.hpp"

#include <iostream>
#include <memory>

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <config_file_path> <disk_image_path>\n";
        print_fs_config_usage(std::cerr);
        return 1;
    }

    std::string_view config_path = argv[1];
    std::string_view disk_image_path = argv[2];

    auto cfg_res = load_fs_config(config_path);
    if (!cfg_res.has_value()) {
        std::cerr << "Failed to load FsConfig: " << toString(cfg_res.error()) << std::endl;
        print_fs_config_usage(std::cerr);
        return 1;
    }

    FsConfig cfg = cfg_res.value();

    FileDisk disk;
    auto create_res = disk.create(disk_image_path, cfg.total_size);
    if (!create_res.has_value()) {
        std::cerr << "Failed to create disk file at: " << disk_image_path
                  << ", error: " << toString(create_res.error()) << std::endl;
        return 1;
    }

    PpFS fs(disk, nullptr);

    auto fmt_res = fs.format(cfg);
    if (!fmt_res.has_value()) {
        std::cerr << "Failed to format filesystem: " << toString(fmt_res.error()) << std::endl;
        return 1;
    }

    std::cout << "Filesystem successfully formatted: " << disk_image_path << "\n";

    return 0;
}
