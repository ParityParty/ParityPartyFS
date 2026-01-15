cmake_minimum_required(VERSION 3.14)
project(MyFuseFS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option to enable FreeRTOS backend (OFF by default)
option(ENABLE_FREERTOS "Build PpFSMutex using FreeRTOS primitives" OFF)

# Enable top-level testing
include(CTest)
enable_testing()

# FUSE (system lib)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FUSE3 REQUIRED fuse3)

# Subdirectories
add_subdirectory(lib)
add_subdirectory(exec)
add_subdirectory(unit_tests)
add_subdirectory(performance_tests)
add_subdirectory(usage_simulator)
add_subdirectory(simulation_runner)