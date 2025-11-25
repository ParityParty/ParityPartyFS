add_executable(usage_simulator main.cpp)
target_link_libraries(usage_simulator PRIVATE
        filesystem
        mock_user
)

add_subdirectory(mock_user)
