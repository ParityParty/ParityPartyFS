set(NAME disk)

add_library(${NAME} STATIC)

target_include_directories(${NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_sources(${NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/file_disk.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/heap_disk.cpp
)

target_link_libraries(${NAME}
        common
)
