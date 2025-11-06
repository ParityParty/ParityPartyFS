set(NAME unit_tests)

# Bring in googletest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(${NAME}
        test_ppfs.cpp
        test_stack_disk.cpp
        test_ppfs_data_structures.cpp
        test_hamming_block_device.cpp
        test_bitmap.cpp
)

target_link_libraries(${NAME} PUBLIC
    bitmap
    disk
    ppfs
    blockdevice
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(${NAME}
    PROPERTIES
        LABELS "unit"
        ENVIRONMENT "GTEST_COLOR=1"   # force colors in output 🌈
)
