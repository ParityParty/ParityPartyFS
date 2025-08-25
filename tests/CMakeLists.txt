# Bring in googletest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(test_ppfs test_ppfs.cpp)
target_link_libraries(test_ppfs
    ppfs
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_ppfs
    PROPERTIES
        LABELS "unit"
        ENVIRONMENT "GTEST_COLOR=1"   # force colors in output 🌈
)
