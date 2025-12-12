add_executable(usage_simulator main.cpp)
target_link_libraries(usage_simulator PRIVATE
        filesystem
        simulation
        data_collection
)

add_subdirectory(simulation)
