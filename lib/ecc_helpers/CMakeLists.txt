set(NAME ecc_helpers)

add_library(${NAME} STATIC)

target_sources(${NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gf256.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/polynomial_gf256.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gf256_utils.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/crc_polynomial.cpp
)

target_include_directories(${NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${NAME}
        common
)