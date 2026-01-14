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

    // 1️⃣ Wczytanie konfiguracji
    auto cfg_res = load_fs_config(config_path);
    if (!cfg_res.has_value()) {
        std::cerr << "Failed to load FsConfig: ";
        switch (cfg_res.error()) {
        case FsError::Config_IOError:
            std::cerr << "Cannot open config file\n";
            break;
        case FsError::Config_SyntaxError:
            std::cerr << "Syntax error in config file\n";
            break;
        case FsError::Config_InvalidValue:
            std::cerr << "Invalid value in config file\n";
            break;
        case FsError::Config_MissingField:
            std::cerr << "Missing required field in config file\n";
            break;
        case FsError::Config_UnknownKey:
            std::cerr << "Unknown key in config file\n";
            break;
        default:
            std::cerr << "Unknown error\n";
            break;
        }
        print_fs_config_usage(std::cerr);
        return 1;
    }

    FsConfig cfg = cfg_res.value();

    // 2️⃣ Tworzymy plikowy dysk
    FileDisk disk;
    auto create_res = disk.create(disk_image_path, cfg.total_size);
    if (!create_res.has_value()) {
        std::cerr << "Failed to create disk file: " << disk_image_path << "\n";
        return 1;
    }

    PpFS fs(disk, nullptr);

    // 4️⃣ Formatujemy
    auto fmt_res = fs.format(cfg);
    if (!fmt_res.has_value()) {
        std::cerr << "Failed to format filesystem\n";
        return 1;
    }

    // 5️⃣ Powodzenie
    std::cout << "Filesystem successfully formatted: " << disk_image_path << "\n";

    return 0;
}
