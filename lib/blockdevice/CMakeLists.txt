set(NAME blockdevice)

add_library(${NAME} STATIC)

target_sources(${NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/raw_block_device.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/hamming_block_device.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/parity_block_device.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/rs_block_device.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/crc_block_device.cpp
)

target_include_directories(${NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${NAME}
        disk
        ecc_helpers
        common
        data_collection
)
