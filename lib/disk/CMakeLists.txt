set(NAME disk)

add_library(${NAME} STATIC)

target_include_directories(${NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${NAME}
        common
)
