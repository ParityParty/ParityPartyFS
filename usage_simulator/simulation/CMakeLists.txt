set(NAME simulation)

add_library(${NAME} STATIC)

target_sources(${NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/mock_user.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/bit_flipper.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/simulation_config.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/irradiated_disk.cpp
)

target_include_directories(${NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${NAME}
        filesystem
        common
        data_collection
)
