add_executable(mount_ppfs mount_main.cpp)

add_executable(mkfs_ppfs mkfs_main.cpp)

target_link_libraries(mkfs_ppfs PRIVATE low_level_fuse disk)
target_link_libraries(mount_ppfs PRIVATE low_level_fuse disk)
