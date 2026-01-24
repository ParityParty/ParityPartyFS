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

# Create venv environment

set(VENV_DIR "${CMAKE_SOURCE_DIR}/.venv")

if (NOT EXISTS "${VENV_DIR}/bin/activate")
    message(STATUS "Creating Python virtual environment at ${VENV_DIR}")
    execute_process(
            COMMAND python3 -m venv ${VENV_DIR}
            RESULT_VARIABLE VENV_RESULT
    )
    if (NOT VENV_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to create Python virtual environment.")
    endif ()

    set(PIP_EXECUTABLE "${VENV_DIR}/bin/pip")
    message(STATUS "Installing Python dependencies from requirements.txt")
    execute_process(
            COMMAND "${PIP_EXECUTABLE}" install --upgrade pip
            COMMAND "${PIP_EXECUTABLE}" install -r "${CMAKE_SOURCE_DIR}/requirements.txt"
            RESULT_VARIABLE PIP_RESULT
    )
    if (NOT PIP_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to install Python dependencies.")
    endif ()
endif ()

# Subdirectories
add_subdirectory(lib)
add_subdirectory(unit_tests)
add_subdirectory(performance_tests)
add_subdirectory(usage_simulator)
add_subdirectory(simulation_runner)

if (NOT ENABLE_FREERTOS)
add_subdirectory(fuse_exec)
endif()
