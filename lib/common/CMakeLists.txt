set(NAME common)

add_library(${NAME} INTERFACE)

target_include_directories(${NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
