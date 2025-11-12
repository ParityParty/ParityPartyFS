cmake_minimum_required(VERSION 3.14)

set(NAME performance_tests)

# Google Benchmark
include(FetchContent)
FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG main
)
# Avoid installing system-wide or building tests inside benchmark
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_USE_LIBCXX OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(benchmark)

add_executable(${NAME}
        bench_blockdevice.cpp
)

target_link_libraries(${NAME} PUBLIC
        benchmark::benchmark
        blockdevice
)

# Add a custom target to run benchmarks
add_custom_target(run_performance
        COMMAND ${NAME}
        DEPENDS ${NAME}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
