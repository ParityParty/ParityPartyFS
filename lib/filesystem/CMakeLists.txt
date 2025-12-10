set(NAME filesystem)

add_library(${NAME} STATIC)

target_sources(${NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ppfs.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ppfs_low_level.cpp
)

target_include_directories(${NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${NAME}
        inode_manager
        block_manager
        super_block_manager
        directory_manager
        blockdevice
        common
        data_collection
)
