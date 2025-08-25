cmake_minimum_required(VERSION 3.14)
project(MyFuseFS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable top-level testing
include(CTest)
enable_testing()

# FUSE (system lib)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FUSE3 REQUIRED fuse3)

# Subdirectories
add_subdirectory(lib)
add_subdirectory(exec)
add_subdirectory(tests)
