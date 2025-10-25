cmake_minimum_required(VERSION 3.14)
project(MyFuseFS LANGUAGES CXX)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FUSE3 REQUIRED fuse3)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${FUSE3_INCLUDE_DIRS}
    ../include
)

add_executable(examplefs main.cpp)
target_link_libraries(examplefs ${FUSE3_LIBRARIES})
target_compile_options(examplefs PRIVATE ${FUSE3_CFLAGS_OTHER})