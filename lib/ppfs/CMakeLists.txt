set(NAME ppfs)

add_library(${NAME} STATIC)

target_sources(${NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ppfs.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/data_structures.cpp
)

target_include_directories(${NAME} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${NAME}
    disk
    fusepp
    blockdevice
)
